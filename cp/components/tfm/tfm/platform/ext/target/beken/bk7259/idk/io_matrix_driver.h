// Copyright 2025 Beken
//
// BK7259 io_matrix_driver.h stub.
//
// The Beken-patched TF-M BL2 (bl2_main.c) includes io_matrix_driver.h
// unconditionally. io_matrix (IO Matrix v2.0) is a bk7236n/bk7239n feature;
// BK7259 does not use it in the secure boot path. Provide an empty stub so
// the include resolves. Add real declarations if BK7259 io_matrix is needed.

#pragma once
