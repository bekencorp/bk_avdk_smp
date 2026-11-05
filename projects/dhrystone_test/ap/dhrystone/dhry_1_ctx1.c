/*
 * Dhrystone 2.1 - second instance (context 1) for the SMP dual-core run.
 *
 * The official dhry_1.c keeps all of its mutable state in file-scope globals and
 * is therefore not reentrant: running the same translation unit on both cores
 * would make the two contexts overwrite each other's Ptr_Glob / Arr_2_Glob /
 * Int_Glob and fail the final-value check. This wrapper simply pulls the
 * unmodified official source in a second time; CMake compiles it with a set of
 * -D renames (Ptr_Glob=Ptr_Glob_1, Proc_1=Proc_1_1, main=dhrystone_main1, ...)
 * so both instances get their own .bss and their own entry point, and neither
 * the official files nor the first instance are touched.
 */
#include "dhry_1.c"
