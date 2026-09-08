#!/usr/bin/env python3
"""Per-case prepare recipes for secureboot_xip OTA tests.

Generates boot_param / mutated images under artifacts/<case_id>/ and prints
board steps. Host cannot drive AON_PMU try; TRIAL rollback needs warm resets.
"""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

HERE = Path(__file__).resolve().parent
BP_TOOL = HERE / "boot_param_tool.py"
ART_ROOT = HERE / "artifacts"

BP_OFFSET_BK7258 = 0x7FC000
OTA_HDR = 32 + 32  # global + img hdr


def _run(cmd: List[str]) -> None:
    print("+", " ".join(cmd))
    subprocess.check_call(cmd)


def _bp_build(out: Path, args: List[str], fresh: bool = True) -> None:
    cmd = [sys.executable, str(BP_TOOL), "--chip", "bk7258", "build",
           "--out", str(out)] + args
    if fresh:
        cmd.insert(cmd.index("build") + 1, "--fresh")
    _run(cmd)


def _find_ota(build_dir: Optional[Path]) -> Optional[Path]:
    if not build_dir:
        return None
    for p in (build_dir / "ota.bin", build_dir / "install" / "ota.bin"):
        if p.exists():
            return p
    return None


def _mutate_corrupt(src: Path, dst: Path, offset: int = OTA_HDR, nbytes: int = 34) -> None:
    data = bytearray(src.read_bytes())
    n = min(nbytes, len(data) - offset)
    if n <= 0:
        raise SystemExit(f"corrupt offset {offset} past EOF ({len(data)})")
    for i in range(n):
        data[offset + i] ^= 0xFF
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(data)
    print(f"[mutate] corrupted {n}B @ {offset} -> {dst}")


def _mutate_truncate(src: Path, dst: Path, keep: int) -> None:
    data = src.read_bytes()[:keep]
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(data)
    print(f"[mutate] truncated to {keep} -> {dst}")


