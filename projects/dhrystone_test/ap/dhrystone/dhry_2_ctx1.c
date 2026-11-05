/*
 * Dhrystone 2.1 - second instance (context 1), part 3 of 3.
 *
 * Mirrors dhry_1_ctx1.c: the official dhry_2.c (Proc_6..8 / Func_1..3) is pulled
 * in a second time. CMake compiles this wrapper with the same globals renamed
 * (Int_Glob_1, Ch_1_Glob_1) and the Proc_/Func_ symbols suffixed, so the second
 * instance links against its own copy of the benchmark code.
 */
#include "dhry_2.c"
