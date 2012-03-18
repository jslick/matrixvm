/**
 * @file    opcodes.h
 *
 * Matrix VM
 */

#ifndef OPCODES
#define OPCODES

/*
 * General instruction encoding
 * *--------------------------------*
 * |  31-24 |  23-21 | 20-17 | 15-0 |
 * | opcode | addrmd |  reg  |  src |
 * *--------------------------------*
 */

/* Registers */
#define INS_REG 17
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

/* Status register */

#define STATUS_INTERRUPT_MASK   (0b10000000 << 24)
#define STATUS_ZERO_MASK        (0b00000001 <<  0)
#define STATUS_NEG_MASK         (0b00000010 <<  0)
#define STATUS_CARRY_MASK       (0b00000100 <<  0)
#define STATUS_OVERFLOW_MASK    (0b00001000 <<  0)

/* Addressing modes */
#define INS_ADDR    21
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
#define IDLE    ( 0x01 << INS_OPCODE )
#define CLI     ( 0x03 << INS_OPCODE )
#define STI     ( 0x04 << INS_OPCODE )

// Control flow
#define CMP     ( 0x10 << INS_OPCODE )
#define JMP     ( 0x19 << INS_OPCODE )
#define LNGJMP  ( 0x1a << INS_OPCODE )
#define JE      ( 0x1b << INS_OPCODE )
#define JNE     ( 0x1c << INS_OPCODE )
#define CALL    ( 0x20 << INS_OPCODE )
#define LNGCALL ( 0x21 << INS_OPCODE )
#define RET     ( 0x23 << INS_OPCODE )
#define RTI     ( 0x24 << INS_OPCODE )

// Move
#define MOV     ( 0x30 << INS_OPCODE )

// Load/Store
#define LOAD    ( 0x38 << INS_OPCODE )
#define LOADW   ( 0x39 << INS_OPCODE )
#define LOADB   ( 0x3a << INS_OPCODE )
#define STORE   ( 0x40 << INS_OPCODE )
#define STOREW  ( 0x41 << INS_OPCODE )
#define STOREB  ( 0x42 << INS_OPCODE )
#define MEMCPY  ( 0x48 << INS_OPCODE )
#define MEMSET  ( 0x49 << INS_OPCODE )  // NOTE:  not yet implemented
#define CLRSET  ( 0x4a << INS_OPCODE )
#define CLRSETV ( 0x4b << INS_OPCODE )

// I/O
#define READ    ( 0x50 << INS_OPCODE )
#define WRITE   ( 0x51 << INS_OPCODE )

// Math
#define ADD     ( 0x60 << INS_OPCODE )
#define MUL     ( 0x64 << INS_OPCODE )
#define MULW    ( 0x65 << INS_OPCODE )

#endif // OPCODES
