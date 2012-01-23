/**
 * @file    device.h
 *
 * Matrix VM
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <common.h>
#include "motherboard.h"

#include <string>

namespace machine
{

class Device
{
public:

    virtual ~Device() { }

    /**
     * @return  Name of the device
     */
    virtual std::string getName() const = 0;

    /**
     * Initialize the device
     *
     * This includes setting up memory-mapped I/O, port-mapped I/O, creating
     * I/O device threads from the real OS, etc.
     * @param[in] mb
     */
    virtual void init(Motherboard& mb) { }

    /**
     * Write to this device
     * @param[in]   what    Word to write to device
     * @param[in]   port    Port that was invoked to write to this device
     */
    virtual void write(MemAddress what, int port) { }

protected:

    /**
     * @param[in]   mb
     * @return Main memory from Motherboard
     */
    std::vector<uint8_t>& getMemory(Motherboard& mb) { return mb.getMemory(); }

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
     * Request port from the motherboard
     * @param[in]   mb      Motherboard
     * @param[in]   dev     Device that is requesting the port
     * @param[in]   port    Port number to request.  Defaults to 0
     * @return  Number of obtained port, or 0 to indicate failure
     * @sa Motherboard::requestPort
     */
    int requestPort(
        Motherboard&    mb,
        Device*         dev,
        int             port = 0)
    {
        return mb.requestPort(dev, port);
    }

    /**
     * Tell the motherboard to write to a port
     * @param[in]   mb      Motherboard
     * @param[in]   port    Port to write to
     * @param[in]   what    Word to write to device
     */
    void writeMb(
        Motherboard&    mb,
        int             port,
        MemAddress      what)
    {
        return mb.write(port, what);
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
