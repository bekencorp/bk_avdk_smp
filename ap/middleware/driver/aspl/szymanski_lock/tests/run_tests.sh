#!/bin/bash
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
docker run --rm -v "$ROOT":/work -w /work/tests -u $(id -u):$(id -g) i386/micky/gtest \
    sh -c "cmake -B build . && cmake --build build --parallel $(nproc) && ctest --test-dir build -V"
