#include "basicinterruptcontroller.h"

#include <machine/motherboard.h>
#include <machine/cpu.h>

#include <stdexcept>

using namespace std;
using namespace machine;

BasicInterruptController::BasicInterruptController(Motherboard& mb)
: InterruptController(mb)
{ }

void BasicInterruptController::init(Motherboard& mb)
{
    MemAddress vectorLoc = Device::reserveMemIO(mb, *this, BasicCpu::NUM_INTERRUPT_LINES * 4);
    if (vectorLoc < 0)
        throw runtime_error("Could not obtain memory for interrupt vector");
    else
        this->vectorLoc = vectorLoc;
}

std::string BasicInterruptController::getName() const
{
    return "Basic interrupt controller";
}

MemAddress BasicInterruptController::getInterruptVectorAddress() const
{
    return this->vectorLoc;
}

void BasicInterruptController::interrupt(unsigned int line)
{
    this->mb.getMasterCpu()->interrupt(line);
}
