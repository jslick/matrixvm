/**
 * @file    device.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#ifndef DEVICE_H
#define DEVICE_H

#include "common.h"
#include "motherboard.h"

#include <string>

class Device
{
public:

    /**
     * @return  Name of the device
     */
    virtual std::string getName() const = 0;

    virtual ~Device() { }
};

/**
 * Create an instance of the device
 * @param[in]   mb  The Motherboard instance that the device belongs to
 * @return  The loaded device
 * @post    The calling function should delete the device.  The library does
 *          not delete the object.
 */
SLDECL Device* createDevice(motherboard::Motherboard& mb);

#endif // DEVICE_H
