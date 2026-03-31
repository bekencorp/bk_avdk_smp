#!/bin/bash

docker run --rm \
    -v $(dirname $PWD):/work \
    -w /work/tests/unit \
    -u $(id -u):$(id -g) \
    i386/micky/gtest:latest \
    sh -c "cmake -B build . && cmake --build build --parallel $(nproc) && ctest --test-dir build -V"
