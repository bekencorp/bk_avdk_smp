#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
docker run --rm -v "${SCRIPT_DIR}/..:/work" \
            -w /work/tests -u $(id -u):$(id -g) \
            i386/micky/gtest \
    sh -c "cmake -B build . && cmake --build build --parallel 8 && ctest --test-dir build -V"
