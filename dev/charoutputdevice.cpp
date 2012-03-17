/**
 * @file    charoutputdevice.cpp
 *
 * Matrix VM
 */

#include "charoutputdevice.h"

#include <stdio.h>
#include <stdexcept>
#include <cassert>

using namespace std;
using namespace machine;

// declared, but not defined, in device.h
SLDECL Device* createDevice(void* args)
{
    return new CharOutputDevice;
}

/* public CharOutputDevice */

string CharOutputDevice::getName() const
{
    return "HostStdout";
}

void CharOutputDevice::init(Motherboard& mb)
{
    this->mb = &mb;

    MemAddress dmaLoc = Device::reserveMemIO(mb, *this, OUTDEV_BUFFER_SIZE);
    if (dmaLoc < 0)
        throw runtime_error("Could not request DMA memory for host stdout");
    else
        this->mappingAddr = dmaLoc;
    // Set the boundary char.  This is inaccessible to the virtualized system.
    // It allows us to print a c-string without any manipulation regardless of
    // whether or not the buffer is null-terminated
    vector<uint8_t>& memory = Device::getMemory(mb);
    memory[dmaLoc + OUTDEV_BUFFER_SIZE - 1] = 0;

    if (!Device::requestPort(mb, this, 1))
        throw runtime_error("Could not initiate device port for host stdout");
}

void CharOutputDevice::write(MemAddress what, int port)
{
    // TODO:  thread it
    printf("%s", reinterpret_cast<const char*>( &Device::getMemory(*mb)[this->mappingAddr+1] ));
    // TODO:  interrupt
}
