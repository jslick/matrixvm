/**
 * @file    charoutputdevice.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#ifndef CHAROUTPUTDEVICE_H
#define CHAROUTPUTDEVICE_H

#include "device.h"

#include <string>

// corresponds to the amount of memory-mapped to reserve
#define OUTDEV_BUFFER_SIZE 82 // 8-bit flags + 80 chars + 1 null char

namespace machine
{

/**
 * @class CharOutputDevice
 *
 * This is a character output device.  This allows the virtualized system to
 * communicate with the real operating system through character data.
 */
class CharOutputDevice : public Device
{
public:

    /**
     * @return  Name of the device
     */
    virtual std::string getName() const;

    virtual ~CharOutputDevice() { }

    virtual void init(Motherboard& init);

    /**
     * Flush output to real OS standard streams
     * @param[in] mb
     */
    virtual void flushOutput(Motherboard& mb);

    /**
     * Wrapper for CharOutputDevice::flushOutput
     *
     * This is called by the Motherboard Device thread for this device.
     * @param[in] dev   Used to call flushOutput
     * @param[in] mb
     */
    static void flushOutputCb(Device* dev, Motherboard& mb);

private:
    MemAddress mappingAddr;
};

}   // namespace machine

#endif // CHAROUTPUTDEVICE_H
