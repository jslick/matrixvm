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

static inline MemAddress getMemory32(vector<uint8_t>& memory, MemAddress location)
{
    return memory[location + 0] << 24 |
           memory[location + 1] << 16 |
           memory[location + 2] <<  8 |
           memory[location + 3] <<  0;
}

/**
 * Update memory with a 32-bit value
 * @param[in,out]   memory
 * @param[in]       location    Location to write to
 * @param[in]       what        32-bit value to write to memory
 */
static inline void updateMemory32(
        vector<uint8_t>&    memory,
        MemAddress          location,
        MemAddress          what)
{
    memory[location + 0] = (what & 0xFF000000) >> 24;
    memory[location + 1] = (what & 0x00FF0000) >> 16;
    memory[location + 2] = (what & 0x0000FF00) >>  8;
    memory[location + 3] = (what & 0x000000FF) >>  0;
}

/**
 * Update memory with a 16-bit value
 * @param[in,out]   memory
 * @param[in]       location    Location to write to
 * @param[in]       what        32-bit value to write to memory
 */
static inline void updateMemory16(
        vector<uint8_t>&    memory,
        MemAddress          location,
        MemAddress          what)
{
    memory[location + 0] = (what & 0xFF00) >> 8;
    memory[location + 1] = (what & 0x00FF) >> 0;
}

static inline MemAddress getInstruction(vector<uint8_t>& memory, MemAddress& ip)
{
    ip += 4;
    return getMemory32(memory, ip - 4);
}

/**
 * Pushes `what` into the stack and updates the stack pointer
 * @param[in,out]   memory
 * @param[in,out]   sp      A reference to the stack pointer
 * @param[in]       what    The value to push onto the stack
 */
static inline void push(vector<uint8_t>& memory, MemAddress& sp, MemAddress what)
{
    updateMemory32(memory, sp -= 4, what);
}

/**
 * Pushes 2 bytes `what` into the stack and updates the stack pointer
 * @param[in,out]   memory
 * @param[in,out]   sp      A reference to the stack pointer
 * @param[in]       what    The value to push onto the stack
 */
