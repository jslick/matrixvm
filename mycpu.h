/**
 * @file    mycpu.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#ifndef MYCPU_H
#define MYCPU_H

#include "cpu.h"
#include "motherboard.h"

class MyCpu : public Cpu
{
public:

    /**
     * Create a new MyCpu
     * @param[in]   mb  Motherboard instance that CPU is to run on
     */
    MyCpu(motherboard::Motherboard& mb);

    /**
     * @return  Name of device
     */
    std::string getName() const;

    /**
     * Start processing CPU instructions at a place in memory
     * @param[in]   addr    Place in memory to start processing from
     */
    void start(motherboard::MemAddress addr);

private:

    motherboard::Motherboard& mb;
};

#endif // MYCPU_H
