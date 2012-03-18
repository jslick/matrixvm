/**
 * @file    interruptcontroller.h
 *
 * Matrix VM
 */

#ifndef INTERRUPTCONTROLLER_H
#define INTERRUPTCONTROLLER_H

#include <machine/device.h>

namespace machine
{

class Motherboard;

/**
 * @class InterruptController
 */
class InterruptController : public Device
{
public:

    InterruptController(Motherboard& mb);

    /**
     * @return The address in memory where the interrupt vector starts
     */
    virtual MemAddress getInterruptVectorAddress() const = 0;

    /**
     * Get the value of a CPU pin
     * @param[in]   pin
     * @return  The value at `pin`
     */
    MemAddress getPin(unsigned int pin) const;

    /**
     * Set the value of a CPU pin
     * To simplify guest code, a pin is word-sized
     * @param[in]   pin     Which pin to set
     * @param[in]   word    What the value should be
     */
    void setPin(unsigned int pin, MemAddress word);

    /**
     * Generate an interrupt
     * @param[in]   Which hardware-specific line to interrupt on
     */
    virtual void interrupt(unsigned int line) = 0;

protected:

    Motherboard& mb;

private:

    std::unordered_map<unsigned int,MemAddress> pins;

};

}   // namespace machine

#endif // INTERRUPTCONTROLLER_H
