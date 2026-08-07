import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))

from dump_uart_owner_model import (  # noqa: E402
    AP,
    AP_HANDOFF,
    CP,
    LocalState,
    NONE,
    State,
    abort_local_owner,
    adopt,
    boot_reset,
    boot_reset_local,
    claim_after_hspl,
    cp_heartbeat_timeout,
    direct_dump_complete,
    exception_is_secondary,
    force_claim,
    force_write_after_hspl_failure,
    handoff,
    heartbeat_timeout,
    reset_stale_local,
    release,
)


class DumpUartOwnerModelTests(unittest.TestCase):
    def test_normal_claim_and_release(self) -> None:
        owned = claim_after_hspl(State(), AP)
        self.assertEqual(State(AP, 1), owned)
        self.assertEqual(State(NONE, 1), release(owned, AP, 1))

    def test_hspl_success_cannot_overwrite_active_force_peer(self) -> None:
        active_force = State(AP, 9)
        self.assertEqual(active_force, claim_after_hspl(active_force, CP))

    def test_same_owner_cannot_reclaim_shared_ap_token(self) -> None:
        active = State(AP, 9)
        self.assertEqual(active, claim_after_hspl(active, AP))

    def test_ap_handoff_to_cp_claim_succeeds(self) -> None:
        active = State(AP, 9)
        handed = handoff(active, AP, 9)
        self.assertEqual(State(AP_HANDOFF, 9), handed)
        claimed = claim_after_hspl(handed, CP)
        self.assertEqual((CP, 10), (claimed.owner, claimed.sequence))

    def test_ap_handoff_to_ap_claim_is_rejected(self) -> None:
        handed = State(AP_HANDOFF, 9)
        self.assertEqual(handed, claim_after_hspl(handed, AP))

    def test_stale_handoff_token_is_rejected(self) -> None:
        active = State(AP, 10)
        self.assertEqual(active, handoff(active, AP, 9))

    def test_force_claim_only_wins_from_none(self) -> None:
        self.assertEqual(State(CP, 10), force_claim(State(NONE, 9), CP))
        active = State(AP, 9)
        self.assertEqual(active, force_claim(active, CP))

    def test_ap_direct_dump_holds_owner_until_boot_reset(self) -> None:
        active = claim_after_hspl(State(), AP)
        completed = direct_dump_complete(active, AP, active.sequence)
        self.assertEqual(active, completed)
        self.assertEqual(State(NONE, 2), boot_reset(completed))

    def test_heartbeat_cannot_take_ap_active_owner(self) -> None:
        active = State(AP, 9)
        retained, should_assert = heartbeat_timeout(active)
        self.assertEqual(active, retained)
        self.assertFalse(should_assert)

    def test_heartbeat_defers_for_handoff_and_cp_owner(self) -> None:
        for owner in (AP_HANDOFF, CP):
            with self.subTest(owner=owner):
                active = State(owner, 9)
                retained, should_assert = heartbeat_timeout(active)
                self.assertEqual(active, retained)
                self.assertFalse(should_assert)

    def test_heartbeat_none_force_claims_cp_and_asserts(self) -> None:
        claimed, should_assert = heartbeat_timeout(State(NONE, 9))
        self.assertEqual(State(CP, 10), claimed)
        self.assertTrue(should_assert)

    def test_cp_heartbeat_cannot_interrupt_cp_direct_dump(self) -> None:
        active = State(CP, 9)
        retained, should_dump = cp_heartbeat_timeout(active)
        self.assertEqual(active, retained)
        self.assertFalse(should_dump)

    def test_cp_heartbeat_none_allows_ap_fallback_dump(self) -> None:
        claimed, should_dump = cp_heartbeat_timeout(State(NONE, 9))
        self.assertEqual(State(AP, 10), claimed)
        self.assertTrue(should_dump)

    def test_stale_release_does_not_clear_new_owner(self) -> None:
        current = State(CP, 10)
        self.assertEqual(current, release(current, AP, 9))
        self.assertEqual(current, release(current, CP, 9))

    def test_adopt_same_owner_succeeds_and_peer_fails(self) -> None:
        published = State(CP, 10)
        self.assertEqual(10, adopt(published, CP))
        self.assertIsNone(adopt(published, AP))

    def test_boot_reset_clears_retained_owner_and_advances_sequence(self) -> None:
        self.assertEqual(State(NONE, 10), boot_reset(State(AP, 9)))

    def test_boot_reset_sequence_wrap_skips_zero(self) -> None:
        self.assertEqual(State(NONE, 1), boot_reset(State(CP, 0x3FFFFFFF)))

    def test_ap_force_write_requires_published_ap_owner(self) -> None:
        self.assertFalse(force_write_after_hspl_failure(State(NONE, 9), AP))
        self.assertFalse(force_write_after_hspl_failure(State(CP, 9), AP))
        self.assertFalse(force_write_after_hspl_failure(State(AP, 10), AP))
        self.assertTrue(force_write_after_hspl_failure(
            State(AP, 10), AP, local_claim_succeeded=True,
        ))

    def test_cp_force_write_requires_published_cp_owner(self) -> None:
        self.assertFalse(force_write_after_hspl_failure(State(NONE, 9), CP))
        self.assertFalse(force_write_after_hspl_failure(State(AP, 9), CP))
        self.assertTrue(force_write_after_hspl_failure(State(CP, 10), CP))

    def test_stale_magic_without_local_owner_is_primary(self) -> None:
        self.assertFalse(exception_is_secondary(
            True, LocalState(), State(NONE, 9), AP, 1,
        ))

    def test_magic_with_valid_local_token_is_secondary(self) -> None:
        local = LocalState(True, True, 9, 1)
        shared = State(AP, 9)
        self.assertTrue(exception_is_secondary(True, local, shared, AP, 1))

    def test_peer_takeover_clears_local_without_hspl_unlock(self) -> None:
        stale = LocalState(True, True, 9, 1)
        peer = State(CP, 10)
        cleared, hspl_unlocked = reset_stale_local(stale, peer, AP)
        self.assertEqual(LocalState(hspl_poisoned=True), cleared)
        self.assertFalse(hspl_unlocked)

    def test_handoff_makes_completed_ap_owner_inactive(self) -> None:
        local = LocalState(True, True, 9, 1)
        handed = State(AP_HANDOFF, 9)
        self.assertFalse(exception_is_secondary(True, local, handed, AP, 1))

    def test_secondary_abort_releases_own_token_and_hspl(self) -> None:
        local = LocalState(True, True, 9, 1)
        shared = State(AP, 9)
        cleared, released, hspl_unlocked = abort_local_owner(
            local, shared, AP, 1,
        )
        self.assertEqual(LocalState(), cleared)
        self.assertEqual(State(NONE, 9), released)
        self.assertTrue(hspl_unlocked)

    def test_secondary_abort_does_not_clear_peer_takeover(self) -> None:
        stale = LocalState(True, True, 9, 1)
        peer = State(CP, 10)
        cleared, retained, hspl_unlocked = abort_local_owner(
            stale, peer, AP, 1,
        )
        self.assertEqual(LocalState(hspl_poisoned=True), cleared)
        self.assertEqual(peer, retained)
        self.assertFalse(hspl_unlocked)

    def test_retained_secondary_recovers_to_none_in_one_abort(self) -> None:
        retained_local = LocalState(True, True, 9, 1)
        retained_shared = State(AP, 9)
        self.assertTrue(exception_is_secondary(
            True, retained_local, retained_shared, AP, 1,
        ))
        cleared, shared, _ = abort_local_owner(
            retained_local, retained_shared, AP, 1,
        )
        self.assertEqual(LocalState(), cleared)
        self.assertEqual(NONE, shared.owner)
        self.assertFalse(exception_is_secondary(False, cleared, shared, AP, 1))

    def test_normal_boot_retires_matching_retained_ap_epoch(self) -> None:
        retained_local = LocalState(True, True, 9, 1)
        retained_shared = State(AP, 9)
        cleared, shared, hspl_unlocked = boot_reset_local(
            retained_local, retained_shared, AP,
        )
        self.assertEqual(LocalState(hspl_poisoned=True), cleared)
        self.assertEqual(State(NONE, 9), shared)
        self.assertFalse(hspl_unlocked)
        self.assertFalse(exception_is_secondary(
            False, cleared, shared, AP, 1,
        ))


if __name__ == "__main__":
    unittest.main()
