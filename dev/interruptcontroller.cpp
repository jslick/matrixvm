#include "interruptcontroller.h"

using namespace machine;

InterruptController::InterruptController(Motherboard& mb)
: mb(mb)
{ }

MemAddress InterruptController::getPin(unsigned int pin) const
{
    return this->pins.at(pin);
}

void InterruptController::setPin(unsigned int pin, MemAddress word)
{
    this->pins[pin] = word;
}
