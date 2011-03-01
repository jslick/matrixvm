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
};

}   // namespace machine

/**
 * Create an instance of the device
 * @return  The loaded device
 * @post    The calling function should delete the device.  The library does
 *          not delete the object.  The `new` and `delete` must *not* be
 *          overridden.
 */
SLDECL machine::Device* createDevice();

#endif // DEVICE_H
