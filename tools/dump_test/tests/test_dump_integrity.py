import base64
import json
import re
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))

from dump_integrity import analyze_text, hnd_crc8  # noqa: E402
from run_dump_test import (  # noqa: E402
    DUMP_END_EXPECT, _apply_analysis, _base_record, _dump_completion_expect,
    _effective_command_expect, _evaluate_case_markers, _html_report, _summary,
    reanalyze_report, write_reports,
)


def rx(text: str) -> str:
    return "[12:00:00.000] RX | " + text


def encoded(data: bytes) -> str:
    return base64.b64encode(data + bytes((hnd_crc8(data),))).decode("ascii")


def complete_log(
    target: str = "AP",
    core: int = 1,
    mode: str = "isr_crash",
    reason: str = "HardFault",
    legacy_ap_markers: bool = True,
) -> str:
    payload = bytes(range(40))
    lines = [
        rx("DUMP_TEST_BEGIN case_id=TEST_0001 target={} core={} mode={}".format(target, core, mode)),
        rx("@Dump-time(AON-RTC): 123456"),
        rx("@Dump-reason: {}".format(reason)),
        rx("build time => unit-test-build"),
        rx("CPU{} Current regs:".format(core + 2 if target == "AP" else core)),
        rx("user except handler begin"),
    ]
    if target == "AP":
        lines.extend((
            rx("SMP-Core-id: {}".format(core)),
            rx("@Dump-time(AON-RTC): 123999"),
        ))
        if legacy_ap_markers:
            lines.append(rx("AP memory dump begin"))
    lines.extend((
        rx(">>>>stack mem dump begin, region: stack, stack_top=20000000, stack end=20000028"),
        rx(encoded(payload[:32])),
        rx(encoded(payload[32:])),
        rx("<<<<stack mem dump end. region: stack, stack_top=20000040, stack end=20000028"),
    ))
    if target == "AP" and legacy_ap_markers:
        lines.append(rx("AP memory dump end"))
    lines.extend((rx("user except handler end"), rx("BOOT_COMPLETE"), rx("RECOVERY_OK")))
    return "\n".join(lines) + "\n"


def ap_heartbeat_takeover_log() -> str:
    text = complete_log(target="CP", core=0, mode="heartbeat_timeout",
                        reason="Assert")
    cp_payload = bytes(range(8))
    text = text.replace(
        rx("user except handler begin"),
        rx("IPC[2]heartbeat timeout 100,200") + "\n"
        + rx("user except handler begin") + "\n"
        + rx(">>>>stack mem dump begin, region: cp_stack, "
             "stack_top=21000000, stack end=21000008") + "\n"
        + rx(encoded(cp_payload)) + "\n"
        + rx("<<<<stack mem dump end. region: cp_stack, "
             "stack_top=21000040, stack end=21000008") + "\n"
        + rx("AP memory dump begin"),
    )
    return text.replace(
        rx("user except handler end"),
        rx("AP memory dump end") + "\n" + rx("user except handler end"),
    )


def cp_heartbeat_observer_log() -> str:
    text = complete_log(target="CP", core=0, mode="heartbeat_timeout",
                        reason="Assert")
    text = text.replace(
        rx("user except handler begin"),
        rx("observer=AP target=CP reason=cp_heartbeat_timeout "
           "confidence=second_scene") + "\n"
        + rx("user except handler begin") + "\n"
        + rx("CP memory dump begin"),
    )
    return text.replace(
        rx("user except handler end"),
        rx("CP memory dump end"),
    )


def analyze(
    text: str, target: str = "AP", core: int = 1,
    expected_mode: str = None,
):
    return analyze_text(
        text,
        expected_target=target,
        expected_core=core,
        expected_mode=expected_mode,
        boot_expect=[r"BOOT_COMPLETE"],
        recovery_expect=[r"RECOVERY_OK"],
    )


