/**
 * @file    common.h
 *
 * Matrix VM
 */

#include <stdint.h>
#include <vector>

// Config

#ifndef DEBUG
#  define DEBUG         0
#endif

#ifndef EMULATOR_PROFILE
#  define EMULATOR_PROFILE 0
#endif

// Check validity of instructions
#ifndef CHECK_INSTR
#  define CHECK_INSTR   1
#endif

// shared library declaration
#ifdef __cplusplus
#  define SLDECL extern "C"
#else
#  define SLDECL
#endif  // __cplusplus

typedef int32_t MemAddress;
