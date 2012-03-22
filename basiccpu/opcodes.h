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
 * |  31-24 |  23-21 | 19-16 | 15-0 |
 * | opcode | addrmd |  reg  |  src |
 * *--------------------------------*
 */

/* Registers */
#define INS_REG 16
#define INS_REG_MASK ( 0xF << INS_REG )
#define MAX_REGISTERS ( (INS_REG_MASK >> INS_REG) + 1 )
#define EXTRACT_REG(instruction) ( (instruction & INS_REG_MASK) >> INS_REG )
#define EXTRACT_SRC_REG(instruction) ( instruction & 0xF )
#define R1  (  1 << INS_REG )
#define R2  (  2 << INS_REG )
#define R3  (  3 << INS_REG )
#define R4  (  4 << INS_REG )
#define R5  (  5 << INS_REG )
#define R6  (  6 << INS_REG )
#define R7  (  7 << INS_REG )
#define SP  ( 11 << INS_REG )
#define LR  ( 12 << INS_REG )
#define IP  ( 13 << INS_REG )
#define DL  ( 14 << INS_REG )
#define ST  ( 15 << INS_REG )

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
#define RSTR    ( 0x08 << INS_OPCODE )

// Control flow
#define CMP     ( 0x10 << INS_OPCODE )
#define TST     ( 0x15 << INS_OPCODE )
#define JMP     ( 0x19 << INS_OPCODE )
#define LNGJMP  ( 0x1a << INS_OPCODE )
#define JE      ( 0x1b << INS_OPCODE )
#define JNE     ( 0x1c << INS_OPCODE )
#define JGE     ( 0x1d << INS_OPCODE )
#define JG      ( 0x1e << INS_OPCODE )
#define JLE     ( 0x1f << INS_OPCODE )
#define JL      ( 0x20 << INS_OPCODE )
#define CALL    ( 0x21 << INS_OPCODE )
#define LNGCALL ( 0x22 << INS_OPCODE )
#define RET     ( 0x24 << INS_OPCODE )
#define RTI     ( 0x25 << INS_OPCODE )

// Move
#define MOV     ( 0x30 << INS_OPCODE )

// Load/Store
#define LOAD    ( 0x38 << INS_OPCODE )
#define LOADW   ( 0x39 << INS_OPCODE )
#define LOADB   ( 0x3a << INS_OPCODE )
#define STR     ( 0x40 << INS_OPCODE )
#define STRW    ( 0x41 << INS_OPCODE )
#define STRB    ( 0x42 << INS_OPCODE )
#define PUSH    ( 0x43 << INS_OPCODE )
#define PUSHW   ( 0x44 << INS_OPCODE )
#define PUSHB   ( 0x45 << INS_OPCODE )
#define POP     ( 0x46 << INS_OPCODE )
#define POPW    ( 0x47 << INS_OPCODE )
#define POPB    ( 0x48 << INS_OPCODE )
#define MEMCPY  ( 0x49 << INS_OPCODE )
#define MEMSET  ( 0x4a << INS_OPCODE )  // NOTE:  not yet implemented
#define CLRSET  ( 0x4b << INS_OPCODE )
#define CLRSETV ( 0x4c << INS_OPCODE )

// I/O
#define READ    ( 0x50 << INS_OPCODE )
#define WRITE   ( 0x51 << INS_OPCODE )

// Math
#define ADD     ( 0x60 << INS_OPCODE )
#define INC     ( 0x63 << INS_OPCODE )
#define SUB     ( 0x64 << INS_OPCODE )
#define DEC     ( 0x67 << INS_OPCODE )
#define MUL     ( 0x68 << INS_OPCODE )
#define MULW    ( 0x69 << INS_OPCODE )
#define MULB    ( 0x6a << INS_OPCODE )
#define SHR     ( 0x6b << INS_OPCODE )
#define SHL     ( 0x6c << INS_OPCODE )

#endif // OPCODES