static inline void push16(vector<uint8_t>& memory, MemAddress& sp, uint16_t what)
{
    updateMemory16(memory, sp -= 2, what);
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
    return getMemory32(memory, sp - 4);
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

void BasicCpu::start(Motherboard& mb, MemAddress addr)
{
    this->ip = addr;

    vector<uint8_t>& memory = Device::getMemory(mb);

    // Get location to interrupt vector
    InterruptController* ic = mb.getInterruptController();
    MemAddress icVector = ic ? ic->getInterruptVectorAddress() : -1;

    // Initialize sp to last memory spot.  Stack grows down
    sp = mb.getMemorySize() - 1;
    if (sp % 4) // align sp
        sp -= sp % 4;

    st = 0;
    dl = 100000;

    MemAddress dummyReg;
    vector<MemAddress*> registers(MAX_REGISTERS, &dummyReg);
    registers[1]  = &r1;
    registers[2]  = &r2;
    registers[3]  = &r3;
    registers[4]  = &r4;
    registers[5]  = &r5;
    registers[6]  = &r6;
    registers[7]  = &r7;
    registers[SP >> INS_REG] = &sp;
    registers[LR >> INS_REG] = &lr;
    registers[IP >> INS_REG] = &ip;
    registers[DL >> INS_REG] = &dl;
    registers[ST >> INS_REG] = &st;

    #if DEBUG
    const char* str_opcode  = 0;
    const char* str_mode    = 0;
    #define BCPU_DBGI(dbg_opcode, dbg_mode) \
            if (!str_opcode)    str_opcode  = dbg_opcode; \
            if (!str_mode)      str_mode    = dbg_mode;
    #else
    #define BCPU_DBGI(dbg_opcode, dbg_mode)
    #endif

    /* Loop variables */
    MemAddress  instr_mode;     // mode of an instruction
    int16_t     instr_operand;  // an operand part of an instruction
    MemAddress  operand;        // a pointer-size operand following an instruction
    MemAddress  before;         // value before a calculation
    MemAddress  result;         // value after  a calculation
    MemAddress* dest_reg;       // destination register

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
                    MemAddress handler = getMemory32(memory, icVector + i * 4);
                    if (handler)
                    {
                        // save current registers
                        // TODO:  commit status
                        pushRegisters(memory, ip);
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
            BCPU_DBGI("cmp", modeToString(instr_mode));

            before = *registers[EXTRACT_REG(instruction)];
            if (instr_mode == IMMEDIATE)
                result = before - getInstruction(memory, ip);
            else if (instr_mode == REGISTER)
                result = before - *registers[EXTRACT_SRC_REG(instruction)];
            else
                /* TODO:  generate instruction fault */;

            break;

        case TST:
            BCPU_DBGI("tst", 0);
            result = before = *registers[EXTRACT_REG(instruction)];
            break;

        case JMP:
            BCPU_DBGI("jmp", "relative");
            reljump(instruction & 0xFFFF, ip);
            break;

        case JE:
            BCPU_DBGI("je", "relative");
            if (result == 0)
                reljump(instruction & 0xFFFF, ip);
            break;

        case JNE:
            BCPU_DBGI("jne", "relative");
            if (result != 0)
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
            // restore registers
            restoreRegisters(memory, ip);
            break;

        case CLI:
            BCPU_DBGI("cli", 0);
            this->st &= ~STATUS_INTERRUPT_MASK;
            break;

        case STI:
            BCPU_DBGI("sti", 0);
            this->st |= STATUS_INTERRUPT_MASK;
            break;

        case RSTR:
            BCPU_DBGI("rstr", "register");
            operand = *registers[EXTRACT_REG(instruction)];
            for (unsigned int i = 1; i < registers.size(); i++)
                *registers[i] = getMemory32(memory, operand + (i-1) * 4);
            break;

        case MOV:
            instr_mode = getMode(instruction);
            BCPU_DBGI("mov", modeToString(instr_mode));
            if (instr_mode == IMMEDIATE)
                *registers[EXTRACT_REG(instruction)] = getInstruction(memory, ip);
            else if (instr_mode == REGISTER)
                *registers[EXTRACT_REG(instruction)] = *registers[EXTRACT_SRC_REG(instruction)];
            else
                /* TODO:  generate instruction fault */;
            break;

        case LOAD:
            instr_mode = getMode(instruction);
            BCPU_DBGI("load", modeToString(instr_mode));
            if (instr_mode == ABSOLUTE)
                *registers[EXTRACT_REG(instruction)] = getMemory32(memory, getInstruction(memory, ip));
            else if (instr_mode == INDIRECT)
                *registers[EXTRACT_REG(instruction)] = getMemory32(memory, *registers[EXTRACT_SRC_REG(instruction)]);
            break;

        case LOADB:
            instr_mode = getMode(instruction);
            BCPU_DBGI("loadb", modeToString(instr_mode));
            if (instr_mode == ABSOLUTE)
                *registers[EXTRACT_REG(instruction)] = memory[getInstruction(memory, ip)];
            else if (instr_mode == INDIRECT)
                *registers[EXTRACT_REG(instruction)] = memory[*registers[EXTRACT_SRC_REG(instruction)]];
            break;

        case STR:
            instr_mode = getMode(instruction);
            BCPU_DBGI("str", modeToString(instr_mode));
            if (instr_mode == IMMEDIATE)
                updateMemory32(memory, *registers[EXTRACT_REG(instruction)], getInstruction(memory, ip));
            else if (instr_mode == REGISTER)
                updateMemory32(memory, *registers[EXTRACT_REG(instruction)], *registers[EXTRACT_SRC_REG(instruction)]);
            else
                /* TODO:  generate instruction fault */;
            break;

        case STRB:
            instr_mode = getMode(instruction);
            BCPU_DBGI("strb", modeToString(instr_mode));
            if (instr_mode == IMMEDIATE)
                memory[*registers[EXTRACT_REG(instruction)]] = instruction & 0xFFFF;
            else if (instr_mode == REGISTER)
                memory[*registers[EXTRACT_REG(instruction)]] = *registers[EXTRACT_SRC_REG(instruction)];
            else
                /* TODO:  generate instruction fault */;
            break;

        case PUSH:
            instr_mode = getMode(instruction);
            BCPU_DBGI("push", modeToString(instr_mode));
            if (instr_mode == IMMEDIATE)
                push(memory, sp, getInstruction(memory, ip));
            else if (instr_mode == REGISTER)
                push(memory, sp, *registers[EXTRACT_SRC_REG(instruction)]);
            else
                /* TODO:  generate instruction fault */;

            break;

        case POP:
            BCPU_DBGI("pop", 0);
            *registers[EXTRACT_REG(instruction)] = pop(memory, sp);
            break;

        case PUSHW:
            instr_mode = getMode(instruction);
            BCPU_DBGI("pushw", modeToString(instr_mode));
            if (instr_mode == IMMEDIATE)
                push16(memory, sp, instruction & 0xFFFF);
            else if (instr_mode == REGISTER)
                push16(memory, sp, *registers[EXTRACT_SRC_REG(instruction)]);
            else
                /* TODO:  generate instruction fault */;

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
            BCPU_DBGI("memcpy", "register");
            {
                MemAddress* destReg = registers[EXTRACT_REG(instruction)];
                MemAddress* srcReg  = registers[(instruction & 0xFF00) >> 8];
                MemAddress* lenReg  = registers[EXTRACT_SRC_REG(instruction)];
                for (int i = 0; i < *lenReg; i++)
                    memory[*destReg + i] = memory[*srcReg + i];
            }
            break;

        case MEMSET:
            BCPU_DBGI("memset", "register");
            {
                MemAddress* destReg = registers[EXTRACT_REG(instruction)];
                MemAddress* srcReg  = registers[(instruction & 0xFF00) >> 8];
                MemAddress* lenReg  = registers[EXTRACT_SRC_REG(instruction)];
                for (int i = 0; i < *lenReg; i++)
                    memory[*destReg + i] = *srcReg;
            }
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
            BCPU_DBGI("add", modeToString(instr_mode));

            dest_reg = registers[EXTRACT_REG(instruction)];
            before = *dest_reg;

            if (instr_mode == IMMEDIATE)
                result = *dest_reg += getInstruction(memory, ip);
            else if (instr_mode == REGISTER)
                result = *dest_reg += *registers[EXTRACT_SRC_REG(instruction)];
            else
                /* TODO:  generate instruction fault */;
            break;

        case INC:
            BCPU_DBGI("inc", 0);
            dest_reg = registers[EXTRACT_REG(instruction)];
            before = *dest_reg;

            result = ++(*dest_reg);

            break;

        case DEC:
            BCPU_DBGI("dec", 0);
            dest_reg = registers[EXTRACT_REG(instruction)];
            before = *dest_reg;

            result = --(*dest_reg);

            break;

        case SUB:
            instr_mode = getMode(instruction);
            BCPU_DBGI("sub", modeToString(instr_mode));

            dest_reg = registers[EXTRACT_REG(instruction)];
            before = *dest_reg;

            if (instr_mode == IMMEDIATE)
                result = *dest_reg -= getInstruction(memory, ip);
            else if (instr_mode == REGISTER)
                result = *dest_reg -= *registers[EXTRACT_SRC_REG(instruction)];
            else
                /* TODO:  generate instruction fault */;
            break;

        case MUL:
            instr_mode = getMode(instruction);
            BCPU_DBGI("mul", modeToString(instr_mode));

            dest_reg = registers[EXTRACT_REG(instruction)];
            before = *dest_reg;

            if (instr_mode == IMMEDIATE)
                result = *dest_reg *= getInstruction(memory, ip);
            else if (instr_mode == REGISTER)
                result = *dest_reg *= *registers[EXTRACT_SRC_REG(instruction)];
            else
                /* TODO:  generate instruction fault */;
            break;

        case MULW:
            BCPU_DBGI("mulw", "immediate");

            dest_reg = registers[EXTRACT_REG(instruction)];
            before = *dest_reg;

            instr_operand = instruction & 0xFFFF;
            result = *dest_reg *= instr_operand;
            break;

        case SHR:
            instr_mode = getMode(instruction);
            BCPU_DBGI("shr", modeToString(instr_mode));

            dest_reg = registers[EXTRACT_REG(instruction)];
            before = *dest_reg;

            if (instr_mode == IMMEDIATE)
                result = *dest_reg >>= instruction & 0x3F;
            else if (instr_mode == REGISTER)
                result = *dest_reg >>= *registers[EXTRACT_SRC_REG(instruction)];
            else
                /* TODO:  generate instruction fault */;
            break;

        case SHL:
            instr_mode = getMode(instruction);
            BCPU_DBGI("shl", modeToString(instr_mode));

            dest_reg = registers[EXTRACT_REG(instruction)];
            before = *dest_reg;

            if (instr_mode == IMMEDIATE)
                result = *dest_reg <<= instruction & 0x3F;
            else if (instr_mode == REGISTER)
                result = *dest_reg <<= *registers[EXTRACT_SRC_REG(instruction)];
            else
                /* TODO:  generate instruction fault */;
            break;

        default:
            BCPU_DBGI("undefined", 0);
            fprintf(stderr, "Undefined instruction:  0x%08x\n", instruction);
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

void BasicCpu::pushRegisters(std::vector<uint8_t>& memory, MemAddress& ip)
{
    push(memory, sp, st);
    push(memory, sp, dl);
    push(memory, sp, ip);
    push(memory, sp, lr);
    push(memory, sp, sp);
    push(memory, sp, 0);
    push(memory, sp, 0);
    push(memory, sp, 0);
    push(memory, sp, 0);
    push(memory, sp, r6);
    push(memory, sp, r5);
    push(memory, sp, r4);
    push(memory, sp, r3);
    push(memory, sp, r2);
    push(memory, sp, r1);
}

void BasicCpu::restoreRegisters(std::vector<uint8_t>& memory, MemAddress& ip)
{
    MemAddress restoredSp;

    r1  = pop(memory, sp);
    r2  = pop(memory, sp);
    r3  = pop(memory, sp);
    r4  = pop(memory, sp);
    r5  = pop(memory, sp);
    r6  = pop(memory, sp);
    sp += 4 * 4;
    restoredSp = pop(memory, sp);
    lr  = pop(memory, sp);
    ip  = pop(memory, sp);
    dl  = pop(memory, sp);
    st  = pop(memory, sp);
    sp = restoredSp;
}

void BasicCpu::updateStatus(MemAddress before, MemAddress result)
{
    st = st & 0xFFFFFF00;
    if (result == 0)
        st |= STATUS_ZERO_MASK;
    else if (result < 0)
        st |= STATUS_NEG_MASK;
    if (result > before)
        st |= STATUS_OVERFLOW_MASK;
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
