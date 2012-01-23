/**
 * @file    charoutputdevice.h
 *
 * Matrix VM
 */

#ifndef CHAROUTPUTDEVICE_H
#define CHAROUTPUTDEVICE_H

#include "device.h"

#include <string>

// corresponds to the amount of memory-mapped to reserve
#define OUTDEV_BUFFER_SIZE 83
// 8 bits for flags + 80 chars + 1 null char + 1 boundary char (to eliminate
// costly manipulation)

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

    virtual ~CharOutputDevice() { }

    /**
     * @return  Name of the device
     */
    virtual std::string getName() const;

    virtual void init(Motherboard& init);

    /**
     * Writes the buffer located at the reserverd DMA location to a file on the
     * host
     *
     * @param[in]   what    Ignored
     * @param[in]   port    Currently ignored
     */
    virtual void write(MemAddress what, int port);

private:

    Motherboard* mb;

    MemAddress mappingAddr; //<! The DMA address of the obtained reserved memory
};

}   // namespace machine

#endif // CHAROUTPUTDEVICE_H
