/**
 * @file    basiccpu.cpp
 *
 * Matrix VM
 */

#include "basiccpu.h"
#include <dev/basicinterruptcontroller.h>

#include <stdlib.h>
#include <stdio.h>
#if EMULATOR_BENCHMARK
#  include <chrono>
#endif

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

static inline BasicCpu::Instruction getInstruction(vector<uint8_t>& memory, MemAddress& ip)
{
    ip += 4;
    MemAddress bytes = getMemory32(memory, ip - 4);
    BasicCpu::Instruction* instruction = reinterpret_cast<BasicCpu::Instruction*>( &bytes );
    return *instruction;
}

static inline MemAddress getWord(vector<uint8_t>& memory, MemAddress& ip)
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
    switch (mode)
    {
    case ABSOLUTE  >> INS_ADDR:
        return "absolute";
    case RELATIVE  >> INS_ADDR:
        return "relative";
    case IMMEDIATE >> INS_ADDR:
        return "immediate";
    case REGISTER  >> INS_ADDR:
        return "register";
    case INDIRECT  >> INS_ADDR:
        return "indirect";
    default:
        return 0;
    }
}
#endif

/* public BasicCpu */

string BasicCpu::getName() const
{
    return "BasicCpu";
}

void BasicCpu::start(Motherboard& mb, MemAddress addr)
{
    if (sizeof(BasicCpu::Instruction) != sizeof(MemAddress))
        throw runtime_error("Pre-decoding error");

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
    MemAddress  operand;        // a pointer-size operand following an instruction
    MemAddress  before;         // value before a calculation
    MemAddress  result = 0;     // value after  a calculation
    MemAddress* dest_reg;       // destination register

    #if EMULATOR_BENCHMARK
    typedef chrono::high_resolution_clock Clock;
    typedef std::chrono::microseconds microseconds;

    Clock::time_point t0 = Clock::now();
    unsigned long long numInstructions = 0;
    unsigned long long numInterrupts   = 0;
    #endif

    bool halt = false;
    while (!halt && ip < static_cast<MemAddress>( memory.size() ) - 4)
    {
        // Check for interrupts
        if (this->interruptsEnabled() && this->interrupts.any())
        {
            #if EMULATOR_BENCHMARK
            numInterrupts++;
            #endif
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

        Instruction instruction = getInstruction(memory, ip);
        #if EMULATOR_BENCHMARK
        numInstructions++;
        #endif

        #define CONVERT_OPCODE(OPCODE)  ( OPCODE >> INS_OPCODE )
        #define CONVERT_MODE(MODE)      ( MODE   >> INS_ADDR )

        switch (instruction.opcode)
        {
        case CONVERT_OPCODE(CMP):
            BCPU_DBGI("cmp", modeToString(instruction.addrmode));

            before = *registers[instruction.destreg];
            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                result = before - getWord(memory, ip);
            else if (instruction.addrmode == CONVERT_MODE(REGISTER))
                result = before - *registers[instruction.sources.src2];
            else
                /* TODO:  generate instruction fault */;

            break;

        case CONVERT_OPCODE(TST):
            BCPU_DBGI("tst", 0);
            result = before = *registers[instruction.destreg];
            break;

        case CONVERT_OPCODE(JMP):
            BCPU_DBGI("jmp", "relative");
            reljump(instruction.operand, ip);
            break;

        case CONVERT_OPCODE(JE):
            BCPU_DBGI("je", "relative");
            if (result == 0)
                reljump(instruction.operand, ip);
            break;

        case CONVERT_OPCODE(JNE):
            BCPU_DBGI("jne", "relative");
            if (result != 0)
                reljump(instruction.operand, ip);
            break;

        case CONVERT_OPCODE(JGE):
            BCPU_DBGI("jge", "relative");
            if (result >= 0)
                reljump(instruction.operand, ip);
            break;

        case CONVERT_OPCODE(JG):
            BCPU_DBGI("jg", "relative");
            if (result > 0)
                reljump(instruction.operand, ip);
            break;

        case CONVERT_OPCODE(JLE):
            BCPU_DBGI("jle", "relative");
            if (result <= 0)
                reljump(instruction.operand, ip);
            break;

        case CONVERT_OPCODE(JL):
            BCPU_DBGI("jl", "relative");
            if (result < 0)
                reljump(instruction.operand, ip);
            break;

        case CONVERT_OPCODE(CALL):
            BCPU_DBGI("call", "relative");
            // save current lr
            push(memory, sp, lr);
            // save instruction pointer to link register
            lr = ip;
            reljump(instruction.operand, ip);
            break;

        case CONVERT_OPCODE(RET):
            BCPU_DBGI("ret", 0);
            // return by restoring ip from lr
            ip = lr;
            // restore previous lr
            lr = pop(memory, sp);
            break;

        case CONVERT_OPCODE(RTI):
            BCPU_DBGI("rti", 0);
            // restore registers
            restoreRegisters(memory, ip);
            break;

        case CONVERT_OPCODE(CLI):
            BCPU_DBGI("cli", 0);
            this->st &= ~STATUS_INTERRUPT_MASK;
            break;

        case CONVERT_OPCODE(STI):
            BCPU_DBGI("sti", 0);
            this->st |= STATUS_INTERRUPT_MASK;
            break;

        case CONVERT_OPCODE(RSTR):
            BCPU_DBGI("rstr", "register");
            operand = *registers[instruction.destreg];
            for (unsigned int i = 1; i < registers.size(); i++)
                *registers[i] = getMemory32(memory, operand + (i-1) * 4);
            break;

        case CONVERT_OPCODE(MOV):
            BCPU_DBGI("mov", modeToString(instruction.addrmode));
            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                *registers[instruction.destreg] = getWord(memory, ip);
            else if (instruction.addrmode == CONVERT_MODE(REGISTER))
                *registers[instruction.destreg] = *registers[instruction.sources.src2];
            else
                /* TODO:  generate instruction fault */;
            break;

        case CONVERT_OPCODE(LOAD):
            BCPU_DBGI("load", modeToString(instruction.addrmode));
            if (instruction.addrmode == CONVERT_MODE(ABSOLUTE))
                *registers[instruction.destreg] = getMemory32(memory, getWord(memory, ip));
            else if (instruction.addrmode == CONVERT_MODE(INDIRECT))
                *registers[instruction.destreg] = getMemory32(memory, *registers[instruction.sources.src2]);
            break;

        case CONVERT_OPCODE(LOADB):
            BCPU_DBGI("loadb", modeToString(instruction.addrmode));
            if (instruction.addrmode == CONVERT_MODE(ABSOLUTE))
                *registers[instruction.destreg] = memory[getWord(memory, ip)];
            else if (instruction.addrmode == CONVERT_MODE(INDIRECT))
                *registers[instruction.destreg] = memory[*registers[instruction.sources.src2]];
            break;

        case CONVERT_OPCODE(STR):
            BCPU_DBGI("str", modeToString(instruction.addrmode));
            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                updateMemory32(memory, *registers[instruction.destreg], getWord(memory, ip));
            else if (instruction.addrmode == CONVERT_MODE(REGISTER))
                updateMemory32(memory, *registers[instruction.destreg], *registers[instruction.sources.src2]);
            else
                /* TODO:  generate instruction fault */;
            break;

        case CONVERT_OPCODE(STRB):
            BCPU_DBGI("strb", modeToString(instruction.addrmode));
            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                memory[*registers[instruction.destreg]] = instruction.operand;
            else if (instruction.addrmode == CONVERT_MODE(REGISTER))
                memory[*registers[instruction.destreg]] = *registers[instruction.sources.src2];
            else
                /* TODO:  generate instruction fault */;
            break;

        case CONVERT_OPCODE(PUSH):
            BCPU_DBGI("push", modeToString(instruction.addrmode));
            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                push(memory, sp, getWord(memory, ip));
            else if (instruction.addrmode == CONVERT_MODE(REGISTER))
                push(memory, sp, *registers[instruction.sources.src2]);
            else
                /* TODO:  generate instruction fault */;

            break;

        case CONVERT_OPCODE(POP):
            BCPU_DBGI("pop", 0);
            *registers[instruction.destreg] = pop(memory, sp);
            break;

        case CONVERT_OPCODE(PUSHW):
            BCPU_DBGI("pushw", modeToString(instruction.addrmode));
            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                push16(memory, sp, instruction.operand);
            else if (instruction.addrmode == CONVERT_MODE(REGISTER))
                push16(memory, sp, *registers[instruction.sources.src2]);
            else
                /* TODO:  generate instruction fault */;

            break;

        case CONVERT_OPCODE(READ):
            BCPU_DBGI("read", modeToString(instruction.addrmode));
            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                *registers[instruction.destreg] = ic ? ic->getPin(instruction.operand) : 0;
            else
                /* TODO: generate instruction fault */;
            break;

        case CONVERT_OPCODE(WRITE):
            BCPU_DBGI("write", "immediate");
            Device::writeMb(mb, instruction.operand, getWord(memory, ip));
            break;

        case CONVERT_OPCODE(MEMCPY):
            BCPU_DBGI("memcpy", "register");
            {
                MemAddress* destReg = registers[instruction.destreg];
                MemAddress* srcReg  = registers[instruction.sources.src1];
                MemAddress* lenReg  = registers[instruction.sources.src2];
                for (int i = 0; i < *lenReg; i++)
                    memory[*destReg + i] = memory[*srcReg + i];
            }
            break;

        case CONVERT_OPCODE(MEMSET):
            BCPU_DBGI("memset", "register");
            {
                MemAddress* destReg = registers[instruction.destreg];
                MemAddress* srcReg  = registers[instruction.sources.src1];
                MemAddress* lenReg  = registers[instruction.sources.src2];
                for (int i = 0; i < *lenReg; i++)
                    memory[*destReg + i] = *srcReg;
            }
            break;

        case CONVERT_OPCODE(CLRSET):
            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                this->colorset(memory, getWord(memory, ip));
            else if (instruction.addrmode == CONVERT_MODE(REGISTER)) // this mode is untested
                this->colorset(memory, *registers[instruction.sources.src2]);
            else
                /* TODO:  generate instruction fault */;
            BCPU_DBGI("clrset", modeToString(instruction.addrmode));
            break;

        case CONVERT_OPCODE(CLRSETV):
            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                this->colorsetVertical(memory, getWord(memory, ip));
            else if (instruction.addrmode == CONVERT_MODE(REGISTER)) // this mode is untested
                this->colorsetVertical(memory, *registers[instruction.sources.src2]);
            else
                /* TODO:  generate instruction fault */;
            BCPU_DBGI("clrsetv", modeToString(instruction.addrmode));
            break;

        case CONVERT_OPCODE(HALT):
            BCPU_DBGI("halt", 0);
            halt = true;
            break;

        case CONVERT_OPCODE(IDLE):
            BCPU_DBGI("idle", 0);
            usleep(dl);
            break;

        case CONVERT_OPCODE(ADD):
            BCPU_DBGI("add", modeToString(instruction.addrmode));

            dest_reg = registers[instruction.destreg];
            before = *dest_reg;

            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                result = *dest_reg += getWord(memory, ip);
            else if (instruction.addrmode == CONVERT_MODE(REGISTER))
                result = *dest_reg += *registers[instruction.sources.src2];
            else
                /* TODO:  generate instruction fault */;
            break;

        case CONVERT_OPCODE(INC):
            BCPU_DBGI("inc", 0);
            dest_reg = registers[instruction.destreg];
            before = *dest_reg;

            result = ++(*dest_reg);

            break;

        case CONVERT_OPCODE(DEC):
            BCPU_DBGI("dec", 0);
            dest_reg = registers[instruction.destreg];
            before = *dest_reg;

            result = --(*dest_reg);

            break;

        case CONVERT_OPCODE(SUB):
            BCPU_DBGI("sub", modeToString(instruction.addrmode));

            dest_reg = registers[instruction.destreg];
            before = *dest_reg;

            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                result = *dest_reg -= getWord(memory, ip);
            else if (instruction.addrmode == CONVERT_MODE(REGISTER))
                result = *dest_reg -= *registers[instruction.sources.src2];
            else
                /* TODO:  generate instruction fault */;
            break;

        case CONVERT_OPCODE(MUL):
            BCPU_DBGI("mul", modeToString(instruction.addrmode));

            dest_reg = registers[instruction.destreg];
            before = *dest_reg;

            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                result = *dest_reg *= getWord(memory, ip);
            else if (instruction.addrmode == CONVERT_MODE(REGISTER))
                result = *dest_reg *= *registers[instruction.sources.src2];
            else
                /* TODO:  generate instruction fault */;
            break;

        case CONVERT_OPCODE(MULW):
            BCPU_DBGI("mulw", "immediate");

            dest_reg = registers[instruction.destreg];
            before = *dest_reg;

            result = *dest_reg *= instruction.operand;
            break;

        case CONVERT_OPCODE(AND):
            BCPU_DBGI("and", modeToString(instruction.addrmode));

            dest_reg = registers[instruction.destreg];
            before = *dest_reg;

            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                result = *dest_reg &= getWord(memory, ip);
            else if (instruction.addrmode == CONVERT_MODE(REGISTER))
                result = *dest_reg &= *registers[instruction.sources.src2];
            else
                /* TODO:  generate instruction fault */;
            break;

        case CONVERT_OPCODE(SHR):
            BCPU_DBGI("shr", modeToString(instruction.addrmode));

            dest_reg = registers[instruction.destreg];
            before = *dest_reg;

            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                result = *dest_reg = static_cast<uint32_t>( *dest_reg ) >> instruction.sources.src2;
            else if (instruction.addrmode == CONVERT_MODE(REGISTER))
                result = *dest_reg = static_cast<uint32_t>( *dest_reg ) >> *registers[instruction.sources.src2];
            else
                /* TODO:  generate instruction fault */;
            break;

        case CONVERT_OPCODE(SHL):
            BCPU_DBGI("shl", modeToString(instruction.addrmode));

            dest_reg = registers[instruction.destreg];
            before = *dest_reg;

            if (instruction.addrmode == CONVERT_MODE(IMMEDIATE))
                result = *dest_reg = static_cast<uint32_t>( *dest_reg ) << instruction.sources.src2;
            else if (instruction.addrmode == CONVERT_MODE(REGISTER))
                result = *dest_reg = static_cast<uint32_t>( *dest_reg ) << *registers[instruction.sources.src2];
            else
                /* TODO:  generate instruction fault */;
            break;

        default:
            BCPU_DBGI("undefined", 0);
            fprintf(stderr, "Undefined instruction:  0x%08x\n", instruction.opcode);
            exit(1);
        }

        #if DEBUG
        MemAddress* instructionCode = reinterpret_cast<MemAddress*>( &instruction );
        printf("instr = 0x%08x", *instructionCode);
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
        printf("r7    = 0x%08x\n", r7);
        printf("sp    = 0x%08x\n", static_cast<unsigned int>( sp ));
        printf("lr    = 0x%08x\n", static_cast<unsigned int>( lr ));
        printf("ip    = 0x%08x\n\n", static_cast<unsigned int>( ip ));
        #endif
    }

    #if EMULATOR_BENCHMARK
    Clock::time_point endtime = Clock::now();
    microseconds us = std::chrono::duration_cast<microseconds>(endtime - t0);
    double mhz = static_cast<double>( numInstructions ) / us.count();

    printf("Number of instructions executed:  %12lld\n", numInstructions);
    printf("Number of microseconds:           %12ld\n", us.count());
    printf("Calculated MHz:                   %12.3f\n", mhz);
    printf("Number of interrupts:             %12lld\n", numInterrupts);
    #endif
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
    push(memory, sp, r7);
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
    r7  = pop(memory, sp);
    sp += 4 * 3;
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
