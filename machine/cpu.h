/**
 * @file    cpu.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#ifndef CPU_H
#define CPU_H

#include "device.h"
#include "motherboard.h"

namespace machine
{

class Cpu : public Device
{
public:

    virtual ~Cpu() { }

    /**
     * Start processing CPU instructions at a place in memory
     * @param[in]   mb      Motherboard to operate on
     * @param[in]   addr    Place in memory to start processing from
     */
    virtual void start(Motherboard& mb, MemAddress addr) = 0;

protected:

    /**
     * @param[in]   mb
     * @return Main memory from Motherboard
     */
    Memory& getMemory(Motherboard& mb) { return mb.getMemory(); }
};

}   // namespace machine

#endif // CPU_H
