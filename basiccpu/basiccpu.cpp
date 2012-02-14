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

// declared, but not defined, in device.h
SLDECL Device* createDevice()
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
    registers[13] = &sp;
    registers[14] = &lr;
    registers[15] = &ip;

    bool halt = false;
    while (!halt && ip < static_cast<MemAddress>( memory.size() ) - 4)
    {
        MemAddress instruction = getInstruction(memory, ip);

        switch (instruction & INS_OPCODE_MASK)
        {
        case CALL:
            memory[sp -= 4] = lr;   // save current lr
            lr = ip;                // save instruction pointer to link register
            // fall through
        case JMP:
            instr_operand = instruction & 0xFFFF;
            #if CHECK_INSTR
            if (instr_operand % 4) {
                fprintf(stderr, "0x%08x:  Invalid jump length:  0x%x\n", ip, instr_operand);
                exit(1);
            }
            #endif
            ip += instr_operand - 4;    // -4 compensates for increment
            break;

        case RET:
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
            break;

        case WRITE:
            instr_operand = instruction & 0xFFFF;
            operand = getInstruction(memory, ip);
            Device::writeMb(mb, instr_operand, operand);
            break;

        case MEMCPY:
            operand = getInstruction(memory, ip);
            // copy r1 to operand, length r2
            for (uint32_t i = 0; i < static_cast<uint32_t>( r2 ); i++)
                memory[operand + i] = memory[r1 + i];

            break;

        case HALT:
            halt = true;
            break;

        default:
            fprintf(stderr, "Undefined instruction:  0x%8x\n", instruction);
            exit(1);
        }

        #if DEBUG
        printf("instr = 0x%08x\n", instruction);
        printf("r1    = 0x%08x\n", r1);
        printf("r2    = 0x%08x\n", r2);
        printf("r3    = 0x%08x\n", r2);
        printf("r4    = 0x%08x\n", r2);
        printf("r5    = 0x%08x\n", r2);
        printf("sp    = 0x%08x\n", static_cast<unsigned int>( sp ));
        printf("lr    = 0x%08x\n", static_cast<unsigned int>( lr ));
        printf("ip    = 0x%08x\n\n", static_cast<unsigned int>( ip ));
        #endif
    }
}