class DumpIntegrityTests(unittest.TestCase):
    def test_runner_accepts_legacy_and_direct_dump_end_markers(self) -> None:
        self.assertIsNotNone(re.search(DUMP_END_EXPECT, "AP memory dump end"))
        self.assertIsNotNone(re.search(
            DUMP_END_EXPECT, "user except handler end",
        ))

    def test_ping_load_accepts_worker_start_without_echo_reply(self) -> None:
        pattern = _effective_command_expect(
            "ping 192.168.50.1 100 1400",
            r"bytes from .*icmp_seq=",
        )
        self.assertIsNotNone(re.search(pattern, "ping: size:1400 times:100"))

    def test_complete_dump_passes(self) -> None:
        result = analyze(complete_log())
        self.assertEqual("PASS", result.result)
        self.assertTrue(result.dump_pass)
        self.assertTrue(result.boot_pass)
        self.assertTrue(result.recovery_pass)
        self.assertEqual(2, result.crc_lines)
        self.assertEqual(40, result.decoded_bytes)

    def test_ap_does_not_require_handler_end(self) -> None:
        text = complete_log().replace(rx("user except handler end") + "\n", "")
        self.assertEqual("PASS", analyze(text).result)

    def test_ap_direct_dump_without_delegation_markers_passes(self) -> None:
        result = analyze(complete_log(legacy_ap_markers=False))
        self.assertEqual("PASS", result.result)
        self.assertTrue(result.dump_pass)
        self.assertEqual("AP", result.actual_target)
        self.assertEqual(1, result.actual_core)

    def test_nested_ap_scope_may_repeat_local_region_addresses(self) -> None:
        payload = bytes(range(40))
        nested = "\n".join((
            rx("AP memory dump begin"),
            rx(">>>>stack mem dump begin, region: stack, "
               "stack_top=20000000, stack end=20000028"),
            rx(encoded(payload[:32])),
            rx(encoded(payload[32:])),
            rx("<<<<stack mem dump end. region: stack, "
               "stack_top=20000040, stack end=20000028"),
            rx("AP memory dump end"),
        ))
        text = complete_log(legacy_ap_markers=False).replace(
            rx("user except handler end"),
            nested + "\n" + rx("user except handler end"),
        )
        result = analyze(text)
        self.assertEqual("PASS", result.result)
        self.assertEqual(4, result.crc_lines)
        self.assertEqual(80, result.decoded_bytes)

    def test_ap_direct_dump_without_handler_end_warns_but_passes(self) -> None:
        text = complete_log(legacy_ap_markers=False).replace(
            rx("user except handler end") + "\n", "",
        )
        result = analyze(text)
        self.assertEqual("PASS", result.result)
        self.assertIn(
            "user except handler end not observed after complete memory dump",
            result.warnings,
        )

    def test_cp_without_handler_end_warns_but_passes(self) -> None:
        text = complete_log(target="CP", core=0).replace(
            rx("user except handler end") + "\n", "",
        )
        result = analyze(text, target="CP", core=0)
        self.assertEqual("PASS", result.result)
        self.assertIn(
            "user except handler end not observed after complete memory dump",
            result.warnings,
        )

    def test_cp_smp_core_marker_is_not_mistaken_for_ap_direct_dump(self) -> None:
        text = complete_log(target="CP", core=0).replace(
            rx("user except handler begin"),
            rx("SMP-Core-id: 0") + "\n" + rx("user except handler begin"),
        )
        result = analyze(text, target="CP", core=0)
        self.assertEqual("PASS", result.result)
        self.assertEqual("CP", result.actual_target)

    def test_ap_heartbeat_timeout_is_cp_takeover_of_ap_target(self) -> None:
        result = analyze_text(
            ap_heartbeat_takeover_log(), expected_target="AP",
            expected_core=None, expected_mode="heartbeat_timeout",
            boot_expect=[r"BOOT_COMPLETE"], scenario="ap_heartbeat_timeout",
        )
        self.assertEqual("PASS", result.result)
        self.assertEqual("AP", result.actual_target)
        self.assertEqual("CP", result.actual_dumper)
        self.assertIsNone(result.actual_core)
        self.assertTrue(result.heartbeat_timeout_seen)
        self.assertTrue(result.heartbeat_takeover_pass)

    def test_ap_heartbeat_takeover_requires_timeout_marker(self) -> None:
        text = ap_heartbeat_takeover_log().replace(
            rx("IPC[2]heartbeat timeout 100,200") + "\n", "",
        )
        result = analyze_text(
            text, expected_target="AP", scenario="ap_heartbeat_timeout",
        )
        self.assertEqual("DUMP_INCOMPLETE", result.result)
        self.assertFalse(result.heartbeat_takeover_pass)

    def test_cp_heartbeat_timeout_is_ap_observer_of_cp_target(self) -> None:
        result = analyze_text(
            cp_heartbeat_observer_log(), expected_target="CP",
            expected_core=0, expected_mode="heartbeat_timeout",
            boot_expect=[r"BOOT_COMPLETE"], scenario="cp_heartbeat_timeout",
        )
        self.assertEqual("PASS", result.result)
        self.assertEqual("CP", result.actual_target)
        self.assertEqual("AP", result.actual_dumper)
        self.assertEqual(0, result.actual_core)
        self.assertTrue(result.heartbeat_takeover_pass)
        self.assertEqual(1, result.complete_dumps)

    def test_cp_heartbeat_observer_requires_cp_dump_end(self) -> None:
        text = cp_heartbeat_observer_log().replace(
            rx("CP memory dump end") + "\n", "",
        )
        result = analyze_text(
            text, expected_target="CP", scenario="cp_heartbeat_timeout",
        )
        self.assertEqual("DUMP_INCOMPLETE", result.result)
        self.assertFalse(result.heartbeat_takeover_pass)

    def test_blank_rx_payload_lines_are_ignored(self) -> None:
        payload_line = rx(encoded(bytes(range(32))))
        text = complete_log().replace(
            payload_line + "\n", payload_line + "\n" + rx("   ") + "\n",
        )
        result = analyze(text)
        self.assertEqual("PASS", result.result)
        self.assertEqual(2, result.region_details[0].payload_lines)

    def test_late_metadata_after_reason_and_core_is_merged(self) -> None:
        lines = complete_log().splitlines()
        metadata = lines.pop(0)
        handler_index = next(
            index for index, line in enumerate(lines)
            if "user except handler begin" in line
        )
        lines.insert(handler_index + 1, metadata)
        result = analyze("\n".join(lines))
        self.assertEqual("PASS", result.result)
        self.assertEqual("TEST_0001", result.case_id)
        self.assertEqual(1, result.sessions_found)

    def test_metadata_mode_mismatch_warns_without_failing_dump(self) -> None:
        result = analyze(
            complete_log(mode="task_assert", reason="Assert"),
            expected_mode="isr_crash",
        )
        self.assertEqual("PASS", result.result)
        self.assertTrue(result.dump_pass)
        self.assertFalse(result.mode_reason_match)
        self.assertTrue(any("expected mode" in warning
                            for warning in result.warnings))

    def test_deleted_base64_line_fails(self) -> None:
        lines = complete_log().splitlines()
        del lines[10]
        result = analyze("\n".join(lines))
        self.assertIn(result.result, ("DATA_CORRUPT", "REGION_MISMATCH"))
        self.assertFalse(result.dump_pass)

    def test_modified_base64_character_fails_crc(self) -> None:
        lines = complete_log().splitlines()
        original = lines[10]
        replacement = "A" if original[-2] != "A" else "B"
        lines[10] = original[:-2] + replacement + original[-1]
        result = analyze("\n".join(lines))
        self.assertEqual("DATA_CORRUPT", result.result)
        self.assertGreaterEqual(result.crc_failures, 1)

    def test_deleted_region_end_is_incomplete(self) -> None:
        text = "\n".join(
            line for line in complete_log().splitlines()
            if "<<<<stack mem dump end." not in line
        )
        result = analyze(text)
        self.assertEqual("DUMP_INCOMPLETE", result.result)

    def test_wrong_target_does_not_break_dump_integrity(self) -> None:
        result = analyze(complete_log(target="CP", core=0), target="AP", core=1)
        self.assertEqual("PASS", result.result)
        self.assertTrue(result.dump_pass)
        self.assertFalse(result.target_match)
        self.assertEqual("CP", result.actual_target)
        self.assertTrue(any("physical dump is CP" in warning
                            for warning in result.warnings))

    def test_complete_cp_dump_with_ap_metadata_is_still_pass(self) -> None:
        text = complete_log(target="CP", core=0).replace(
            "target=CP core=0", "target=AP core=1",
        )
        result = analyze(text, target="AP", core=1)
        self.assertEqual("PASS", result.result)
        self.assertTrue(result.dump_pass)
        self.assertEqual("AP", result.requested_target)
        self.assertEqual("CP", result.actual_target)
        self.assertEqual(0, result.actual_core)
        self.assertFalse(result.target_match)

    def test_recovery_failure_does_not_break_dump_integrity(self) -> None:
        text = complete_log().replace(rx("RECOVERY_OK") + "\n", "")
        result = analyze(text)
        self.assertEqual("PASS", result.result)
        self.assertTrue(result.dump_pass)
        self.assertFalse(result.recovery_pass)
        self.assertIn("recovery expectation not observed", result.warnings)

    def test_trigger_reject_reason_is_extracted(self) -> None:
        text = rx(
            "DUMP_TEST_REJECT case_id=D000008 reason=timer_busy"
        ) + "\n"
        result = analyze(text, target="CP", core=0)
        self.assertEqual("NOT_TRIGGERED", result.result)
        self.assertFalse(result.dump_present)
        self.assertEqual("timer_busy", result.trigger_reject_reason)

    def test_phys_gap_is_host_error(self) -> None:
        text = "[12:00:00.001] SYS | PHYS GAP: serial closed\n" + complete_log()
        self.assertEqual("HOST_ERROR", analyze(text).result)

    def test_buffer_overflow_is_host_error(self) -> None:
        text = "[12:00:00.001] SYS | BUFFER OVERFLOW: dropped bytes\n" + complete_log()
        self.assertEqual("HOST_ERROR", analyze(text).result)

    def test_tx_dump_like_text_is_ignored(self) -> None:
        prefix = "[12:00:00.000] TX | DUMP_TEST_BEGIN case_id=FAKE target=CP core=0 mode=task_assert\n"
        result = analyze(prefix + complete_log())
        self.assertEqual("PASS", result.result)
        self.assertEqual("TEST_0001", result.case_id)

    def test_fault_reasons_match_modes(self) -> None:
        cases = (
            ("task_assert", "Assert"),
            ("isr_udf", "UsageFault"),
            ("critical_divzero", "Usage Fault"),
            ("task_badpc", "MemFault"),
            ("isr_badpc", "BusFault"),
            ("critical_crash", "HardFault"),
        )
        for mode, reason in cases:
            with self.subTest(mode=mode, reason=reason):
                result = analyze(complete_log(mode=mode, reason=reason))
                self.assertEqual("PASS", result.result)
                self.assertEqual(reason, result.fault_reason)

    def test_reason_prefix_and_case_are_accepted(self) -> None:
        text = complete_log(mode="task_udf", reason="UsageFault").replace(
            "@Dump-reason: UsageFault", "fault-prefix @dUmP-ReAsOn: usage_fault"
        )
        result = analyze(text)
        self.assertEqual("PASS", result.result)
        self.assertEqual("usage_fault", result.fault_reason)

    def test_wrong_fault_reason_is_warning_only(self) -> None:
        result = analyze(complete_log(mode="isr_divzero", reason="HardFault"))
        self.assertEqual("PASS", result.result)
        self.assertTrue(result.dump_pass)
        self.assertFalse(result.mode_reason_match)
        self.assertTrue(any("expected dump reason UsageFault" in error
                            for error in result.warnings))

    def test_manifest_accepts_explicit_ap_region_skip(self) -> None:
        skip = rx(
            ">>>>skip mem dump, region: SRAM5, stack_top=2c180000, "
            "stack end=2c1c0000, reason=ap_bus_untrusted"
        )
        text = complete_log(target="CP", core=0).replace(
            rx("user except handler end"), skip + "\n" + rx("user except handler end"),
        )
        manifest = {
            "regions": [
                {
                    "target": "CP", "build_version": "unit-test-build",
                    "power_state": "default", "region_name": "stack",
                    "start": 0x20000000, "end": 0x20000028,
                    "occurrence_index": 0,
                },
                {
                    "target": "CP", "build_version": "unit-test-build",
                    "power_state": "default", "region_name": "SRAM5",
                    "start": 0x2C180000, "end": 0x2C1C0000,
                    "occurrence_index": 0,
                },
            ],
        }
        result = analyze_text(
            text, expected_target="CP", expected_core=0,
            golden_manifest=manifest,
        )
        self.assertEqual("PASS", result.result)
        self.assertEqual(1, len(result.skip_details))
        self.assertEqual("SRAM5", result.skip_details[0].name)
        self.assertEqual("ap_bus_untrusted", result.skip_details[0].reason)
        self.assertFalse(any("SRAM5" in error for error in result.errors))

    def test_manifest_still_rejects_missing_region_without_skip(self) -> None:
        manifest = {
            "regions": [
                {
                    "target": "CP", "build_version": "unit-test-build",
                    "power_state": "default", "region_name": "stack",
                    "start": 0x20000000, "end": 0x20000028,
                    "occurrence_index": 0,
                },
                {
                    "target": "CP", "build_version": "unit-test-build",
                    "power_state": "default", "region_name": "SRAM5",
                    "start": 0x2C180000, "end": 0x2C1C0000,
                    "occurrence_index": 0,
                },
            ],
        }
        result = analyze_text(
            complete_log(target="CP", core=0),
            expected_target="CP", expected_core=0,
            golden_manifest=manifest,
        )
        self.assertEqual("REGION_MISMATCH", result.result)
        self.assertIn("regions differ from golden manifest", result.errors)


class CaseDefinitionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        data = json.loads((HERE / "cases.json").read_text(encoding="utf-8"))
        cls.cases = {case["id"]: case for case in data["cases"]}

    def test_wrong_password_expect_does_not_match_command_echo(self) -> None:
        case = self.cases["AP0_WRONG_PASSWORD_ISR_CRASH"]
        pattern = case["prepare_expect"][0]
        self.assertIsNone(re.search(
            pattern, "sta ASUS_AC1900P intentionally-wrong-password",
            re.IGNORECASE,
        ))
        self.assertIsNotNone(re.search(
            pattern, "State: 4WAY_HANDSHAKE -> DISCONNECTED",
            re.IGNORECASE,
        ))

    def test_not_found_ssid_fits_wifi_limit(self) -> None:
        case = self.cases["CP_AP_NOT_FOUND_TASK_ASSERT"]
        ssid = case["prepare"][0].split(maxsplit=1)[1]
        self.assertLessEqual(len(ssid.encode("utf-8")), 32)

    def test_ping_load_waits_for_first_reply_not_completion(self) -> None:
        for case in self.cases.values():
            for command, pattern in zip(
                case.get("prepare", []), case.get("prepare_expect", []),
            ):
                if command.startswith("ping "):
                    with self.subTest(case=case["id"]):
                        self.assertNotIn("ping end", pattern)
                        self.assertRegex(
                            "64 bytes from 192.168.1.1 icmp_seq=0",
                            re.compile(pattern, re.IGNORECASE),
                        )

    def test_new_heartbeat_and_owner_follower_matrix(self) -> None:
        mandatory = {
            case["scenario"]: case for case in self.cases.values()
            if case.get("mandatory") and case.get("scenario")
        }
        self.assertEqual({
            "ap_heartbeat_timeout", "cp_heartbeat_timeout", "ap_cp", "cp_ap",
            "ap0_ap1", "ap1_ap0",
        }, set(mandatory))
        self.assertFalse(
            mandatory["ap_heartbeat_timeout"].get("skip_dump_cli_ready", False)
        )
        self.assertFalse(
            mandatory["cp_heartbeat_timeout"].get("skip_dump_cli_ready", False)
        )
        self.assertEqual(
            "CP memory dump end",
            mandatory["cp_heartbeat_timeout"]["dump_end_expect"],
        )
        race = self.cases["AP_OWNER_RACE"]
        self.assertFalse(race["mandatory"])
        self.assertEqual([1, 2], race["expected_winner"])


