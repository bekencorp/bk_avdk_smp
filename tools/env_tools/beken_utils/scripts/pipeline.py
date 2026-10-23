#!/usr/bin/env python3
"""Declarative pack pipeline.

A pipeline is an ordered list of action names. Where that list comes from:
  * pack.json's optional "pipelines" object, e.g.
        {"pipelines": {"all": ["GEN_PPC", "SIGN_APP", "PACK_CONTAINERS"]}}
    where a preset may reference another preset by name;
  * DEFAULT_PIPELINE when pack.json has no "pipelines" key, so existing
    projects keep working without any config change.

Stage S1 (now) keeps the substrate only: pack_all() resolves the step list and
runs it, and the whole pre-pipeline implementation is wrapped as a single
action (LEGACY_PACK_ALL), so behavior and artifacts are unchanged. Later stages
split that action into fine grained ones (GEN_PPC / SIGN_APP /
PACK_CONTAINERS / ...) and add the split-signing steps.
"""

import json
import logging
import os

from .common import PIPELINES_KEY, parse_bool
from .ota import OTA
from .partition import Partitions
from .security import Security

DEFAULT_PIPELINE = ['GEN_PPC', 'CHECK_KEYS', 'GEN_OTP_EFUSE', 'PREPARE_BINS',
                    'SIGN_MANIFEST', 'SIGN_APP', 'SIGN_APP_FOR_OTA', 'COMPRESS_APP',
                    'SIGN_OTA', 'PACK_CONTAINERS', 'INSTALL', 'IMAGE_CHECK']

# Built-in presets. A pack.json "pipelines" entry with the same name overrides
# the built-in one. The split-signing presets split the same action set across
# two machines:
#   pack server: hash -> (ship *_hash.json) -> apply-sig
#                -> hash-ota -> (ship ota_hash.json) -> apply-sig-ota -> pack
#   sign server: sign-hash / sign-hash-ota (the only side holding the key)
# The OTA stages form a second round trip because the OVERWRITE ota payload is
# the compressed *signed* app image: its digest cannot exist before
# APPLY_SIG_APP. XIP hashes the unsigned concatenation and would tolerate a
# single round trip, but shares the same sequence so one flow covers both modes.
PRESETS = {
    'all': DEFAULT_PIPELINE,

    'hash':       ['CHECK_CONFIG', 'PREPARE_BINS', 'HASH_MANIFEST', 'HASH_APP'],
    # No CHECK_CONFIG here: the sign server only needs security.csv and the
    # *_hash.json files, not the firmware bins.
    'sign-hash':  ['CHECK_KEYS', 'SIGN_HASH_MANIFEST', 'SIGN_HASH_APP'],
    'apply-sig':  ['CHECK_CONFIG', 'APPLY_SIG_MANIFEST', 'APPLY_SIG_APP'],

    # COMPRESS_APP is a no-op for XIP and must run before the OVERWRITE hash.
    'hash-ota':      ['CHECK_CONFIG', 'COMPRESS_APP', 'HASH_OTA'],
    'sign-hash-ota': ['CHECK_KEYS', 'SIGN_HASH_OTA'],
    'apply-sig-ota': ['CHECK_CONFIG', 'APPLY_SIG_OTA'],

    'pack':       ['CHECK_CONFIG', 'GEN_PPC', 'GEN_OTP_EFUSE', 'PACK_CONTAINERS',
                   'INSTALL', 'IMAGE_CHECK'],

    'pack-server': ['hash', 'apply-sig', 'hash-ota', 'apply-sig-ota', 'pack'],
    'sign-server': ['sign-hash', 'sign-hash-ota'],
}

ACTIONS = {}


def register(cls):
    """Class decorator: register an action under its `name`."""
    ACTIONS[cls.name] = cls
    return cls