def _write_readme(adir: Path, lines: List[str]) -> None:
    adir.mkdir(parents=True, exist_ok=True)
    (adir / "RUN.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))


# ---- recipes ---------------------------------------------------------------

def prep_P01(adir: Path, build_dir: Optional[Path], **_) -> None:
    if not build_dir:
        raise SystemExit("P-01 needs --build-dir")
    _write_readme(adir, [
        "CASE P-01  pack check (AES=NONE)",
        f"Run: python3 ota_xip_test.py check-pack --build-dir {build_dir} --aes none --record",
        "Expect: ota_crc.bin present, no ota_aes.bin, CONFIG_OTA_ENCRYPTED=0",
    ])


def prep_P02(adir: Path, build_dir: Optional[Path], **_) -> None:
    if not build_dir:
        raise SystemExit("P-02 needs --build-dir (FIXED pack output)")
    _write_readme(adir, [
        "CASE P-02  pack check (AES=FIXED)",
        "Rebuild with security.csv flash_aes_type=FIXED first.",
        f"Run: python3 ota_xip_test.py check-pack --build-dir {build_dir} --aes fixed --record",
    ])


def prep_normal_A(adir: Path, **_) -> None:
    out = adir / "boot_param.bin"
    _bp_build(out, ["--exec-slot", "A", "--state", "normal", "--dl", "idle"])
    _write_readme(adir, [
        "Flash boot_param.bin @ 0x7fc000",
        "Reboot. Expect: decide: NORMAL -> slot 0 (A)",
        f"Artifact: {out}",
    ])


def prep_normal_B(adir: Path, **_) -> None:
    out = adir / "boot_param.bin"
    _bp_build(out, ["--exec-slot", "B", "--update-slot", "B", "--state", "normal"])
    _write_readme(adir, [
        "Pre: slot B must have a valid image (else SoftCRC/unavailable).",
        "Flash boot_param.bin @ 0x7fc000",
        "Reboot. Expect: decide: NORMAL -> slot 1 (B)",
        f"Artifact: {out}",
    ])


def prep_trial_AB(adir: Path, try_max: int = 5, **_) -> None:
    out = adir / "boot_param.bin"
    _bp_build(out, [
        "--exec-slot", "A", "--update-slot", "B",
        "--state", "trial", "--dl", "done",
        "--try-max", str(try_max),
    ])
    _write_readme(adir, [
        f"CASE TRIAL A->B  try_max={try_max}",
        "Pre: BOTH slots have bootable images (B = 'new' under test).",
        "Flash boot_param.bin @ 0x7fc000",
        "NOTE: try lives in AON_PMU — cold power-on clears it; use WARM reset to accumulate.",
        f"T-01: reboot once, AP confirm -> NORMAL exec=B",
        f"T-02: do NOT confirm; warm reset until try>{try_max} -> rollback A",
        f"T-03: at try=={try_max} still prefer B (log decide: TRIAL {try_max}/{try_max})",
        f"T-04: next warm reset try>{try_max} -> rollback",
        f"Artifact: {out}",
    ])


def prep_trial_try_max3(adir: Path, **kwargs) -> None:
    out = adir / "boot_param.bin"
    _bp_build(out, [
        "--exec-slot", "A", "--update-slot", "B",
        "--state", "trial", "--dl", "done", "--try-max", "3",
    ])
    _write_readme(adir, [
        "CASE T-08 custom try_max=3",
        "Flash boot_param.bin @ 0x7fc000; both slots valid images",
        "Warm reset without confirm: try>3 -> rollback A",
        f"Artifact: {out}",
    ])


def prep_virgin(adir: Path, **_) -> None:
    # 8K 0xFF
    img = b"\xff" * 0x2000
    out = adir / "boot_param.bin"
    adir.mkdir(parents=True, exist_ok=True)
    out.write_bytes(img)
    _write_readme(adir, [
        "CASE N-03 virgin boot_param (all 0xFF)",
        "Flash boot_param.bin @ 0x7fc000",
        "Reboot. Expect: decide: virgin -> slot A",
        f"Artifact: {out}",
    ])


def prep_bad_crc_one_sector(adir: Path, **_) -> None:
    # valid sector0 + corrupted sector1 (higher seq but bad CRC)
    good = adir / "boot_param.bin"
    adir.mkdir(parents=True, exist_ok=True)
    _bp_build(good, ["--exec-slot", "A", "--state", "normal", "--seq", "1", "--sector", "0"],
              fresh=True)
    _bp_build(good, ["--exec-slot", "B", "--state", "normal", "--seq", "99", "--sector", "1"],
              fresh=False)
    data = bytearray(good.read_bytes())
    off = 0x1000 + 28  # CRC field of sector1
    data[off] ^= 0xFF
    good.write_bytes(data)
    _write_readme(adir, [
        "CASE N-04 one sector CRC bad",
        "sector0 valid seq=1 exec=A; sector1 seq=99 but CRC broken",
        "Flash @ 0x7fc000. Expect firmware picks sector0 (A), not B",
        f"Artifact: {good}",
    ])


def prep_C01(adir: Path, build_dir: Optional[Path], **_) -> None:
    ota = _find_ota(build_dir)
    if not ota:
        raise SystemExit("C-01 needs --build-dir with ota.bin")
    bad = adir / "ota_bad_header.bin"
    _mutate_corrupt(ota, bad, offset=OTA_HDR, nbytes=34)
    bp = adir / "boot_param.bin"
    # Prefer the slot we will corrupt via OTA write path: arm trial to B
    _bp_build(bp, ["--exec-slot", "A", "--update-slot", "B", "--state", "trial", "--dl", "done"])
    _write_readme(adir, [
        "CASE C-01 bad header must not CBUS hang",
        f"1) Keep good image on slot A; flash {bp.name} @ 0x7fc000 (TRIAL->B)",
        f"2) Download/flash corrupted payload {bad.name} into slot B",
        "   (or burn at secondary_all phy offset if using flash tool)",
        "3) Warm reset. Expect: SoftCRC/EBADIMAGE on B, boot A, no hang",
        f"Artifacts: {bad}, {bp}",
    ])


def prep_N01(adir: Path, build_dir: Optional[Path], **_) -> None:
    ota = _find_ota(build_dir)
    if not ota:
        raise SystemExit("N-01 needs --build-dir with ota.bin")
    cut = adir / "ota_truncated.bin"
    _mutate_truncate(ota, cut, keep=1024)
    _write_readme(adir, [
        "CASE N-01 truncated OTA",
        f"HTTP/CLI flash {cut.name}; expect reject or no set_trial; old slot still boots",
        f"Artifact: {cut}",
    ])


def prep_E03(adir: Path, build_dir: Optional[Path], **_) -> None:
    ota = _find_ota(build_dir)
    if not ota:
        raise SystemExit("E-03 needs --build-dir with ota.bin")
    # Flip one byte deep in payload (after hdr) to break hash
    bad = adir / "ota_tampered.bin"
    data = bytearray(ota.read_bytes())
    off = min(len(data) - 1, OTA_HDR + 4096)
    data[off] ^= 0x01
    adir.mkdir(parents=True, exist_ok=True)
    bad.write_bytes(data)
    bp = adir / "boot_param.bin"
    _bp_build(bp, ["--exec-slot", "A", "--update-slot", "B", "--state", "trial", "--dl", "done"])
    _write_readme(adir, [
        "CASE E-03 hash fail after body tamper",
        f"Flash {bp.name} @ 0x7fc000; put {bad.name} on slot B; reboot",
        "Expect: verify fail on B, fall back / unavailable, boot A",
        f"Artifacts: {bad}, {bp}",
    ])


def prep_H01_manual(adir: Path, **_) -> None:
    bp = adir / "boot_param.bin"
    _bp_build(bp, ["--exec-slot", "A", "--state", "normal"])
    _write_readme(adir, [
        "CASE H-01 A->B OTA + confirm (manual OTA)",
        f"1) Optional baseline: flash {bp.name} @ 0x7fc000 (NORMAL A)",
        "2) On device: http_ota <url of higher-version ota.bin>",
        "3) Reboot -> decide TRIAL B; AP confirm -> NORMAL B",
        "4) Cold reboot still B",
        "Log: trial armed / decide: TRIAL / confirm done",
    ])


def prep_sticky_note(adir: Path, **_) -> None:
    bp = adir / "boot_param.bin"
    _bp_build(bp, ["--exec-slot", "A", "--state", "normal"])
    _write_readme(adir, [
        "CASE S-01/S-02 sticky (semi-manual)",
        "Need: preferred slot header OK but body hangs; other slot good.",
        f"Baseline BP: {bp.name} NORMAL A @ 0x7fc000",
        "Force preferred=bad via BP exec/update + hang image on that slot.",
        "Warm reset until try==5 -> sticky once; try>5 -> clear, no sticky",
        "Log: verified exceed / try>5 cleared, no sticky",
    ])


RECIPES: Dict[str, Any] = {
    "P-01": prep_P01,
    "P-02": prep_P02,
    "P-03": prep_P01,
    "H-01": prep_H01_manual,
    "H-02": prep_normal_B,          # start from B then OTA to A (manual)
    "H-05": prep_normal_A,
    "T-01": prep_trial_AB,
    "T-02": prep_trial_AB,
    "T-03": prep_trial_AB,
    "T-04": prep_trial_AB,
    "T-05": prep_trial_AB,
    "T-08": prep_trial_try_max3,
    "S-01": prep_sticky_note,
    "S-02": prep_sticky_note,
    "S-03": prep_normal_A,          # then corrupt preferred — see C-01 style
    "C-01": prep_C01,
    "C-02": prep_virgin,            # empty other slot + normal — user erases slot
    "E-01": prep_normal_A,
    "E-03": prep_E03,
    "N-01": prep_N01,
    "N-03": prep_virgin,
    "N-04": prep_bad_crc_one_sector,
}

# P0 board set that prepare can materialize
P0_PREPARE_IDS = [
    "P-01", "H-01", "T-01", "T-02", "T-03", "T-04",
    "C-01", "E-01", "S-03", "N-03",
]


def prepare_case(case_id: str, build_dir: Optional[Path] = None) -> Path:
    if not BP_TOOL.exists():
        raise SystemExit(f"missing {BP_TOOL}")
    fn = RECIPES.get(case_id)
    if not fn:
        raise SystemExit(
            f"no prepare recipe for {case_id}. "
            f"Supported: {', '.join(sorted(RECIPES))}"
        )
    adir = ART_ROOT / case_id
    if adir.exists():
        shutil.rmtree(adir)
    adir.mkdir(parents=True)
    print(f"\n==== prepare {case_id} -> {adir} ====", flush=True)
    fn(adir, build_dir=build_dir)
    return adir


def prepare_many(ids: List[str], build_dir: Optional[Path] = None) -> None:
    for cid in ids:
        try:
            prepare_case(cid, build_dir=build_dir)
        except SystemExit as e:
            print(f"[SKIP] {cid}: {e}")
