/**
 * @file    basiccpu.cpp
 *
 * Matrix VM
 */

#include "basiccpu.h"

#include <stdlib.h>
#include <stdio.h>

using namespace std;
using namespace machine;

inline MemAddress getInstruction(vector<uint8_t>& memory, MemAddress& ip)
{
    ip += 4;
    return memory[ip-4] << 24 |
           memory[ip-3] << 16 |
           memory[ip-2] <<  8 |
           memory[ip-1] <<  0;
}

inline MemAddress getMode(MemAddress instruction)
{
    return instruction & (0x7 << INS_ADDR);
}

#if DEBUG
const char* modeToString(MemAddress mode)
{
    return mode == IMMEDIATE ? "immediate" :
           mode == REGISTER  ? "register"  :
           0;
}
#endif

// declared, but not defined, in device.h
SLDECL Device* createDevice(void* args)
{
    return new BasicCpu;
}

/* public BasicCpu */

string BasicCpu::getName() const
{
    return "BasicCpu";
}

void BasicCpu::start(Motherboard& mb, MemAddress ip)
{
    vector<uint8_t>& memory = Device::getMemory(mb);

    /* Loop variables */
    MemAddress  instr_mode;     // mode of an instruction
    int16_t     instr_operand;  // an operand part of an instruction
    MemAddress  operand;        // a pointer-size operand following an instruction

    // Initialize sp to last memory spot.  Stack grows down
    sp = mb.getMemorySize() - 1;
    if (sp % 4) // align sp
        sp -= sp % 4;

    MemAddress dummyReg;
    vector<MemAddress*> registers(MAX_REGISTERS, &dummyReg);
    registers[1]  = &r1;
    registers[2]  = &r2;
    registers[3]  = &r3;
    registers[4]  = &r4;
    registers[5]  = &r5;
    registers[6]  = &r6;
    registers[13] = &sp;
    registers[14] = &lr;
    registers[15] = &ip;

    #if DEBUG
    const char* str_opcode  = 0;
    const char* str_mode    = 0;
    #define BCPU_DBGI(dbg_opcode, dbg_mode) \
            if (!str_opcode)    str_opcode  = dbg_opcode; \
            if (!str_mode)      str_mode    = dbg_mode;
    #else
    #define BCPU_DBGI(dbg_opcode, dbg_mode)
    #endif

    bool halt = false;
    while (!halt && ip < static_cast<MemAddress>( memory.size() ) - 4)
    {
        MemAddress instruction = getInstruction(memory, ip);

        switch (instruction & INS_OPCODE_MASK)
        {
        case CALL:
            BCPU_DBGI("call", "relative");
            memory[sp -= 4] = lr;   // save current lr
            lr = ip;                // save instruction pointer to link register
            // fall through
        case JMP:
            instr_operand = instruction & 0xFFFF;
            BCPU_DBGI("jmp", "relative");
            #if CHECK_INSTR
            if (instr_operand % 4) {
                fprintf(stderr, "0x%08x:  Invalid jump length:  0x%x\n", ip, instr_operand);
                exit(1);
            }
            #endif
            ip += instr_operand - 4;    // -4 compensates for increment
            break;

        case RET:
            BCPU_DBGI("ret", 0);
            ip = lr;            // return by restoring ip from lr
            lr = memory[sp];    // restore previous lr
            sp += 4;            // --^
            break;

        case MOV:
            instr_mode = getMode(instruction);
            if (instr_mode == IMMEDIATE)
                *registers[EXTRACT_REG(instruction)] = getInstruction(memory, ip);
            else if (instr_mode == REGISTER)
                *registers[EXTRACT_REG(instruction)] = *registers[EXTRACT_SRC_REG(instruction)];
            else
                /* TODO:  generate instruction fault */;
            BCPU_DBGI("mov", modeToString(instr_mode));
            break;

        case WRITE:
            BCPU_DBGI("write", "immediate");
            instr_operand = instruction & 0xFFFF;
            operand = getInstruction(memory, ip);
            Device::writeMb(mb, instr_operand, operand);
            break;

        case MEMCPY:
            BCPU_DBGI("memcpu", "immediate");
            operand = getInstruction(memory, ip);
            // copy r1 to operand, length r2
            for (uint32_t i = 0; i < static_cast<uint32_t>( r2 ); i++)
                memory[operand + i] = memory[r1 + i];

            break;

        case CLRSET:
            instr_mode = getMode(instruction);
            if (instr_mode == IMMEDIATE)
                this->colorset(memory, getInstruction(memory, ip));
            else if (instr_mode == REGISTER) // this mode is untested
                this->colorset(memory, *registers[EXTRACT_SRC_REG(instruction)]);
            else
                /* TODO:  generate instruction fault */;
            BCPU_DBGI("clrset", modeToString(instr_mode));
            break;

        case CLRSETV:
            instr_mode = getMode(instruction);
            if (instr_mode == IMMEDIATE)
                this->colorsetVertical(memory, getInstruction(memory, ip));
            else if (instr_mode == REGISTER) // this mode is untested
                this->colorsetVertical(memory, *registers[EXTRACT_SRC_REG(instruction)]);
            else
                /* TODO:  generate instruction fault */;
            BCPU_DBGI("clrsetv", modeToString(instr_mode));
            break;

        case HALT:
            BCPU_DBGI("halt", 0);
            halt = true;
            break;

        case ADD:
            instr_mode = getMode(instruction);
            if (instr_mode == IMMEDIATE)
                *registers[EXTRACT_REG(instruction)] += getInstruction(memory, ip);
            else if (instr_mode == REGISTER)
                *registers[EXTRACT_REG(instruction)] += *registers[EXTRACT_SRC_REG(instruction)];
            else
                /* TODO:  generate instruction fault */;
            BCPU_DBGI("add", modeToString(instr_mode));
            break;

        case MUL:
            instr_mode = getMode(instruction);
            if (instr_mode == IMMEDIATE)
                *registers[EXTRACT_REG(instruction)] *= getInstruction(memory, ip);
            else if (instr_mode == REGISTER)
                *registers[EXTRACT_REG(instruction)] *= *registers[EXTRACT_SRC_REG(instruction)];
            else
                /* TODO:  generate instruction fault */;
            BCPU_DBGI("mul", modeToString(instr_mode));
            break;

        case MULW:
            BCPU_DBGI("mulw", "immediate");
            instr_operand = instruction & 0xFFFF;
            *registers[EXTRACT_REG(instruction)] *= instr_operand;
            break;

        default:
            BCPU_DBGI("undefined", 0);
            fprintf(stderr, "Undefined instruction:  0x%8x\n", instruction);
            exit(1);
        }

        #if DEBUG
        printf("instr = 0x%08x", instruction);
        if (str_opcode)
        {
            printf(" (%s", str_opcode);
            if (str_mode)
                printf(", %s)", str_mode);
            else
                putchar(')');
        }
        putchar('\n');
        // reset instruction debug strings
        str_opcode  = 0;
        str_mode    = 0;
        printf("r1    = 0x%08x\n", r1);
        printf("r2    = 0x%08x\n", r2);
        printf("r3    = 0x%08x\n", r3);
        printf("r4    = 0x%08x\n", r4);
        printf("r5    = 0x%08x\n", r5);
        printf("r6    = 0x%08x\n", r6);
        printf("sp    = 0x%08x\n", static_cast<unsigned int>( sp ));
        printf("lr    = 0x%08x\n", static_cast<unsigned int>( lr ));
        printf("ip    = 0x%08x\n\n", static_cast<unsigned int>( ip ));
        #endif
    }
}

void BasicCpu::colorset(std::vector<uint8_t>& memory, MemAddress what)
{
    uint8_t red   = (what & 0xFF0000) >> (4 * 4);
    uint8_t green = (what & 0x00FF00) >> (2 * 4);
    uint8_t blue  = (what & 0x0000FF) >> 0;

    for (MemAddress i = r1; i < r1 + r2 * 3; i += 3)
    {
        memory[i+0] = red;
        memory[i+1] = green;
        memory[i+2] = blue;
    }
}

void BasicCpu::colorsetVertical(std::vector<uint8_t>& memory, MemAddress what)
{
    // r1 = start address
    // r2 = skip interval
    // r3 = length

    uint8_t red   = (what & 0xFF0000) >> (4 * 4);
    uint8_t green = (what & 0x00FF00) >> (2 * 4);
    uint8_t blue  = (what & 0x0000FF) >> 0;

    for (MemAddress i = r1; i < r1 + r2 * r3 * 3; i += r2 * 3)
    {
        memory[i+0] = red;
        memory[i+1] = green;
        memory[i+2] = blue;
    }
}
