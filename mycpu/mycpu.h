/**
 * @file    mycpu.h
 *
 * Matrix VM
 */

#ifndef MYCPU_H
#define MYCPU_H

#include "opcodes.h"
#include <machine/cpu.h>

namespace machine
{

class MyCpu : public Cpu
{
public:

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

private:

    // registers
    MemAddress r1;
    MemAddress r2;
};

}   // namespace machine

#endif // MYCPU_H
