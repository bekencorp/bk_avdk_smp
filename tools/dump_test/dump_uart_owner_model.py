"""Host model of the firmware dump-UART owner/token state machine."""

from dataclasses import dataclass

NONE, AP, CP, AP_HANDOFF = 0, 1, 2, 3
SEQUENCE_MASK = 0x3FFFFFFF


@dataclass(frozen=True)
class State:
    owner: int = NONE
    sequence: int = 0


@dataclass(frozen=True)
class LocalState:
    locked: bool = False
    hspl_locked: bool = False
    sequence: int = 0
    core_id: int = -1
    hspl_poisoned: bool = False
    force_write: bool = False


def local_owner_active(
    local: LocalState,
    shared: State,
    expected_owner: int,
    current_core: int,
) -> bool:
    return (
        local.locked
        and local.core_id == current_core
        and shared.owner == expected_owner
        and shared.sequence == local.sequence
    )


def exception_is_secondary(
    magic_set: bool,
    local: LocalState,
    shared: State,
    expected_owner: int,
    current_core: int,
) -> bool:
    return magic_set and local_owner_active(
        local, shared, expected_owner, current_core,
    )


def reset_stale_local(
    local: LocalState,
    shared: State,
    expected_owner: int,
) -> tuple[LocalState, bool]:
    token_matches = (
        local.locked
        and shared.owner == expected_owner
        and shared.sequence == local.sequence
    )
    if token_matches:
        return local, False
    # Firmware deliberately clears bookkeeping without issuing HSPL unlock.
    return LocalState(hspl_poisoned=local.hspl_locked or local.hspl_poisoned), False


def abort_local_owner(
    local: LocalState,
    shared: State,
    expected_owner: int,
    current_core: int,
) -> tuple[LocalState, State, bool]:
    token_valid = local_owner_active(
        local, shared, expected_owner, current_core,
    )
    hspl_owned = (
        token_valid
        and local.hspl_locked
        and not local.force_write
        and not local.hspl_poisoned
    )
    new_shared = (
        release(shared, expected_owner, local.sequence)
        if token_valid else shared
    )
    released = new_shared != shared
    cleared = LocalState(
        hspl_poisoned=local.hspl_locked and not (released and hspl_owned),
    )
    return cleared, new_shared, released and hspl_owned


def next_sequence(state: State) -> int:
    sequence = (state.sequence + 1) & SEQUENCE_MASK
    return sequence or 1


def claim_after_hspl(state: State, owner: int) -> State:
    if owner not in (AP, CP):
        return state
    allowed = (
        state.owner == NONE
        or (state.owner == AP_HANDOFF and owner == CP)
    )
    if not allowed:
        return state
    return State(owner, next_sequence(state))


def force_claim(state: State, owner: int) -> State:
    if owner not in (AP, CP) or state.owner != NONE:
        return state
    return State(owner, next_sequence(state))


def adopt(state: State, owner: int) -> int | None:
    if owner not in (AP, CP) or state.owner != owner or state.sequence == 0:
        return None
    return state.sequence


def force_write_after_hspl_failure(
    state: State,
    owner: int,
    local_claim_succeeded: bool = False,
) -> bool:
    if owner == AP:
        # AP0/AP1 share one owner value, so an AP may never infer local
        # ownership by adopting an existing AP token.
        return local_claim_succeeded and state.owner == AP
    return owner == CP and adopt(state, CP) is not None


def boot_reset(state: State) -> State:
    return State(NONE, next_sequence(state))


def boot_reset_local(
    local: LocalState,
    shared: State,
    expected_owner: int,
) -> tuple[LocalState, State, bool]:
    token_matches = (
        local.locked
        and shared.owner == expected_owner
        and shared.sequence == local.sequence
    )
    retired = (
        release(shared, expected_owner, local.sequence)
        if token_matches else shared
    )
    cleared = LocalState(
        hspl_poisoned=local.hspl_locked or local.hspl_poisoned,
    )
    return cleared, retired, False


def direct_dump_complete(state: State, owner: int, sequence: int) -> State:
    """A completed direct dump retains its exact owner token until reboot."""
    if state.owner != owner or state.sequence != sequence:
        return state
    return state


def heartbeat_timeout(state: State) -> tuple[State, bool]:
    """Return post-timeout state and whether CP should enter its assert dump."""
    if state.owner != NONE:
        return state, False
    return force_claim(state, CP), True


def cp_heartbeat_timeout(state: State) -> tuple[State, bool]:
    """Return state and whether AP should start its CP-hang fallback dump."""
    if state.owner != NONE:
        return state, False
    return claim_after_hspl(state, AP), True


def handoff(state: State, owner: int, sequence: int) -> State:
    if owner != AP or state.owner != AP or state.sequence != sequence:
        return state
    return State(AP_HANDOFF, sequence)


def release(state: State, owner: int, sequence: int) -> State:
    if state.owner != owner or state.sequence != sequence:
        return state
    return State(NONE, sequence)
