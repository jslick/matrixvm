/**
 * @file    opcodes.h
 *
 * Matrix VM
 */

#ifndef OPCODES
#define OPCODES

/*
 * General instruction encoding
 * *---------------------------------------*
 * |  31-24 | 23-20 |  19-17 | 16-13 | ... |
 * | opcode |  cnd  | addrmd |  reg  | ... |
 * *---------------------------------------*
 */

/* Registers */
#define INS_REG 13
#define INS_REG_MASK ( 0xF << INS_REG )
#define MAX_REGISTERS ( (INS_REG_MASK >> INS_REG) + 1 )
#define EXTRACT_REG(instruction) ( (instruction & INS_REG_MASK) >> INS_REG )
#define EXTRACT_SRC_REG(instruction) ( instruction & 0xF )
#define R1  ( 1 << INS_REG )
#define R2  ( 2 << INS_REG )
#define R3  ( 3 << INS_REG )
#define R4  ( 4 << INS_REG )
#define R5  ( 5 << INS_REG )
#define R6  ( 6 << INS_REG )

/* Addressing modes */
#define INS_ADDR    17
#define ABSOLUTE    ( 0 << INS_ADDR )
#define RELATIVE    ( 1 << INS_ADDR )
#define IMMEDIATE   ( 2 << INS_ADDR )
#define REGISTER    ( 3 << INS_ADDR )
#define INDIRECT    ( 4 << INS_ADDR )

/* Opcodes */

#define INS_OPCODE      24
#define INS_OPCODE_MASK ( 0xFF << INS_OPCODE )

// CPU modes/stuff
#define HALT    ( 0x00 << INS_OPCODE )

// Control flow
#define JMP     ( 0x10 << INS_OPCODE )
#define LNGJMP  ( 0x11 << INS_OPCODE )
#define CALL    ( 0x14 << INS_OPCODE )
#define LNGCALL ( 0x15 << INS_OPCODE )
#define RET     ( 0x16 << INS_OPCODE )

// Move
#define MOV     ( 0x20 << INS_OPCODE )

// Load/Store
#define LOAD    ( 0x28 << INS_OPCODE )
#define LOADW   ( 0x29 << INS_OPCODE )
#define LOADB   ( 0x2a << INS_OPCODE )
#define STORE   ( 0x30 << INS_OPCODE )
#define STOREW  ( 0x31 << INS_OPCODE )
#define STOREB  ( 0x32 << INS_OPCODE )
#define MEMCPY  ( 0x38 << INS_OPCODE )
#define MEMSET  ( 0x39 << INS_OPCODE )  // NOTE:  not yet implemented
#define CLRSET  ( 0x3a << INS_OPCODE )

// I/O
#define WRITE   ( 0x40 << INS_OPCODE )

// Math
#define ADD     ( 0x50 << INS_OPCODE )
#define MUL     ( 0x54 << INS_OPCODE )
#define MULW    ( 0x55 << INS_OPCODE )

#endif // OPCODES
