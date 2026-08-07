#!/usr/bin/env python3
"""BK7259 dump test orchestrator; all device I/O goes through view_log.sh."""

import argparse
import csv
import datetime as dt
import html
import json
import os
import random
import re
import signal
import subprocess
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Mapping, Optional, Sequence, Tuple

from dump_integrity import AnalysisResult, analyze_file, load_manifest

CASE_ID_RE = re.compile(r"^[A-Z0-9_]{1,64}$")
VARIABLE_RE = re.compile(r"\$\{([A-Z0-9_]+)\}")
ALLOWED_VARIABLES = {
    "CASE_ID", "SSID", "PASSWORD", "GATEWAY", "PING_COUNT", "PING_LENGTH",
    "FLASH_ADDRESS", "FLASH_LENGTH", "FS_MOUNT", "FS_TEST_PATH",
    "QSPI_ID", "QSPI_ADDRESS", "QSPI_LENGTH", "RANDOM_PING_COUNT",
}
SKIPPED = {"PRECONDITION_FAILED", "SKIPPED_DISABLED", "SKIPPED_UNSAFE", "DRY_RUN"}
DUMP_END_EXPECT = r"AP memory dump end|user except handler end"
STATUS_RE = re.compile(
    r"DUMP_ORCH_STATUS\s+raw=\S+\s+valid=(\d+)\s+scenario=(\d+)\s+"
    r"phase=(\d+)\s+winner=(\d+)\s+follower=(\d+)"
)
OWNER_RE = re.compile(r"DUMP_ORCH_OWNER\s+winner=(\d+)\s+follower_wait=(\d+)")
STOP_REQUESTED = False


def _stop(_signum: int, _frame: Any) -> None:
    global STOP_REQUESTED
    STOP_REQUESTED = True


def _load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as stream:
        return json.load(stream)


def _number(mapping: Mapping[str, Any], key: str, minimum: float, default: float) -> float:
    value = mapping.get(key, default)
    if not isinstance(value, (int, float)) or isinstance(value, bool) or value < minimum:
        raise ValueError("{} must be a number >= {}".format(key, minimum))
    return float(value)


def _safe_child(base: Path, name: str) -> Path:
    candidate = (base / name).resolve()
    if base.resolve() not in candidate.parents:
        raise ValueError("unsafe output path: {}".format(name))
    return candidate


def validate_config(config: Dict[str, Any], cases: List[Dict[str, Any]]) -> None:
    transport = config.get("transport")
    run = config.get("run")
    if not isinstance(transport, dict) or not isinstance(run, dict):
        raise ValueError("config requires transport and run objects")
    mode = transport.get("mode")
    if mode not in ("local", "remote"):
        raise ValueError("transport.mode must be local or remote")
    script = Path(str(transport.get("view_log_script", ""))).expanduser()
    if not script.is_absolute() or not script.is_file():
        raise ValueError("transport.view_log_script must name an existing absolute file")
    port = transport.get("port")
    if not isinstance(port, str) or not port or any(char in port for char in "\r\n\0"):
        raise ValueError("transport.port is invalid")
    baudrate = transport.get("baudrate", 115200)
    if not isinstance(baudrate, int) or baudrate <= 0:
        raise ValueError("transport.baudrate must be a positive integer")
    for key in ("fixed_repeats", "random_cases", "random_seed"):
        if not isinstance(run.get(key), int) or run[key] < 0:
            raise ValueError("run.{} must be a non-negative integer".format(key))
    for key in ("prepare_timeout_s", "dump_timeout_s", "boot_timeout_s", "recovery_timeout_s"):
        _number(run, key, 0.1, 1)
    minimum = _number(run, "random_delay_min_s", 0, 0)
    maximum = _number(run, "random_delay_max_s", 0, 0)
    if maximum < minimum:
        raise ValueError("random delay maximum must be >= minimum")
    output = run.get("output_dir")
    if not isinstance(output, str) or not output.strip():
        raise ValueError("run.output_dir is required")
    for section_name, keys in (
        ("network", ("ssid", "password", "gateway")),
        ("filesystem", ("mount_point", "test_path")),
        ("flash", ("address", "length")),
        ("qspi", ("address", "length")),
    ):
        section = config.get(section_name, {})
        if not isinstance(section, dict):
            raise ValueError("{} must be an object".format(section_name))
        for key in keys:
            value = str(section.get(key, ""))
            if any(char in value for char in "\r\n\0"):
                raise ValueError("{}.{} contains a forbidden control character".format(section_name, key))
    flash = config.get("flash", {})
    if flash.get("enabled", False):
        try:
            flash_address = int(str(flash.get("address", "0")), 0)
            flash_length = int(str(flash.get("length", "0")), 0)
        except ValueError as exc:
            raise ValueError("flash address and length must be integers") from exc
        if (
            flash_address == 0 or flash_address % 4096 != 0
            or flash_length == 0 or flash_length % 4096 != 0
            or flash_length > 64 * 1024
        ):
            raise ValueError(
                "enabled flash range must be nonzero, 4 KiB aligned, and at most 64 KiB"
            )
    seen = set()
    for case in cases:
        if not isinstance(case, dict):
            raise ValueError("every case must be an object")
        case_id = case.get("id")
        if not isinstance(case_id, str) or not CASE_ID_RE.fullmatch(case_id):
            raise ValueError("invalid case id: {!r}".format(case_id))
        if case_id in seen:
            raise ValueError("duplicate case id: {}".format(case_id))
        seen.add(case_id)
        if case.get("target") not in ("AP", "CP"):
            raise ValueError("{} has invalid target".format(case_id))
        core = case.get("core", 0)
        if core not in (None, 0, 1) or case["target"] == "CP" and core not in (None, 0):
            raise ValueError("{} has invalid core".format(case_id))
        if not isinstance(case.get("weight", 1), int) or case.get("weight", 1) <= 0:
            raise ValueError("{} weight must be positive".format(case_id))
        if not isinstance(case.get("trigger"), str) or not case["trigger"]:
            raise ValueError("{} trigger must be a non-empty string".format(case_id))
        scenario = case.get("scenario")
        if scenario is not None and (not isinstance(scenario, str) or not scenario):
            raise ValueError("{} scenario must be a non-empty string".format(case_id))
        if "dump_end_expect" in case:
            if not isinstance(case["dump_end_expect"], str) or not case["dump_end_expect"]:
                raise ValueError("{} dump_end_expect must be a non-empty string".format(case_id))
            re.compile(case["dump_end_expect"])
        if "dump_timeout_s" in case:
            _number(case, "dump_timeout_s", 0.1, 1)
        if "status_command" in case and (
            not isinstance(case["status_command"], str) or not case["status_command"]
        ):
            raise ValueError("{} status_command must be a non-empty string".format(case_id))
        if "status_scenario" in case and (
            not isinstance(case["status_scenario"], int)
            or isinstance(case["status_scenario"], bool)
            or case["status_scenario"] not in range(1, 6)
        ):
            raise ValueError("{} status_scenario is invalid".format(case_id))
        expected_winner = case.get("expected_winner")
        winners = expected_winner if isinstance(expected_winner, list) else [expected_winner]
        if expected_winner is not None and (
            not winners or any(
                isinstance(item, bool) or item not in (1, 2, 3)
                for item in winners
            )
        ):
            raise ValueError("{} expected_winner is invalid".format(case_id))
        if (
            isinstance(case.get("expected_follower"), bool)
            or case.get("expected_follower") not in (None, 1, 2, 3)
        ):
            raise ValueError("{} expected_follower is invalid".format(case_id))
        if (
            "expected_phase" in case
            and (
                not isinstance(case["expected_phase"], int)
                or isinstance(case["expected_phase"], bool)
                or case["expected_phase"] not in range(1, 4)
            )
        ):
            raise ValueError("{} expected_phase is invalid".format(case_id))
        for field in ("prepare", "recovery"):
            if not isinstance(case.get(field, []), list) or not all(isinstance(item, str) for item in case.get(field, [])):
                raise ValueError("{} {} must be a string array".format(case_id, field))
        commands = (
            case.get("prepare", []) + [case.get("trigger", "")]
            + ([case["status_command"]] if case.get("status_command") else [])
            + case.get("recovery", [])
        )
        for command in commands:
            if any(char in command for char in "\r\n\0"):
                raise ValueError("{} command contains a forbidden control character".format(case_id))
            unknown = set(VARIABLE_RE.findall(command)) - ALLOWED_VARIABLES
            if unknown:
                raise ValueError("{} uses unknown variables: {}".format(case_id, sorted(unknown)))


