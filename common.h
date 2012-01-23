/**
 * @file    common.h
 *
 * Matrix VM
 */

#include <stdint.h>
#include <vector>

// Config
#define DEBUG       0
#define CHECK_INSTR 1

// shared library declaration
#ifdef __cplusplus
#define SLDECL extern "C"
#else
#define SLDECL
#endif  // __cplusplus

typedef int32_t MemAddress;
