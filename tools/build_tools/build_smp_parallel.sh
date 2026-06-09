#!/usr/bin/env bash
# Build AP and CP in parallel with normal stdout/stderr. On first failure, stop the
# other side's process group (fail-fast).
set -u

SOC="${ARMINO_SOC:?ARMINO_SOC not set}"
ARMINO_TOOLS_PATH="${ARMINO_TOOLS_PATH:?ARMINO_TOOLS_PATH not set}"
PROJECT_DIR="${PROJECT_DIR:?PROJECT_DIR not set}"
BUILD_DIR="${BUILD_DIR:?BUILD_DIR not set}"
APP_NAME="${APP_NAME:-}"
APP_VERSION="${APP_VERSION:-unknown}"
ARMINO_AP_DIR="${ARMINO_AP_DIR:?ARMINO_AP_DIR not set}"
ARMINO_CP_DIR="${ARMINO_CP_DIR:?ARMINO_CP_DIR not set}"

MAKE_BIN="${MAKE:-make}"

kill_group() {
	local leader="${1:-}"
	[[ -z "${leader}" ]] && return 0
	kill -TERM "-${leader}" 2>/dev/null || kill -TERM "${leader}" 2>/dev/null || true
}

run_make() {
	local dir="$1"
	local goal="$2"
	local -a cmd=(
		"${MAKE_BIN}" "${goal}"
		"ARMINO_TOOLS_PATH=${ARMINO_TOOLS_PATH}"
		"PROJECT_DIR=${PROJECT_DIR}"
		"BUILD_DIR=${BUILD_DIR}"
		"APP_NAME=${APP_NAME}"
		"APP_VERSION=${APP_VERSION}"
		-C "${dir}"
	)
	if command -v setsid >/dev/null 2>&1; then
		setsid "${cmd[@]}" &
	else
		"${cmd[@]}" &
	fi
}

run_make "${ARMINO_AP_DIR}" "${SOC}_ap"
ap_leader=$!

run_make "${ARMINO_CP_DIR}" "${SOC}"
cp_leader=$!

fail=0
for _ in 1 2; do
	if ! wait -n; then
		fail=1
		break
	fi
done

if [[ "${fail}" -ne 0 ]]; then
	kill_group "${ap_leader}"
	kill_group "${cp_leader}"
	wait 2>/dev/null || true
	exit 1
fi

exit 0
