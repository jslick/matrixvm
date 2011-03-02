/**
 * @file    mycpu.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#include "mycpu.h"
#include <common/common.h>

using namespace std;
using namespace machine;

// TODO:  Move these
#define HALT    0x00
#define MOV1    0x08
#define MOV2    0x09

/* public MyCpu */

// declared, but not defined, in device.h
SLDECL Device* createDevice()
{
    return new MyCpu;
}

/* public MyCpu */

string MyCpu::getName() const
{
    return "MyCpu";
}

void MyCpu::start(Motherboard& mb, MemAddress addr)
{
    Memory& memory = Device::getMemory(mb);

    bool halt = false;
    while (!halt)
    {
        switch (memory[addr++])
        {
        case HALT:
            halt = true;
            break;
        default:    // undefined instruction
            break;
        }
    }
}
