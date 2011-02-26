/**
 * @file    mycpu.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#include "mycpu.h"
#include "common.h"

using namespace std;
using namespace motherboard;

/* public function */

SLDECL Device* createDevice(Motherboard& mb)
{
    return new MyCpu(mb);
}

/* public MyCpu */

MyCpu::MyCpu(Motherboard& mb)
: mb(mb)
{   }

string MyCpu::getName() const
{
    return "MyCpu";
}

void MyCpu::start(MemAddress addr)
{

}
