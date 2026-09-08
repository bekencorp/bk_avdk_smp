#!/usr/bin/env bash
# Prepare + list P0 OTA XIP tests. Board flash/confirm still manual.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
PY="${ROOT}/ota_xip_test.py"

BUILD_DIR="${1:-}"
if [[ -z "${BUILD_DIR}" ]]; then
  CAND="${ROOT}/../../../../build/bk7258/secureboot_xip/bk7258/_build"
  if [[ -d "${CAND}" ]]; then
    BUILD_DIR="${CAND}"
  fi
fi

echo "== P0 plan =="
python3 "${PY}" plan --priority P0

echo
echo "== prepare recipes (artifacts/) =="
if [[ -n "${BUILD_DIR}" ]]; then
  python3 "${PY}" prepare p0 --build-dir "${BUILD_DIR}"
  echo
  echo "== check-pack AES=NONE =="
  python3 "${PY}" check-pack --build-dir "${BUILD_DIR}" --aes none --record || true
else
  python3 "${PY}" prepare p0
  echo "(pass build _build as \$1 to also check-pack / mutate ota.bin)"
fi

echo
echo "Board loop:"
echo "  1) read artifacts/<ID>/RUN.txt"
echo "  2) flash boot_param.bin @ 0x7fc000 (and ota_* if any)"
echo "  3) warm/cold reset; watch BL2 logs"
echo "  4) python3 ${PY} record <ID> pass|fail --note '...'"
echo
echo "Start with:  ls ${ROOT}/artifacts/*/RUN.txt"
