/**
 * @file    basiccpu.cpp
 *
 * Matrix VM
 */

#include "basiccpu.h"
#include <dev/basicinterruptcontroller.h>

#include <stdlib.h>
#include <stdio.h>

using namespace std;
using namespace machine;

SLDECL Device* createDevice(void* args)
{
    return new BasicCpu;
}

static inline MemAddress getInstruction(vector<uint8_t>& memory, MemAddress& ip)
{
    ip += 4;
    return memory[ip-4] << 24 |
           memory[ip-3] << 16 |
           memory[ip-2] <<  8 |
           memory[ip-1] <<  0;
}

/**
 * Pushes `what` into the stack and updates the stack pointer
 * @param[in,out]   memory
 * @param[in,out]   sp      A reference to the stack pointer
 * @param[in]       what    The value to push onto the stack
 */
static inline void push(vector<uint8_t>& memory, MemAddress& sp, MemAddress what)
{
    sp -= 4;
    memory[sp + 0] = (what & 0xFF000000) >> 24;
    memory[sp + 1] = (what & 0x00FF0000) >> 16;
    memory[sp + 2] = (what & 0x0000FF00) >>  8;
    memory[sp + 3] = (what & 0x000000FF) >>  0;
}

/**
 * Pops a 32-bit value from the stack and updates the stack pointer
 * @param[in,out]   memory
 * @param[in,out]   sp      A reference to the stack pointer
 * @return  The 32-bit value from the stack
 */
static inline MemAddress pop(vector<uint8_t>& memory, MemAddress& sp)
{
    sp += 4;
    return memory[sp - 4] << 24 |
           memory[sp - 3] << 16 |
           memory[sp - 2] <<  8 |
           memory[sp - 1] <<  0;
}

/**
 * Do relative jump
 * @param[in]       offset
 * @param[in,out]   ip      Current instruction pointer
 */
static inline void reljump(int16_t offset, MemAddress& ip)
{
    #if CHECK_INSTR
    if (offset % 4) {
        fprintf(stderr, "0x%08x:  Invalid jump length:  0x%x\n", ip, offset);
        exit(1);
    }
    #endif
    ip += offset - 4;   // -4 compensates for increment
}

/**
 * Extracts the addressing mode from the instruction
 * @param[in]   instruction
 * @return The mode, masked off from the instruction
 */
static inline MemAddress getMode(MemAddress instruction)
{
    return instruction & (0x7 << INS_ADDR);
}

#if DEBUG
/**
 * Convert the mode from bits to human-readable string
 * @param[in]   mode    Mode, encoded as part of an instruction
 * @return  The string representation of the mode
 */
static const char* modeToString(MemAddress mode)
{
    return mode == IMMEDIATE ? "immediate" :
           mode == REGISTER  ? "register"  :
           0;
}
#endif

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

    // Get location to interrupt vector
    InterruptController* ic = mb.getInterruptController();
    MemAddress icVector = ic ? ic->getInterruptVectorAddress() : -1;
    // Hardcode keyboard interrupt address since we don't have store instruction yet
    // handle_keyboard is at in hello2.s 0x006ad080
    memory[icVector + 1 * 4 + 0] = 0x00;
    memory[icVector + 1 * 4 + 1] = 0x6a;
    memory[icVector + 1 * 4 + 2] = 0xD0;
    memory[icVector + 1 * 4 + 3] = 0x80;

    // Initialize sp to last memory spot.  Stack grows down
    sp = mb.getMemorySize() - 1;
    if (sp % 4) // align sp
        sp -= sp % 4;

    st = 0;
    dl = 100000;    // software cannot yet change this register

    MemAddress dummyReg;
    vector<MemAddress*> registers(MAX_REGISTERS, &dummyReg);
    registers[1]  = &r1;
    registers[2]  = &r2;
    registers[3]  = &r3;
    registers[4]  = &r4;
    registers[5]  = &r5;
    registers[6]  = &r6;
    registers[11] = &sp;
    registers[12] = &lr;
    registers[13] = &ip;
    registers[14] = &dl;
    registers[15] = &st;

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
        // Check for interrupts
        if (this->interruptsEnabled() && this->interrupts.any())
        {
            for (size_t i = 0; i < this->interrupts.size(); i++)
            {
                if (this->interrupts.test(i))
                {
                    MemAddress handler = memory[icVector + i * 4 + 0] << 24 |
                                         memory[icVector + i * 4 + 1] << 16 |
                                         memory[icVector + i * 4 + 2] <<  8 |
                                         memory[icVector + i * 4 + 3] <<  0;
                    if (handler)
                    {
                        // save current ip
                        push(memory, sp, ip);
                        // set ip to value of interrupt vector
                        ip = handler;
                        // clear this interrupt line
                        this->interrupts[i] = 0;

                        break;
                    }
                }
            }
        }

        MemAddress instruction = getInstruction(memory, ip);

        switch (instruction & INS_OPCODE_MASK)
        {
        case CMP:
            instr_mode = getMode(instruction);
            {
                MemAddress op1 = *registers[EXTRACT_REG(instruction)];
                MemAddress result;
                if (instr_mode == IMMEDIATE)
                    result = op1 - getInstruction(memory, ip);
                else if (instr_mode == REGISTER)
                    result = op1 - *registers[EXTRACT_SRC_REG(instruction)];
                else
                    /* TODO:  generate instruction fault */;

                // Room for improvement:  delay this calculation until needed;
                // then only calculate the bits that are needed.
                st = st & 0xFFFFFF00;
                if (result == 0)
                    st |= STATUS_ZERO_MASK;
                else if (result < 0)
                    st |= STATUS_NEG_MASK;
                if (result > op1)
                    st |= STATUS_OVERFLOW_MASK;
            }
            BCPU_DBGI("cmp", modeToString(instr_mode));
            break;

        case JMP:
            BCPU_DBGI("jmp", "relative");
            reljump(instruction & 0xFFFF, ip);
            break;

        case JE:
            BCPU_DBGI("je", "relative");
            if (st & STATUS_ZERO_MASK)
                reljump(instruction & 0xFFFF, ip);
            break;

        case CALL:
            BCPU_DBGI("call", "relative");
            // save current lr
            push(memory, sp, lr);
            // save instruction pointer to link register
            lr = ip;
            reljump(instruction & 0xFFFF, ip);
            break;

        case RET:
            BCPU_DBGI("ret", 0);
            // return by restoring ip from lr
            ip = lr;
            // restore previous lr
            lr = pop(memory, sp);
            break;

        case RTI:
            BCPU_DBGI("rti", 0);
            // restore ip
            ip = pop(memory, sp);
            break;

        case STI:
            BCPU_DBGI("sti", 0);
            this->st |= STATUS_INTERRUPT_MASK;
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

        case READ:
            instr_mode = getMode(instruction);
            BCPU_DBGI("read", modeToString(instr_mode));
            if (instr_mode == IMMEDIATE)
                *registers[EXTRACT_REG(instruction)] = ic ? ic->getPin(instruction & 0xFFFF) : 0;
            else
                /* TODO: generate instruction fault */;
            break;

        case WRITE:
            BCPU_DBGI("write", "immediate");
            instr_operand = instruction & 0xFFFF;
            operand = getInstruction(memory, ip);
            Device::writeMb(mb, instr_operand, operand);
            break;

        case MEMCPY:
            BCPU_DBGI("memcpy", "immediate");
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

        case IDLE:
            BCPU_DBGI("idle", 0);
            usleep(dl);
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

void BasicCpu::interrupt(unsigned int line)
{
    this->interrupts[line] = 1;
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
