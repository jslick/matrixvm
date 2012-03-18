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

protected:

    inline bool interruptsEnabled() const
    {
        return this->st & STATUS_INTERRUPT_MASK ? true : false;
    }

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
    MemAddress sp;
    MemAddress lr;
    MemAddress st;
    MemAddress dl; //<! delay register

    std::bitset<NUM_INTERRUPT_LINES> interrupts;

};

}   // namespace machine

#endif // MYCPU_H