class ReportTests(unittest.TestCase):
    def test_dump_collection_stops_on_dump_end_or_reboot(self) -> None:
        expect = _dump_completion_expect(
            {"dump_end_expect": "CP memory dump end"},
            {"boot_expect": "user app entry|AP main running"},
        )
        self.assertIsNotNone(re.search(expect, "CP memory dump end"))
        self.assertIsNotNone(re.search(expect, "AP main running"))

    def _report(self):
        return {
            "run_id": "run<&", "seed": 1,
            "started_at": "2026-07-27T12:00:00+08:00",
            "finished_at": "2026-07-27T12:01:00+08:00",
            "cases": [{
                "sequence": 1, "case_id": "CASE<&", "target": "AP", "core": 1,
                "mode": "isr_crash", "load": "base", "result": "HOST_ERROR",
                "dump_pass": False, "boot_pass": False, "recovery_pass": False,
                "crc_lines": 2, "crc_failures": 1, "regions": 1,
                "decoded_bytes": 40, "start_time": "start", "end_time": "end",
                "duration_s": 1.25, "log": "case<&.log",
                "build_version": "build<&", "errors": ["bad <detail>"],
            }],
            "summary": {
                "total": 1, "dump_pass": 0, "dump_fail": 0,
                "not_triggered": 0, "host_error": 1, "skipped": 0,
                "dump_eligible": 0, "dump_pass_rate": None,
                "target_mismatch": 0, "recovery_fail": 0,
            },
        }

    def test_html_report_escapes_and_has_columns_and_colors(self) -> None:
        content = _html_report(self._report())
        self.assertIn("CASE_NAME", content)
        self.assertIn("EXPECTED TARGET", content)
        self.assertIn("ACTUAL TARGET", content)
        self.assertIn("DUMP RESULT", content)
        self.assertIn("Dump pass rate", content)
        self.assertIn('class="badge na"', content)
        self.assertIn(".pass{color:", content)
        self.assertIn(".na{color:", content)
        self.assertIn("CASE&lt;&amp;", content)
        self.assertNotIn("bad <detail>", content)

    def test_html_keeps_warning_only_dump_green(self) -> None:
        report = self._report()
        item = report["cases"][0]
        item.update({
            "result": "PASS", "dump_present": True, "dump_pass": True,
            "target_match": False, "recovery_pass": False, "errors": [],
            "warnings": ["expected <AP> but physical dump is CP"],
        })
        report["summary"] = {
            "total": 1, "dump_pass": 1, "dump_fail": 0,
            "not_triggered": 0, "host_error": 0, "skipped": 0,
            "dump_eligible": 1, "dump_pass_rate": 100.0,
            "target_mismatch": 1, "recovery_fail": 0,
        }
        content = _html_report(report)
        self.assertIn('class="badge pass">PASS', content)
        self.assertIn("expected &lt;AP&gt;", content)
        self.assertNotIn("expected <AP>", content)

    def test_summary_uses_dump_eligibility_only(self) -> None:
        records = [
            {"result": "PASS", "dump_present": True, "dump_pass": True,
             "target_match": False, "recovery_pass": False},
            {"result": "DUMP_INCOMPLETE", "dump_present": True,
             "dump_pass": False, "target_match": True,
             "recovery_pass": True},
            {"result": "NOT_TRIGGERED", "dump_present": False,
             "dump_pass": False, "target_match": None,
             "recovery_pass": False},
            {"result": "PRECONDITION_FAILED", "dump_present": False,
             "dump_pass": False, "target_match": None,
             "recovery_pass": False},
        ]
        summary = _summary(records)
        self.assertEqual(1, summary["dump_pass"])
        self.assertEqual(1, summary["dump_fail"])
        self.assertEqual(2, summary["dump_eligible"])
        self.assertEqual(50.0, summary["dump_pass_rate"])
        self.assertEqual(1, summary["target_mismatch"])
        self.assertEqual(0, summary["recovery_fail"])

    def test_race_uses_single_complete_dump_and_retained_status(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            run_dir = Path(temporary)
            case = {
                "id": "AP_OWNER_RACE", "target": "AP", "core": None,
                "mode": "task_assert", "scenario": "ap_race",
                "expected_winner": [1, 2], "status_scenario": 5,
                "expected_phase": 3,
                "status_command": "ap_cmd ap_dump_orch_status",
            }
            log = run_dir / "0001_AP_OWNER_RACE.log"
            text = (
                rx("DUMP_ORCH_ARMED case_id=D000001 scenario=ap_race "
                   "follower=dynamic mode=task_assert") + "\n"
                + complete_log(target="AP", core=1, mode="task_assert",
                               reason="Assert", legacy_ap_markers=False)
                + rx("DUMP_ORCH_OWNER winner=2 follower_wait=1") + "\n"
                + rx("DUMP_TEST_BEGIN case_id=D000001 target=AP core=0 "
                     "mode=task_assert") + "\n"
                + rx("DUMP_ORCH_STATUS raw=0x1 valid=1 scenario=5 phase=3 "
                     "winner=2 follower=1") + "\n"
            )
            log.write_text(text, encoding="utf-8")
            analysis = analyze_text(
                text, expected_target="AP", expected_core=None,
                expected_mode="task_assert", scenario="ap_race",
            )
            record = _base_record(1, case, log)
            _apply_analysis(record, analysis)
            _evaluate_case_markers(record, case, log)
            self.assertEqual("PASS", record["result"])
            self.assertEqual(1, record["complete_dumps"])
            self.assertEqual(2, record["actual_winner"])
            self.assertTrue(record["winner_match"])
            self.assertTrue(record["status_pass"])
            self.assertTrue(record["follower_silent"])

    def test_quiesced_ap_peer_counts_as_silent_after_ready_barrier(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            run_dir = Path(temporary)
            case = {
                "id": "AP0_OWNER_AP1_FOLLOWER", "target": "AP", "core": 0,
                "mode": "task_assert", "scenario": "ap0_ap1",
                "expected_winner": 1, "status_scenario": 3,
                "expected_phase": 3,
                "status_command": "ap_cmd ap_dump_orch_status",
            }
            log = run_dir / "0001_AP0_OWNER_AP1_FOLLOWER.log"
            text = (
                complete_log(target="AP", core=0, mode="task_assert",
                             reason="Assert", legacy_ap_markers=False)
                + rx("DUMP_ORCH_OWNER winner=1 follower_wait=0") + "\n"
                + rx("DUMP_ORCH_STATUS raw=0x0 valid=0 scenario=0 phase=0 "
                     "winner=0 follower=0") + "\n"
            )
            log.write_text(text, encoding="utf-8")
            analysis = analyze_text(
                text, expected_target="AP", expected_core=0,
                expected_mode="task_assert", scenario="ap0_ap1",
            )
            record = _base_record(1, case, log)
            _apply_analysis(record, analysis)
            _evaluate_case_markers(record, case, log)
            self.assertTrue(record["status_pass"])
            self.assertTrue(record["follower_silent"])
            self.assertNotIn(
                "follower silence was not proven", record["warnings"],
            )

    def test_reanalyze_existing_log_and_write_reports(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            run_dir = Path(temporary)
            log = run_dir / "0001_AP_TEST.log"
            log.write_text(complete_log(), encoding="utf-8")
            report = self._report()
            report["run_id"] = "run"
            report["cases"][0].update({
                "case_id": "AP_TEST", "log": log.name, "core": 1,
                "mode": "isr_crash",
            })
            cases = [{
                "id": "AP_TEST", "target": "AP", "core": 1,
                "mode": "isr_crash",
                "trigger": "ap_cmd ap_dump_test ${CASE_ID} isr_crash 1",
            }]
            config = {"run": {"boot_expect": "BOOT_COMPLETE"}}
            reanalyze_report(run_dir, report, cases, config)
            self.assertEqual("PASS", report["cases"][0]["result"])
            self.assertTrue(report["cases"][0]["start_time"])
            self.assertIn("reanalyzed_at", report)
            write_reports(run_dir, report)
            self.assertTrue((run_dir / "report.html").is_file())
            saved = json.loads((run_dir / "result.json").read_text(encoding="utf-8"))
            self.assertEqual(1, saved["summary"]["dump_pass"])

    def test_reanalyze_duration_ends_at_first_boot_match(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            run_dir = Path(temporary)
            body = complete_log().replace(
                "[12:00:00.000]", "[23:59:00.000]",
            ).replace(
                "[23:59:00.000] RX | BOOT_COMPLETE",
                "[00:02:00.000] RX | BOOT_COMPLETE",
            ).replace(
                "[23:59:00.000] RX | RECOVERY_OK",
                "[00:02:00.000] RX | RECOVERY_OK",
            )
            text = (
                "[23:58:00.000] TX | ap_cmd ap_dump_test D000001 isr_crash 1\n"
                + body
                + "\n----- new session @ 2026-07-28 02:59:59 -----\n"
                + "[03:00:00.000] RX | late environment log\n"
            )
            log = run_dir / "0001_AP_TEST.log"
            log.write_text(text, encoding="utf-8")
            report = self._report()
            report["started_at"] = "2026-07-27T23:50:00+08:00"
            report["cases"][0].update({
                "case_id": "AP_TEST", "log": log.name, "core": 1,
                "mode": "isr_crash",
            })
            cases = [{
                "id": "AP_TEST", "target": "AP", "core": 1,
                "mode": "isr_crash",
                "trigger": "ap_cmd ap_dump_test ${CASE_ID} isr_crash 1",
            }]
            config = {"run": {"boot_expect": "BOOT_COMPLETE"}}
            reanalyze_report(run_dir, report, cases, config)
            record = report["cases"][0]
            self.assertEqual("PASS", record["result"])
            self.assertEqual(240.0, record["duration_s"])
            self.assertIn("2026-07-28T00:02:00", record["end_time"])


if __name__ == "__main__":
    unittest.main()
