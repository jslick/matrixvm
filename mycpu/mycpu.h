/**
 * @file    mycpu.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#ifndef MYCPU_H
#define MYCPU_H

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

};

}   // namespace machine

#endif // MYCPU_H
