/**
 * @file    charoutputdevice.cpp
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#include "charoutputdevice.h"

#include <iostream>
#include <stdexcept>

using namespace std;
using namespace machine;

/* public CharOutputDevice */

string CharOutputDevice::getName() const
{
    return "RealStdout";
}

void CharOutputDevice::init(Motherboard& mb)
{
    MemAddress dmaLoc = Device::reserveMemIO(mb, OUTDEV_BUFFER_SIZE);
    if (dmaLoc < 0)
        throw runtime_error("Could not request DMA memory for real stdout");
    else
        this->mappingAddr = dmaLoc;

    if (!Device::requestThread( mb,
                                this,
                                &CharOutputDevice::flushOutputCb,
                                HighSpeed))
    {
        throw runtime_error("Could not initiate machine concurrency for "
                            "stdout");
    }
}

void CharOutputDevice::flushOutput(Motherboard& mb)
{
    Memory& memory = Device::getMemory(mb);
    cout << "DMA is at address " << this->mappingAddr << endl;
    cout << &memory[this->mappingAddr] << endl; // the easy part ;)

    throw runtime_error("Testing exception");
}

/* static public CharOutputDevice */

void CharOutputDevice::flushOutputCb(Device* dev, Motherboard& mb)
{
    cout << "Made it\n";
    assert(dev);
    CharOutputDevice* outdev = dynamic_cast<CharOutputDevice*>( dev );
    if (outdev)
        outdev->flushOutput(mb);
}
