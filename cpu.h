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

class Cpu : public Device
{
public:

    virtual ~Cpu() { }

    /**
     * Start processing CPU instructions at a place in memory
     * @param[in]   addr    Place in memory to start processing from
     */
    virtual void start(motherboard::MemAddress addr) = 0;
};

#endif // CPU_H
