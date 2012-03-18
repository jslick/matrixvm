/**
 * @file    basicinterruptcontroller.h
 *
 * Matrix VM
 */

#ifndef BASICINTERRUPTCONTROLLER_H
#define BASICINTERRUPTCONTROLLER_H

#include <dev/interruptcontroller.h>
#include <basiccpu/basiccpu.h>

namespace machine
{

/**
 * @class BasicInterruptController
 */
class BasicInterruptController : public InterruptController
{
public:

    BasicInterruptController(Motherboard& mb);

    void init(Motherboard& mb);

    std::string getName() const;

    MemAddress getInterruptVectorAddress() const;

    void interrupt(unsigned int line);

private:

    MemAddress vectorLoc;

};

}   // namespace machine

#endif // BASICINTERRUPTCONTROLLER_H
