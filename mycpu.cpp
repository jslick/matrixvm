/**
 * @file    mycpu.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#include "mycpu.h"
#include "common.h"

using namespace std;
using namespace machine;

/* public function */

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

#include <iostream> // TODO:  remove
void MyCpu::start(Motherboard& mb, MemAddress addr)
{
    Memory& memory = Cpu::getMemory(mb);
    cout << &memory[addr] << endl;
}