def _variables(config: Dict[str, Any], case_id: str, rng: random.Random) -> Dict[str, str]:
    network = config.get("network", {})
    flash = config.get("flash", {})
    filesystem = config.get("filesystem", {})
    qspi = config.get("qspi", {})
    ping_max = max(1, int(network.get("ping_count", 1000)))
    return {
        "CASE_ID": case_id,
        "SSID": str(network.get("ssid", "")),
        "PASSWORD": str(network.get("password", "")),
        "GATEWAY": str(network.get("gateway", "")),
        "PING_COUNT": str(ping_max),
        "PING_LENGTH": str(network.get("ping_length", 1400)),
        "FLASH_ADDRESS": str(flash.get("address", "0x0")),
        "FLASH_LENGTH": str(flash.get("length", "0x1000")),
        "FS_MOUNT": str(filesystem.get("mount_point", "/lfs")),
        "FS_TEST_PATH": str(filesystem.get("test_path", "/lfs/dump_test.dat")),
        "QSPI_ID": str(qspi.get("id", 0)),
        "QSPI_ADDRESS": str(qspi.get("address", "0x0")),
        "QSPI_LENGTH": str(qspi.get("length", 4096)),
        "RANDOM_PING_COUNT": str(rng.randint(max(1, ping_max // 4), ping_max)),
    }


def _expand(template: str, values: Mapping[str, str]) -> str:
    unknown = set(VARIABLE_RE.findall(template)) - ALLOWED_VARIABLES
    if unknown:
        raise ValueError("unknown command variables: {}".format(sorted(unknown)))
    return VARIABLE_RE.sub(lambda match: values[match.group(1)], template)


def _dump_completion_expect(
    case: Mapping[str, Any], run: Mapping[str, Any],
) -> str:
    dump_end = str(case.get("dump_end_expect", DUMP_END_EXPECT))
    boot = str(run["boot_expect"])
    return "(?:{})|(?:{})".format(dump_end, boot)


def _sanitize_text(text: str, secrets: Sequence[str]) -> str:
    for secret in sorted((item for item in secrets if item), key=len, reverse=True):
        text = text.replace(secret, "<redacted>")
    return text


def _sanitize_log(path: Path, secrets: Sequence[str]) -> None:
    if not path.exists() or not any(secrets):
        return
    text = path.read_text(encoding="utf-8", errors="replace")
    clean = _sanitize_text(text, secrets)
    if clean != text:
        temporary = path.with_suffix(path.suffix + ".redacting")
        temporary.write_text(clean, encoding="utf-8")
        os.replace(str(temporary), str(path))


class ViewLog:
    def __init__(self, config: Dict[str, Any], secrets: Sequence[str]) -> None:
        transport = config["transport"]
        self.script = str(Path(transport["view_log_script"]).resolve())
        self.mode = str(transport["mode"])
        self.port = str(transport["port"])
        self.baudrate = str(transport.get("baudrate", 115200))
        self.secrets = list(secrets)
        search_roots = (Path.cwd(),) + tuple(Path.cwd().parents)
        self.cwd = next(
            (path for path in search_roots if (path / "bk7259.env").is_file()),
            Path.cwd(),
        )

    def call(
        self, log: Path, duration: Optional[float] = None, command: Optional[str] = None,
        expect: Optional[str] = None, timeout: Optional[float] = None,
    ) -> Tuple[int, str]:
        log_offset = log.stat().st_size if log.exists() else 0
        args = [
            "bash", self.script, "--{}".format(self.mode), "--port", self.port,
            "--baudrate", self.baudrate, "--log-file", str(log),
            "--no-clean-temp-logs", "--no-crash-parse",
        ]
        if command is not None:
            args.extend(("--send", command))
        if expect:
            args.extend(("--expect", expect))
            args.extend(("--timeout", str(max(1, int(timeout or 10)))))
        elif duration is not None:
            args.extend(("--duration", str(max(0.1, duration))))
        try:
            process = subprocess.Popen(
                args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True, encoding="utf-8", errors="replace",
                cwd=str(self.cwd),
            )
            while True:
                try:
                    output, _ = process.communicate(timeout=0.5)
                    break
                except subprocess.TimeoutExpired:
                    if STOP_REQUESTED:
                        process.terminate()
                        try:
                            output, _ = process.communicate(timeout=5)
                        except subprocess.TimeoutExpired:
                            process.kill()
                            output, _ = process.communicate()
                        return 130, _sanitize_text(output or "", self.secrets)
                    if expect and log.exists():
                        with log.open("r", encoding="utf-8", errors="replace") as stream:
                            stream.seek(log_offset)
                            appended = stream.read()
                        if re.search(expect, appended, re.IGNORECASE):
                            process.terminate()
                            try:
                                output, _ = process.communicate(timeout=5)
                            except subprocess.TimeoutExpired:
                                process.kill()
                                output, _ = process.communicate()
                            return 0, _sanitize_text(output or "", self.secrets)
            return process.returncode, _sanitize_text(output or "", self.secrets)
        finally:
            # view_log records TX (and devices may echo it); atomically redact both.
            _sanitize_log(log, self.secrets)


def _host_problem(
    returncode: int, output: str, log: Path, command: Optional[str] = None,
) -> Optional[str]:
    log_text = log.read_text(encoding="utf-8", errors="replace") if log.exists() else ""
    text = output + "\n" + log_text
    if command is not None:
        # A gap reported before this command's first TX only says that nobody
        # was collecting the preceding serial bytes. Keep connection failures
        # strict, and inspect gap/overflow only from the latest TX onward.
        tx_matches = list(re.finditer(r"(?m)^\[[^\]]+\]\s+TX\s*\|", text))
        gap_text = text[tx_matches[-1].start():] if tx_matches else ""
        connection = re.search(r"CONN_LOST|connection (?:failed|error)", text, re.IGNORECASE)
        gap = re.search(r"PHYS GAP|BUFFER OVERFLOW", gap_text, re.IGNORECASE)
        match = connection or gap
    else:
        match = re.search(
            r"PHYS GAP|BUFFER OVERFLOW|CONN_LOST|connection (?:failed|error)",
            text, re.IGNORECASE,
        )
    if match:
        return match.group(0).upper()
    if returncode in (1, 2, 4, 130) or returncode < 0:
        return "view_log failed with exit code {}".format(returncode)
    return None


def _case_available(case: Dict[str, Any], config: Dict[str, Any]) -> Tuple[bool, str]:
    if not case.get("enabled", True):
        return False, "SKIPPED_DISABLED"
    for requirement in case.get("requires", []):
        section = config.get(requirement, {})
        if not isinstance(section, dict) or not section.get("enabled", False):
            return False, "SKIPPED_DISABLED"
        if requirement in ("flash", "qspi"):
            address = str(section.get("address", "0")).strip()
            try:
                nonzero = int(address, 0) != 0
            except ValueError:
                nonzero = False
            if not section.get("confirmed_safe", False) or not nonzero:
                return False, "SKIPPED_UNSAFE"
    return True, ""


def _selection(cases: List[Dict[str, Any]], config: Dict[str, Any], rng: random.Random) -> List[Dict[str, Any]]:
    run = config["run"]
    selected: List[Dict[str, Any]] = []
    mandatory = [case for case in cases if case.get("mandatory", False)]
    for _ in range(run["fixed_repeats"]):
        selected.extend(mandatory)
    pool = [case for case in cases if _case_available(case, config)[0]]
    if pool:
        selected.extend(rng.choices(pool, weights=[case.get("weight", 1) for case in pool], k=run["random_cases"]))
    # Record every unavailable case once without consuming a random execution
    # slot. This makes safety skips auditable while preserving stress coverage.
    selected.extend(
        case for case in cases
        if case.get("enabled", True) and not _case_available(case, config)[0]
    )
    return selected


def _base_record(sequence: int, case: Dict[str, Any], log: Path) -> Dict[str, Any]:
    return {
        "sequence": sequence, "case_id": case["id"],
        "target": None, "core": None, "mode": case["mode"],
        "expected_target": case["target"],
        "expected_core": case.get("core", 0), "expected_mode": case["mode"],
        "requested_target": None, "requested_core": None,
        "requested_mode": None, "actual_target": None, "actual_core": None,
        "actual_fault_reason": None, "target_match": None,
        "mode_reason_match": None, "trigger_reject_reason": None,
        "scenario": case.get("scenario"),
        "expected_winner": case.get("expected_winner"),
        "actual_winner": None, "winner_match": None,
        "follower_silent": None, "status_pass": None,
        "actual_follower": None, "actual_dumper": None,
        "heartbeat_timeout_seen": False, "heartbeat_takeover_pass": None,
        "complete_dumps": 0,
        "load": case.get("load", "base"),
        "result": "", "dump_pass": False, "boot_pass": False, "recovery_pass": False,
        "dump_present": False,
        "crc_lines": 0, "crc_failures": 0, "regions": 0, "decoded_bytes": 0,
        "start_time": "", "end_time": "", "duration_s": 0.0,
        "log": log.name, "build_version": "", "errors": [], "warnings": [],
    }


def _apply_analysis(record: Dict[str, Any], analysis: AnalysisResult) -> None:
    for key in (
        "result", "dump_pass", "boot_pass", "recovery_pass", "dump_present",
        "target", "core", "mode", "expected_target", "expected_core",
        "expected_mode", "requested_target", "requested_core", "requested_mode",
        "actual_target", "actual_core", "actual_fault_reason", "target_match",
        "mode_reason_match", "trigger_reject_reason", "crc_lines",
        "crc_failures", "regions", "decoded_bytes", "build_version", "errors",
        "warnings",
        "scenario", "actual_dumper", "heartbeat_timeout_seen",
        "heartbeat_takeover_pass", "complete_dumps",
    ):
        record[key] = getattr(analysis, key)


def _evaluate_case_markers(
    record: Dict[str, Any], case: Dict[str, Any], log: Path,
) -> None:
    if not log.exists():
        return
    text = log.read_text(encoding="utf-8", errors="replace")
    statuses = list(STATUS_RE.finditer(text))
    owners = list(OWNER_RE.finditer(text))
    status = statuses[-1] if statuses else None
    owner = owners[-1] if owners else None
    actual_winner = int(owner.group(1)) if owner else (
        int(status.group(4)) if status else None
    )
    record["actual_winner"] = actual_winner
    if status and int(status.group(1)) == 1:
        record["actual_follower"] = int(status.group(5))
    elif owner and int(owner.group(2)) == 1:
        expected_follower = case.get("expected_follower")
        if expected_follower is not None:
            record["actual_follower"] = expected_follower
        elif case.get("scenario") == "ap_race" and actual_winner in (1, 2):
            record["actual_follower"] = 2 if actual_winner == 1 else 1
    expected = case.get("expected_winner")
    if expected is not None:
        expected_values = expected if isinstance(expected, list) else [expected]
        record["winner_match"] = actual_winner in expected_values
    if not case.get("status_command"):
        return
    expected_scenario = case.get("status_scenario")
    expected_phase = case.get("expected_phase", 3)
    expected_follower = case.get("expected_follower")
    retained_status_pass = bool(
        status
        and int(status.group(1)) == 1
        and int(status.group(2)) == expected_scenario
        and int(status.group(3)) == expected_phase
    )
    if retained_status_pass and expected_follower is not None:
        retained_status_pass = int(status.group(5)) == expected_follower
    if retained_status_pass and case.get("scenario") == "ap_race":
        retained_status_pass = (
            actual_winner in (1, 2)
            and int(status.group(5)) == (2 if actual_winner == 1 else 1)
        )
    ap_peer_quiesce = case.get("scenario") in (
        "ap0_ap1", "ap1_ap0", "ap_race",
    )
    owner_pass = bool(
        owner
        and int(owner.group(1)) == actual_winner
        and (int(owner.group(2)) == 1 or ap_peer_quiesce)
    )
    # Whole-chip reset clears the shared SRAM status word on this platform.
    # For AP peer races, the owner marker is emitted only after both race tasks
    # crossed the ready barrier; the winning core may then reset-hold its peer
    # before that peer can publish WAIT. If retained state is available on
    # another reset path, require it to agree as an extra check.
    status_pass = owner_pass and (
        not status or int(status.group(1)) == 0 or retained_status_pass
    )
    armed = (
        "DUMP_ORCH_ARMED" in text
        or (ap_peer_quiesce and owner_pass)
    )
    record["status_pass"] = status_pass
    record["follower_silent"] = bool(
        armed and owner and status_pass
        and record.get("complete_dumps") == 1
    )
    if record["winner_match"] is False:
        record["warnings"].append("orchestration winner did not match expectation")
    if not status_pass:
        record["warnings"].append("owner/follower WAIT handshake was not observed")
    elif ap_peer_quiesce and owner and int(owner.group(2)) == 0:
        record["warnings"].append(
            "peer AP was quiesced before publishing follower WAIT"
        )
    if not record["follower_silent"]:
        record["warnings"].append("follower silence was not proven")


def _wait_dump_cli_ready(
    target: str, runner: ViewLog, log: Path, timeout_s: float,
) -> Optional[str]:
    case_id = "READY_PROBE"
    if target == "AP":
        command = "ap_cmd ap_dump_test {} invalid 0".format(case_id)
    else:
        command = "cp_dump_test {} invalid".format(case_id)
    ready = "DUMP_TEST_REJECT case_id={} reason=invalid_mode".format(case_id)
    expect = re.escape(ready) + r"|cmd NOT found"
    deadline = time.monotonic() + timeout_s

    while not STOP_REQUESTED and time.monotonic() < deadline:
        remaining = max(1.0, deadline - time.monotonic())
        rc, output = runner.call(
            log, command=command, expect=expect, timeout=min(10.0, remaining),
        )
        host_error = _host_problem(rc, output, log, command=command)
        if host_error:
            return host_error
        if ready in output:
            return None
        time.sleep(min(2.0, max(0.0, deadline - time.monotonic())))

    return "{} dump CLI did not become ready within {}s".format(
        target, timeout_s,
    )


def _effective_command_expect(
    command: str, expect: Optional[Any],
) -> Optional[str]:
    if expect is None:
        return None
    pattern = str(expect)
    if command.lstrip().startswith("ping "):
        # The load is active once the firmware ping worker reports its
        # parameters. Some APs intentionally drop ICMP echo replies, so a
        # positive reply is not a valid prerequisite for dump stress.
        return "(?:{})|ping:\\s*size:".format(pattern)
    return pattern


def execute_case(
    sequence: int, case: Dict[str, Any], config: Dict[str, Any], runner: ViewLog,
    run_dir: Path, rng: random.Random, manifest: Optional[Dict[str, Any]], dry_run: bool,
) -> Dict[str, Any]:
    log = _safe_child(run_dir, "{:04d}_{}.log".format(sequence, case["id"]))
    record = _base_record(sequence, case, log)
    started = time.monotonic()
    record["start_time"] = dt.datetime.now().astimezone().isoformat()
    # Firmware stores at most 31 bytes. The host report retains the descriptive
    # case ID, while this compact token uniquely correlates device metadata.
    values = _variables(config, "D{:06d}".format(sequence), rng)
    run = config["run"]
    host_error: Optional[str] = None
    try:
        available, skip_result = _case_available(case, config)
        if not available:
            record["result"] = skip_result
            record["errors"] = ["requirements not enabled or safety gate not satisfied"]
            return record
        if dry_run:
            record["result"] = "DRY_RUN"
            return record
        if not case.get("skip_dump_cli_ready", False):
            ready_log = _safe_child(run_dir, "_cli_ready.log")
            host_error = _wait_dump_cli_ready(
                str(case["target"]), runner, ready_log,
                float(run["boot_timeout_s"]),
            )
            if host_error:
                record["result"] = "HOST_ERROR"
                record["errors"] = [host_error]
                return record
        # PREPARE -> CONFIRM_PRECONDITION
        prepare_waits = case.get("prepare_wait_s", [])
        prepare_expects = case.get("prepare_expect", [])
        for index, template in enumerate(case.get("prepare", [])):
            command = _expand(template, values)
            wait = float(prepare_waits[index]) if index < len(prepare_waits) else 1.0
            expect_value = prepare_expects[index] if index < len(prepare_expects) else None
            expect = _effective_command_expect(command, expect_value)
            rc, output = runner.call(log, duration=wait, command=command, expect=expect, timeout=run["prepare_timeout_s"])
            host_error = _host_problem(rc, output, log, command=command)
            if host_error:
                break
            if rc == 3:
                record["result"] = "PRECONDITION_FAILED"
                record["errors"] = ["prepare expectation timed out"]
                return record
        if host_error:
            record["result"] = "HOST_ERROR"
            record["errors"] = [host_error]
            return record

        # RANDOM_DELAY -> TRIGGER -> CAPTURE_DUMP
        delay_min = float(case.get("random_delay_min_s", run["random_delay_min_s"]))
        delay_max = float(case.get("random_delay_max_s", run["random_delay_max_s"]))
        if delay_min < 0 or delay_max < delay_min:
            raise ValueError("{} has invalid random delay range".format(case["id"]))
        delay = rng.uniform(delay_min, delay_max)
        if delay:
            time.sleep(delay)
        trigger = _expand(case["trigger"], values)
        rc, output = runner.call(
            log, command=trigger,
            expect=_dump_completion_expect(case, run),
            timeout=float(case.get("dump_timeout_s", run["dump_timeout_s"])),
        )
        host_error = _host_problem(rc, output, log, command=trigger)
        if host_error:
            record["result"] = "HOST_ERROR"
            record["errors"] = [host_error]
            return record

        # WAIT_BOOT. A dump timeout is not immediately a host failure; keep listening.
        boot_expect = str(case.get(
            "boot_expect",
            run.get("boot_expect", r"user app entry|AP main running|armino app init"),
        ))
        boot_rc, boot_output = runner.call(log, expect=boot_expect, timeout=run["boot_timeout_s"])
        host_error = _host_problem(boot_rc, boot_output, log)
        if host_error:
            record["result"] = "HOST_ERROR"
            record["errors"] = [host_error]
            return record

        if case.get("status_command"):
            status_command = _expand(str(case["status_command"]), values)
            status_rc, status_output = runner.call(
                log, command=status_command,
                expect=r"DUMP_ORCH_STATUS\s+raw=",
                timeout=run["recovery_timeout_s"],
            )
            host_error = _host_problem(
                status_rc, status_output, log, command=status_command,
            )
            if host_error:
                record["result"] = "HOST_ERROR"
                record["errors"] = [host_error]
                return record

        # RECOVER -> VERIFY_RECOVERY
        recovery_waits = case.get("recovery_wait_s", [])
        recovery_expects = case.get("recovery_expect", [])
        for index, template in enumerate(case.get("recovery", [])):
            command = _expand(template, values)
            wait = float(recovery_waits[index]) if index < len(recovery_waits) else 1.0
            expect_value = recovery_expects[index] if index < len(recovery_expects) else None
            expect = _effective_command_expect(command, expect_value)
            rc, output = runner.call(log, duration=wait, command=command, expect=expect, timeout=run["recovery_timeout_s"])
            host_error = _host_problem(rc, output, log, command=command)
            if host_error:
                record["result"] = "HOST_ERROR"
                record["errors"] = [host_error]
                return record

        analysis = analyze_file(
            log, expected_target=case["target"], expected_core=case.get("core", 0),
            expected_mode=case["mode"],
            boot_expect=[boot_expect],
            recovery_expect=[str(item) for item in recovery_expects if item] if any(recovery_expects) else [boot_expect],
            golden_manifest=manifest, power_state=str(case.get("power_state", "default")),
            scenario=case.get("scenario"),
        )
        _apply_analysis(record, analysis)
        _evaluate_case_markers(record, case, log)
        return record
    finally:
        record["end_time"] = dt.datetime.now().astimezone().isoformat()
        record["duration_s"] = round(time.monotonic() - started, 3)


def _summary(records: Sequence[Dict[str, Any]]) -> Dict[str, Any]:
    summary: Dict[str, Any] = {
        "total": len(records),
        "dump_pass": sum(
            item.get("dump_pass") is True and item.get("result") == "PASS"
            for item in records
        ),
        "dump_fail": sum(
            item.get("dump_present") is True
            and item.get("dump_pass") is not True
            and item.get("result") != "HOST_ERROR"
            for item in records
        ),
        "not_triggered": sum(
            item.get("result") == "NOT_TRIGGERED" for item in records
        ),
        "host_error": sum(item["result"] == "HOST_ERROR" for item in records),
        "skipped": sum(item["result"] in SKIPPED for item in records),
        "target_mismatch": sum(
            item.get("target_match") is False for item in records
        ),
        "recovery_fail": sum(
            item.get("dump_present") is True
            and item.get("recovery_pass") is False
            and item.get("target_match") is not False
            for item in records
        ),
        "winner_mismatch": sum(
            item.get("winner_match") is False for item in records
        ),
        "status_fail": sum(
            item.get("status_pass") is False for item in records
        ),
        "follower_silent_fail": sum(
            item.get("follower_silent") is False for item in records
        ),
        "heartbeat_takeovers": sum(
            item.get("heartbeat_takeover_pass") is not None for item in records
        ),
        "heartbeat_takeover_pass": sum(
            item.get("heartbeat_takeover_pass") is True for item in records
        ),
        "heartbeat_takeover_fail": sum(
            item.get("heartbeat_takeover_pass") is False for item in records
        ),
    }
    summary["dump_eligible"] = summary["dump_pass"] + summary["dump_fail"]
    summary["dump_pass_rate"] = (
        round(100.0 * summary["dump_pass"] / summary["dump_eligible"], 2)
        if summary["dump_eligible"] else None
    )
    return summary


def _trigger_pattern(template: str) -> re.Pattern:
    parts: List[str] = []
    position = 0
    for match in VARIABLE_RE.finditer(template):
        parts.append(re.escape(template[position:match.start()]))
        parts.append(r"\S+")
        position = match.end()
    parts.append(re.escape(template[position:]))
    return re.compile(r"^\s*{}\s*$".format("".join(parts)))


def _log_time_bounds(
    path: Path, run_started: str, analysis: AnalysisResult,
    trigger: str, boot_expect: str,
) -> Tuple[str, str, float]:
    timestamp_re = re.compile(r"^\[(\d{2}):(\d{2}):(\d{2})\.(\d{3,6})\]")
    direction_re = re.compile(r"^\[[^\]]+\]\s+(RX|TX|SYS)\s*\|\s?(.*)$")
    entries: List[Tuple[int, dt.time, Optional[str], str]] = []
    for number, line in enumerate(
        path.read_text(encoding="utf-8", errors="replace").splitlines(), 1,
    ):
        match = timestamp_re.match(line)
        if match:
            hour, minute, second, fraction = match.groups()
            direction_match = direction_re.match(line)
            entries.append((number, dt.time(
                int(hour), int(minute), int(second),
                int(fraction.ljust(6, "0")),
            ), direction_match.group(1) if direction_match else None,
                direction_match.group(2) if direction_match else line))
    if not entries:
        return "", "", 0.0
    trigger_re = _trigger_pattern(trigger)
    start_entry = next(
        (entry for entry in entries
         if entry[2] == "TX" and trigger_re.match(entry[3])),
        None,
    )
    if start_entry is None:
        start_line = analysis.session_start_line or entries[0][0]
        start_entry = next(
            (entry for entry in entries if entry[0] >= start_line),
            entries[0],
        )
    end_line = analysis.session_end_line or 0
    end_entry = next(
        (entry for entry in entries
         if entry[0] > end_line
         and entry[2] in ("RX", None)
         and re.search(boot_expect, entry[3], re.IGNORECASE)),
        None,
    )
    if end_entry is None:
        end_entry = next(
            (entry for entry in entries if entry[0] == end_line),
            entries[-1],
        )
    try:
        run_dt = dt.datetime.fromisoformat(run_started)
        day = run_dt.date()
        timezone = run_dt.tzinfo
    except (TypeError, ValueError):
        day = dt.date.today()
        timezone = None
        run_dt = dt.datetime.combine(day, dt.time.min, timezone)
    start = dt.datetime.combine(day, start_entry[1], timezone)
    if start < run_dt - dt.timedelta(hours=12):
        start += dt.timedelta(days=1)
    end = dt.datetime.combine(start.date(), end_entry[1], timezone)
    if end < start:
        end += dt.timedelta(days=1)
    return start.isoformat(), end.isoformat(), round((end - start).total_seconds(), 3)


def reanalyze_report(
    run_dir: Path, report: Dict[str, Any], cases: Sequence[Dict[str, Any]],
    config: Dict[str, Any], manifest: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    definitions = {case["id"]: case for case in cases}
    default_boot = str(config["run"].get(
        "boot_expect", r"user app entry|AP main running|armino app init",
    ))
    for record in report.get("cases", []):
        case = definitions.get(record.get("case_id"))
        if case is None:
            record["result"] = "HOST_ERROR"
            record["errors"] = ["case definition not found"]
            continue
        record.update({
            "expected_target": case["target"],
            "expected_core": case.get("core", 0),
            "expected_mode": case["mode"],
            "scenario": case.get("scenario"),
            "expected_winner": case.get("expected_winner"),
        })
        if record.get("result") in SKIPPED:
            record.update({
                "target": None, "core": None, "actual_target": None,
                "actual_core": None, "actual_fault_reason": None,
                "requested_target": None, "requested_core": None,
                "requested_mode": None, "target_match": None,
                "mode_reason_match": None, "trigger_reject_reason": None,
                "dump_present": False, "warnings": [],
            })
            continue
        log = _safe_child(run_dir, str(record.get("log", "")))
        boot_expect = str(case.get("boot_expect", default_boot))
        recovery_values = [
            str(item) for item in case.get("recovery_expect", []) if item
        ]
        analysis = analyze_file(
            log, expected_target=case["target"],
            expected_core=case.get("core", 0), expected_mode=case["mode"],
            boot_expect=[boot_expect],
            recovery_expect=recovery_values or [boot_expect],
            golden_manifest=manifest,
            power_state=str(case.get("power_state", "default")),
            scenario=case.get("scenario"),
        )
        start_time, end_time, duration = _log_time_bounds(
            log, str(report.get("started_at", "")), analysis,
            str(case["trigger"]), boot_expect,
        )
        record["start_time"] = start_time
        record["end_time"] = end_time
        if start_time and end_time:
            record["duration_s"] = duration
        _apply_analysis(record, analysis)
        _evaluate_case_markers(record, case, log)
    report["summary"] = _summary(report.get("cases", []))
    report["reanalyzed_at"] = dt.datetime.now().astimezone().isoformat()
    return report


def _html_report(report: Dict[str, Any]) -> str:
    esc = lambda value: html.escape(str(value), quote=True)
    summary = report["summary"]
    rows: List[str] = []
    details: List[str] = []
    for item in report["cases"]:
        result = str(item.get("result", ""))
        state = (
            "pass" if item.get("dump_pass") is True
            else ("fail" if item.get("dump_present") is True else "na")
        )
        sequence = item.get("sequence", "")
        detail_id = "detail-{}".format(sequence)
        errors = item.get("errors") or []
        warnings = item.get("warnings") or []
        messages = list(errors) + list(warnings)
        detail = "; ".join(str(message) for message in messages) or (
            "CRC lines: {}, failures: {}; regions: {}; decoded bytes: {}".format(
                item.get("crc_lines", 0), item.get("crc_failures", 0),
                item.get("regions", 0), item.get("decoded_bytes", 0),
            )
        )
        log_name = str(item.get("log", ""))
        detail_cell = esc(detail)
        if messages:
            detail_cell = '<a href="#{}">{}</a>'.format(
                esc(detail_id), esc(detail),
            )
            details.append(
                '<section class="{}" id="{}"><h3>{}: {}</h3>'
                '<p>{}</p><p><a href="{}">Open log</a></p></section>'.format(
                    "failure" if errors else "warning", esc(detail_id),
                    esc(item.get("case_id", "")), esc(result), esc(detail),
                    esc(log_name),
                )
            )
        expected = "{}{}".format(
            item.get("expected_target") or "",
            item.get("expected_core")
            if item.get("expected_core") is not None else "",
        )
        actual = "{}{}".format(
            item.get("actual_target") or "",
            item.get("actual_core")
            if item.get("actual_core") is not None else "",
        )
        target_match = item.get("target_match")
        target_text = (
            "YES" if target_match is True
            else ("NO" if target_match is False else "N/A")
        )
        marker_text = lambda value: (
            "YES" if value is True else ("NO" if value is False else "N/A")
        )
        rows.append(
            "<tr><td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td>"
            "<td>{}</td><td><span class=\"badge {}\">{}</span></td><td>{}</td>"
            "<td>{}</td><td>{}</td><td>{}</td><td>{}</td><td>{}</td>"
            "<td>{}</td><td>{}</td><td>{}/{}</td><td>{}</td>"
            "<td>{:.3f}s</td><td>{}</td>"
            "<td><a href=\"{}\">log</a></td></tr>".format(
                esc(sequence), esc(item.get("case_id", "")),
                esc(item.get("scenario") or ""), esc(expected), esc(actual),
                esc(item.get("actual_dumper") or ""), state, esc(result),
                esc(target_text),
                esc(item.get("expected_winner")
                    if item.get("expected_winner") is not None else ""),
                esc(item.get("actual_winner")
                    if item.get("actual_winner") is not None else ""),
                esc(marker_text(item.get("winner_match"))),
                esc(marker_text(item.get("follower_silent"))),
                esc(marker_text(item.get("status_pass"))),
                esc(item.get("boot_pass", False)),
                esc(item.get("recovery_pass", False)),
                esc(item.get("crc_failures", 0)), esc(item.get("crc_lines", 0)),
                esc(item.get("actual_fault_reason") or ""),
                float(item.get("duration_s", 0.0) or 0.0), detail_cell,
                esc(log_name),
            )
        )
    rate = (
        "{:.2f}%".format(summary["dump_pass_rate"])
        if summary["dump_pass_rate"] is not None else "N/A"
    )
    build = next(
        (item.get("build_version") for item in report["cases"]
         if item.get("build_version")), "unknown",
    )
    return """<!doctype html>
<html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>BK7259 Dump Test Report</title>
<style>
:root{{font-family:Arial,sans-serif;color:#263238;background:#f4f7fb}}body{{margin:0;padding:24px}}.wrap{{max-width:1500px;margin:auto}}
h1{{color:#1976d2;letter-spacing:.08em}}.cards{{display:flex;gap:12px;flex-wrap:wrap;margin:18px 0}}.card{{background:#fff;border-radius:8px;padding:14px 22px;box-shadow:0 1px 5px #ccd6e0}}.pass{{color:#16833b}}.fail{{color:#c62828}}.na{{color:#ef6c00}}
.badge{{font-weight:700}}.meta{{color:#52616b}}.table-wrap{{overflow-x:auto;background:#fff}}table{{border-collapse:collapse;width:100%;font-size:13px}}th{{background:#1976d2;color:#fff;text-align:left}}th,td{{padding:9px;border:1px solid #dce4ea;vertical-align:top}}tr:nth-child(even){{background:#f8fafc}}a{{color:#1565c0;overflow-wrap:anywhere}}.failure,.warning{{background:#fff;padding:8px 16px;margin:12px 0}}.failure{{border-left:4px solid #c62828}}.warning{{border-left:4px solid #ef6c00}}
@media(max-width:700px){{body{{padding:10px}}.cards{{display:grid;grid-template-columns:1fr 1fr}}th,td{{padding:6px}}}}@media print{{body{{background:#fff;padding:0}}.card,.table-wrap{{box-shadow:none}}a{{color:inherit}}.failure{{break-inside:avoid}}}}
</style></head><body><main class="wrap">
<h1>TEST REPORT</h1>
<p class="meta">Run {run} · Build {build} · Started {started} · Finished {finished}</p>
<div class="cards"><div class="card pass"><b>DUMP PASS</b><br>{passed}</div><div class="card fail"><b>DUMP FAIL</b><br>{failed}</div><div class="card na"><b>NOT TRIGGERED</b><br>{not_triggered}</div><div class="card na"><b>SKIPPED</b><br>{skipped}</div><div class="card"><b>Dump pass rate</b><br>{rate}</div></div>
<p>Target mismatch: <b>{target_mismatch}</b> · Winner mismatch: <b>{winner_mismatch}</b> · Status fail: <b>{status_fail}</b> · Silent follower fail: <b>{follower_fail}</b> · Recovery fail: <b>{recovery_fail}</b> · Host error: <b>{host_error}</b></p>
<p>Heartbeat takeover pass/total: <b>{heartbeat_pass}/{heartbeat_total}</b></p>
<p>CRC lines: <b>{crc}</b> · CRC failures: <b>{crc_fail}</b> · Regions: <b>{regions}</b> · Decoded bytes: <b>{decoded}</b></p>
<div class="table-wrap"><table><thead><tr><th>INDEX</th><th>CASE_NAME</th><th>SCENARIO</th><th>EXPECTED TARGET</th><th>ACTUAL TARGET</th><th>ACTUAL DUMPER</th><th>DUMP RESULT</th><th>TARGET MATCH</th><th>EXPECTED WINNER</th><th>ACTUAL WINNER</th><th>WINNER MATCH</th><th>FOLLOWER SILENT</th><th>STATUS PASS</th><th>BOOT</th><th>RECOVERY</th><th>CRC FAIL/TOTAL</th><th>FAULT REASON</th><th>DURATION</th><th>DETAIL</th><th>LOG</th></tr></thead><tbody>
{rows}</tbody></table></div>{details}</main></body></html>
""".format(
        run=esc(report.get("run_id", "")), build=esc(build),
        started=esc(report.get("started_at", "")),
        finished=esc(report.get("finished_at", "partial")),
        passed=summary["dump_pass"], failed=summary["dump_fail"],
        not_triggered=summary["not_triggered"], skipped=summary["skipped"],
        rate=esc(rate),
        target_mismatch=summary["target_mismatch"],
        winner_mismatch=summary.get("winner_mismatch", 0),
        status_fail=summary.get("status_fail", 0),
        follower_fail=summary.get("follower_silent_fail", 0),
        heartbeat_pass=summary.get("heartbeat_takeover_pass", 0),
        heartbeat_total=summary.get("heartbeat_takeovers", 0),
        recovery_fail=summary["recovery_fail"],
        host_error=summary["host_error"],
        crc=sum(item.get("crc_lines", 0) for item in report["cases"]),
        crc_fail=sum(item.get("crc_failures", 0) for item in report["cases"]),
        regions=sum(item.get("regions", 0) for item in report["cases"]),
        decoded=sum(item.get("decoded_bytes", 0) for item in report["cases"]),
        rows="\n".join(rows), details="\n".join(details),
    )


def write_reports(run_dir: Path, report: Dict[str, Any]) -> None:
    (run_dir / "result.json").write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    fields = (
        "run_id", "sequence", "case_id", "expected_target", "expected_core",
        "expected_mode", "requested_target", "requested_core", "requested_mode",
        "actual_target", "actual_core", "actual_fault_reason", "target_match",
        "mode_reason_match", "trigger_reject_reason", "load", "result",
        "scenario", "expected_winner", "actual_winner", "winner_match",
        "actual_follower", "follower_silent", "status_pass", "actual_dumper",
        "heartbeat_timeout_seen", "heartbeat_takeover_pass", "complete_dumps",
        "dump_present", "dump_pass", "boot_pass", "recovery_pass", "crc_lines",
        "crc_failures", "regions", "decoded_bytes", "warnings", "errors",
        "start_time", "end_time", "duration_s", "log",
    )
    with (run_dir / "result.csv").open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        for record in report["cases"]:
            writer.writerow(dict(record, run_id=report["run_id"]))
    summary = report["summary"]
    lines = [
        "# BK7259 Dump Test Report",
        "",
        "- Run: `{}`".format(report["run_id"]),
        "- Seed: `{}`".format(report["seed"]),
        "- Started: {}".format(report["started_at"]),
        "- Finished: {}".format(report.get("finished_at", "partial")),
        "- Build: `{}`".format(next((item.get("build_version") for item in report["cases"] if item.get("build_version")), "unknown")),
        "- Total/Dump pass/Dump fail/Not triggered/Host error/Skipped: {}/{}/{}/{}/{}/{}".format(
            summary["total"], summary["dump_pass"], summary["dump_fail"],
            summary["not_triggered"], summary["host_error"], summary["skipped"],
        ),
        "- Dump pass rate: {}".format(
            "{:.2f}% ({}/{})".format(
                summary["dump_pass_rate"], summary["dump_pass"],
                summary["dump_eligible"],
            ) if summary["dump_pass_rate"] is not None else "N/A"
        ),
        "- Target mismatch: {}; Recovery fail: {}".format(
            summary["target_mismatch"], summary["recovery_fail"],
        ),
        "- Winner mismatch: {}; Status fail: {}; Silent follower fail: {}".format(
            summary.get("winner_mismatch", 0), summary.get("status_fail", 0),
            summary.get("follower_silent_fail", 0),
        ),
        "- Heartbeat takeover pass/total: {}/{}".format(
            summary.get("heartbeat_takeover_pass", 0),
            summary.get("heartbeat_takeovers", 0),
        ),
        "",
        "## Dump integrity failures",
    ]
    failures = [
        item for item in report["cases"]
        if item.get("dump_present") and not item.get("dump_pass")
    ]
    if failures:
        for item in failures:
            reason = item.get("errors", ["unknown"])[0] if item.get("errors") else "unknown"
            lines.append("- `{}`: **{}** — {}; log `{}`".format(item["case_id"], item["result"], reason, item["log"]))
    else:
        lines.append("- None")
    lines.extend(("", "## Non-integrity issues"))
    non_integrity = [
        item for item in report["cases"]
        if item.get("result") in SKIPPED | {"NOT_TRIGGERED", "HOST_ERROR"}
        or item.get("warnings")
    ]
    if non_integrity:
        for item in non_integrity:
            messages = item.get("warnings") or item.get("errors") or ["unknown"]
            lines.append(
                "- `{}`: **{}** — {}; log `{}`".format(
                    item["case_id"], item["result"], "; ".join(messages),
                    item["log"],
                )
            )
    else:
        lines.append("- None")
    lines.extend(("", "## Breakdown"))
    for field in ("target_core", "mode", "load", "scenario"):
        values: Dict[str, List[Dict[str, Any]]] = {}
        for item in report["cases"]:
            name = (
                "{}{}".format(
                    item.get("expected_target", ""),
                    item.get("expected_core", 0),
                )
                if field == "target_core" else str(item.get(field, ""))
            )
            values.setdefault(name, []).append(item)
        lines.append("- {}: {}".format(field, ", ".join(
            "{}={}/{} dump pass (not-triggered={}, host={}, skipped={})".format(
                name,
                sum(item.get("dump_pass") is True for item in items),
                sum(item.get("dump_present") is True for item in items),
                sum(item["result"] == "NOT_TRIGGERED" for item in items),
                sum(item["result"] == "HOST_ERROR" for item in items),
                sum(item["result"] in SKIPPED for item in items),
            )
            for name, items in sorted(values.items())
        )))
    (run_dir / "report.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    (run_dir / "report.html").write_text(_html_report(report), encoding="utf-8")


def _parser() -> argparse.ArgumentParser:
    here = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, default=here / "config.json")
    parser.add_argument("--cases", type=Path, default=here / "cases.json")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--analyze-log", type=Path)
    parser.add_argument("--reanalyze-run", type=Path)
    parser.add_argument("--target", choices=("AP", "CP"))
    parser.add_argument("--core", type=int, choices=(0, 1))
    parser.add_argument("--mode")
    parser.add_argument("--case-id", action="append", default=[],
                        help="run only the named case; may be repeated")
    parser.add_argument("--all-cases", action="store_true",
                        help="run every enabled case once, including auditable skips")
    parser.add_argument(
        "--continue-on-failure", action="store_true",
        help="record an unrecovered case and continue with the remaining cases",
    )
    parser.add_argument("--golden", type=Path)
    parser.add_argument("--boot-expect", action="append", default=[])
    parser.add_argument("--recovery-expect", action="append", default=[])
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = _parser().parse_args(argv)
    if args.analyze_log:
        result = analyze_file(
            args.analyze_log, expected_target=args.target, expected_core=args.core,
            expected_mode=args.mode,
            boot_expect=args.boot_expect, recovery_expect=args.recovery_expect,
            golden_manifest=load_manifest(args.golden),
        )
        print(json.dumps(result.to_dict(), indent=2, ensure_ascii=False))
        return 0 if result.result == "PASS" else 1
    try:
        config = _load_json(args.config.resolve())
        cases_value = _load_json(args.cases.resolve())
        cases = cases_value["cases"] if isinstance(cases_value, dict) else cases_value
        if not isinstance(config, dict) or not isinstance(cases, list):
            raise ValueError("invalid config/cases document")
        validate_config(config, cases)
        manifest = load_manifest(args.golden)
        if args.reanalyze_run:
            run_dir = args.reanalyze_run.expanduser().resolve()
            if not run_dir.is_dir():
                raise ValueError("reanalyze run directory does not exist")
            report = _load_json(run_dir / "result.json")
            if not isinstance(report, dict) or not isinstance(report.get("cases"), list):
                raise ValueError("run result.json is invalid")
            reanalyze_report(run_dir, report, cases, config, manifest)
            write_reports(run_dir, report)
            print("Reanalyzed reports: {}".format(run_dir))
            return 0 if (
                report["summary"]["dump_fail"] == 0
                and report["summary"]["not_triggered"] == 0
                and report["summary"]["host_error"] == 0
                and report["summary"]["winner_mismatch"] == 0
                and report["summary"]["status_fail"] == 0
                and report["summary"]["follower_silent_fail"] == 0
            ) else 1
        run = config["run"]
        output_root = Path(run["output_dir"]).expanduser()
        if not output_root.is_absolute():
            output_root = (args.config.resolve().parent / output_root).resolve()
        run_id = dt.datetime.now().astimezone().strftime("%Y%m%d_%H%M%S")
        output_root.mkdir(parents=True, exist_ok=True)
        run_dir = _safe_child(output_root, run_id)
        run_dir.mkdir(mode=0o700)
        flash = config.get("flash", {})
        print("Run {} seed={} output={}".format(run_id, run["random_seed"], run_dir))
        print("Flash region: enabled={} confirmed_safe={} address={} length={}".format(
            bool(flash.get("enabled")), bool(flash.get("confirmed_safe")),
            flash.get("address", "0x0"), flash.get("length", "0x0"),
        ))
        secrets = [str(config.get("network", {}).get("password", ""))]
        rng = random.Random(run["random_seed"])
        if args.all_cases and args.case_id:
            raise ValueError("--all-cases cannot be combined with --case-id")
        if args.all_cases:
            selected = [case for case in cases if case.get("enabled", True)]
        elif args.case_id:
            cases_by_id = {str(case["id"]): case for case in cases}
            missing = [case_id for case_id in args.case_id
                       if case_id not in cases_by_id]
            if missing:
                raise ValueError("unknown case id(s): {}".format(
                    ", ".join(missing)))
            selected = [cases_by_id[case_id] for case_id in args.case_id]
        else:
            selected = _selection(cases, config, rng)
        report: Dict[str, Any] = {
            "run_id": run_id, "seed": run["random_seed"],
            "started_at": dt.datetime.now().astimezone().isoformat(),
            "cases": [], "summary": {},
        }
        runner = ViewLog(config, secrets)
        signal.signal(signal.SIGINT, _stop)
        signal.signal(signal.SIGTERM, _stop)
        try:
            for sequence, case in enumerate(selected, 1):
                if STOP_REQUESTED:
                    break
                print("[{}/{}] {} ({}/{})".format(sequence, len(selected), case["id"], case["target"], case.get("core", 0)))
                record = execute_case(sequence, case, config, runner, run_dir, rng, manifest, args.dry_run)
                report["cases"].append(record)
                report["summary"] = _summary(report["cases"])
                write_reports(run_dir, report)
                readiness_failure = (
                    record.get("result") == "HOST_ERROR"
                    and any(
                        "dump CLI did not become ready" in error
                        for error in record.get("errors", [])
                    )
                )
                unrecovered_not_triggered = (
                    record.get("result") == "NOT_TRIGGERED"
                    and record.get("boot_pass") is not True
                )
                if readiness_failure or unrecovered_not_triggered:
                    reason = (
                        record.get("errors", ["device did not reboot"])[0]
                        if readiness_failure else
                        "{} did not trigger a dump or recover".format(
                            record["case_id"])
                    )
                    if args.continue_on_failure:
                        print("Continue after case failure: {}".format(reason))
                    else:
                        report["aborted_reason"] = reason
                        print("Abort remaining cases: {}".format(reason))
                        break
        finally:
            report["finished_at"] = dt.datetime.now().astimezone().isoformat()
            report["interrupted"] = STOP_REQUESTED
            report["summary"] = _summary(report["cases"])
            write_reports(run_dir, report)
        print("Reports: {}".format(run_dir))
        return 130 if STOP_REQUESTED else (
            0 if args.dry_run or (
                report["summary"]["dump_fail"] == 0
                and report["summary"]["not_triggered"] == 0
                and report["summary"]["host_error"] == 0
                and report["summary"]["winner_mismatch"] == 0
                and report["summary"]["status_fail"] == 0
                and report["summary"]["follower_silent_fail"] == 0
            ) else 1
        )
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
        print("error: {}".format(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
