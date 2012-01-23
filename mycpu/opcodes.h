/**
 * @file    opcodes.h
 *
 * Matrix VM
 */

#ifndef OPCODES
#define OPCODES

/*
 * *---------------------------------------------*
 * |  31 |  30-23 |  22-20 | 19-17 |  16 || 15-0 |
 * | cnd | opcode | addrmd | extra | reg || word |
 * *---------------------------------------------*
 */

/* Registers */
#define INS_REG 16
#define INS_REG_MASK ( 0xF << INS_REG )
#define EXTRACT_REG(instruction) ( (instruction & INS_REG_MASK) >> INS_REG )
#define R1  ( 1 << INS_REG )
#define R2  ( 2 << INS_REG )

/* Addressing modes */
#define INS_ADDR 20
#define ABSOLUTE    ( 0 << INS_ADDR )
#define RELATIVE    ( 1 << INS_ADDR )
#define IMMEDIATE   ( 2 << INS_ADDR )
#define INDIRECT    ( 3 << INS_ADDR )

/* Opcodes */

#define INS_OPCODE 23
#define INS_OPCODE_MASK ( 0xFF << INS_OPCODE )

// CPU modes/stuff
#define HALT    ( 0x00 << INS_OPCODE )

// Control flow
#define JMP     ( 0x10 << INS_OPCODE )
#define LNGJMP  ( 0x11 << INS_OPCODE )

// Register load/store
#define LOAD    ( 0x20 << INS_OPCODE )
#define LOADW   ( 0x21 << INS_OPCODE )
#define LOADB   ( 0x22 << INS_OPCODE )
#define STORE   ( 0x28 << INS_OPCODE )
#define STOREW  ( 0x29 << INS_OPCODE )
#define STOREB  ( 0x2a << INS_OPCODE )
#define MEMCPY  ( 0x30 << INS_OPCODE )

// I/O
#define WRITE   ( 0x40 << INS_OPCODE )

#endif // OPCODES