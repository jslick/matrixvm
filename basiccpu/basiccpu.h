/**
 * @file    basiccpu.h
 *
 * Matrix VM
 */

#ifndef MYCPU_H
#define MYCPU_H

#include "opcodes.h"
#include <machine/cpu.h>

#include <bitset>

namespace machine
{

class BasicCpu : public Cpu
{
public:

    static const uint16_t NUM_INTERRUPT_LINES = 32;

    /**
     * @return  Name of device
     */
    std::string getName() const;

    /**
     * Start processing CPU instructions at a place in memory
     * @param[in]   mb      Motherboard to operate on
     * @param[in]   addr    Place in memory to start processing from
     */
    void start(Motherboard& mb, MemAddress addr);

    void interrupt(unsigned int line);

    /*
     * 32-bit integer guest code is pre-decoded into this structure
     */
    struct Instruction {
        #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        unsigned opcode         :8;
        unsigned destreg        :4;
        unsigned addrmode       :3;
        unsigned /* pad */      :1;
        union {
            unsigned operand    :16;
            struct {
                unsigned src1 :8;
                unsigned src2 :8;
            } __attribute__((packed)) sources;
        } __attribute__((packed));

        inline uint16_t getOperand() const {
            return ((operand << 8) & 0xFF00) | ((operand >> 8) & 0x00FF);
        }
        #else
        // NOTE:  big endian not tested!
        unsigned opcode         :8;
        unsigned addrmode       :3;
        unsigned /* pad */      :1;
        unsigned destreg        :4;
        union {
            unsigned operand    :16;
            struct {
                unsigned src1 :8;
                unsigned src2 :8;
            }  __attribute__((packed)) sources;
        } __attribute__((packed));

        inline uint16_t getOperand() const {
            return operand;
        }
        #endif
    } __attribute__((packed));

protected:

    inline bool interruptsEnabled() const
    {
        return this->st & STATUS_INTERRUPT_MASK ? true : false;
    }

    /**
     * Push all registers onto the stack
     * @param   memory
     * @param   ip
     */
    void pushRegisters(std::vector<uint8_t>& memory, MemAddress& ip);

    /**
     * Restore all registers from the stack
     * @param   memory
     * @param   ip
     */
    void restoreRegisters(std::vector<uint8_t>& memory, MemAddress& ip);

    /**
     * Update status register with result of an operation
     * @param[in]   before  Value of some register before operation
     * @param[in]   result  Value of the same register after operation
     */
    void updateStatus(MemAddress before, MemAddress result);

    void colorset(std::vector<uint8_t>& memory, MemAddress what);

    void colorsetVertical(std::vector<uint8_t>& memory, MemAddress what);

private:

    // registers
    MemAddress r1;
    MemAddress r2;
    MemAddress r3;
    MemAddress r4;
    MemAddress r5;
    MemAddress r6;
    MemAddress r7;
    MemAddress sp;
    MemAddress lr;
    MemAddress ip;
    MemAddress dl; //<! delay register
    MemAddress st;

    std::bitset<NUM_INTERRUPT_LINES> interrupts;

    #if EMULATOR_BENCHMARK
    unsigned long long numOperations;
    #endif

};

}   // namespace machine

#endif // MYCPU_H
