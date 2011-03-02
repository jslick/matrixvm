/**
 * @file    device.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <common/common.h>
#include "motherboard.h"

#include <string>

namespace machine
{

class Device
{
public:

    /**
     * @return  Name of the device
     */
    virtual std::string getName() const = 0;

    virtual ~Device() { }

    /**
     * Initialize the device
     *
     * This includes setting up memory-mapped I/O, port-mapped I/O, creating
     * I/O device threads from the real OS, etc.
     * @param[in] mb
     */
    virtual void init(Motherboard& mb) { }

protected:

    /**
     * @param[in]   mb
     * @return Main memory from Motherboard
     */
    Memory& getMemory(Motherboard& mb) { return mb.getMemory(); }

    /**
     * Request DMA memory from the Motherboard
     * @param[in] mb
     * @param[in] size  Size of the requested DMA memory
     * @return  The start address of the requestd memory, or -1 on failure
     * @sa Motherboard::reserveMemIO
     */
    MemAddress reserveMemIO(Motherboard& mb, MemAddress size)
    {
        return mb.reserveMemIO(size);
    }

    /**
     * Request a device thread from the Motherboard
     * @param[in]   mb
     * @param[in]   dev     Requesting device
     * @param[in]   cb      Callback used to perform I/O (the probably reason
     *                      for the thread)
     * @param[in]   speed   Relative speed of the device (rate of I/O)
     * @sa Motherboard::requestThread
     */
    bool requestThread( Motherboard&    mb,
                        Device*         dev,
                        DeviceCallFunc  cb,
                        DeviceSpeed     speed)
    {
        return mb.requestThread(dev, cb, speed);
    }
};

}   // namespace machine

/**
 * Create an instance of the device
 * @return  The loaded device
 * @post    The calling function should delete the device.  The library does
 *          not delete the object.  The `new` and `delete` operators must *not*
 *          be overridden.
 */
SLDECL machine::Device* createDevice();

#endif // DEVICE_H