class PackContext:
    """Per-build state shared by every action of a pipeline run."""

    def __init__(self, config_dir=None, aes_key=None, pack_json='pack.json'):
        self.config_dir = config_dir
        self.aes_key = aes_key
        self.pack_json = pack_json
        self._security = None
        self._ota = None
        self._partitions = None

    def chdir(self):
        if self.config_dir != None:
            logging.debug(f'cd {self.config_dir}')
            os.chdir(self.config_dir)

    @property
    def security(self):
        """security.csv, read once per build."""
        if self._security == None:
            self._security = Security('security.csv')
        return self._security

    @property
    def ota(self):
        """ota.csv, read once per build."""
        if self._ota == None:
            self._ota = OTA('ota.csv')
        return self._ota

    @property
    def otp_flash_aes_key(self):
        """Flash AES key written into otp_efuse_config.json.

        --aes_key wins; otherwise the raw security.csv value is used (the two
        differ when security.csv holds a 256-bit key).
        """
        if self.aes_key != None:
            return self.aes_key
        return self.security.raw_flash_aes_key

    @property
    def ota_type(self):
        return self.ota.get_strategy()

    @property
    def boot_ota(self):
        return self.ota.get_boot_ota()

    @property
    def partitions(self):
        """partitions.csv model, built once per build and shared by actions."""
        if self._partitions == None:
            s = self.security
            self._partitions = Partitions('partitions.csv', self.ota_type, self.boot_ota,
                                          s.secureboot_en, s.crc_en, s.sig_verify_en)
        return self._partitions

    @property
    def flash_aes_key(self):
        """Flash AES key used to encrypt the code partitions.

        --aes_key wins; otherwise the security.csv value is used (already
        trimmed to the effective key length there).
        """
        if self.aes_key != None:
            return self.aes_key
        return self.security.flash_aes_key

    @property
    def crc_en(self):
        """True when security.csv enables the flash CRC interleaving."""
        return parse_bool(self.security.crc_en)

    def steps(self, preset='all'):
        """Resolve `preset` into the ordered step list for this build."""
        pipelines = dict(PRESETS)
        pipelines.update(load_pipelines(self.pack_json))
        # `all` may fall back to the built-in order; any other unknown name is a
        # typo and must not silently run the whole pipeline.
        if preset not in pipelines and preset != 'all' and ',' not in preset:
            logging.error(f'unknown pipeline preset "{preset}", '
                          f'available: {", ".join(sorted(pipelines))}')
            exit(1)
        return resolve_steps(preset, pipelines)


class Action:
    name = 'BASE'

    def __init__(self, ctx):
        self.ctx = ctx

    def run(self):
        raise NotImplementedError

    def bl2_partition(self):
        """Signing steps only apply when pack.json declares a bl2 partition."""
        pbl2 = self.ctx.partitions.find_partition_by_name('bl2')
        if pbl2 == None:
            logging.debug(f'{self.name}: pack.json has no bl2 partition, skipped')
        return pbl2


def load_pipelines(pack_json='pack.json'):
    """Return the "pipelines" object of pack.json, {} when absent or legacy."""
    if not os.path.isfile(pack_json):
        return {}

    try:
        with open(pack_json, 'r') as f:
            data = json.load(f)
    except (OSError, ValueError) as e:
        logging.warning(f'cannot read pipelines from {pack_json}: {e}')
        return {}

    pipelines = data.get(PIPELINES_KEY) if isinstance(data, dict) else None
    if pipelines == None:
        return {}
    if not isinstance(pipelines, dict):
        logging.warning(f'{pack_json}: "{PIPELINES_KEY}" must be an object, ignored')
        return {}
    return pipelines


def resolve_steps(preset, pipelines=None, default=None):
    """Resolve a preset name (or a "A,B,C" list) into an ordered step list.

    Presets may reference other presets; a circular reference is an error.
    An unknown preset name falls back to `default` so that projects without a
    "pipelines" declaration keep the built-in order.
    """
    pipelines = pipelines or {}
    default = DEFAULT_PIPELINE if default == None else default

    if preset not in pipelines:
        if ',' in preset:
            return [s.strip() for s in preset.split(',') if s.strip()]
        return list(default)

    steps = []
    visiting = set()

    def _expand(names):
        for name in names:
            if name in pipelines:
                if name in visiting:
                    logging.error(f'circular pipeline preset "{name}"')
                    exit(1)
                visiting.add(name)
                _expand(pipelines[name])
                visiting.discard(name)
            else:
                steps.append(name)

    _expand(pipelines[preset])
    return steps or list(default)


def list_steps(pack_json='pack.json'):
    """Print every available preset (with its expansion) and registered action."""
    pipelines = dict(PRESETS)
    pipelines.update(load_pipelines(pack_json))

    print('presets:')
    for name in sorted(pipelines):
        if name not in PRESETS:
            source = f'  ({pack_json})'
        else:
            source = ''
        print(f'  {name:<16}{source}')
        for step in resolve_steps(name, pipelines):
            print(f'      {step}')

    print('actions:')
    for name in sorted(ACTIONS):
        print(f'  {name}')


def run_pipeline(steps, ctx):
    """Run the actions in order. Unknown names abort the build."""
    for name in steps:
        cls = ACTIONS.get(name)
        if cls == None:
            logging.error(f'unknown pipeline action "{name}", '
                          f'available: {", ".join(sorted(ACTIONS))}')
            exit(1)
        logging.info(f'pipeline: {name}')
        cls(ctx).run()
