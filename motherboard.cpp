/**
 * @file    motherboard.cpp
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#include "motherboard.h"

using namespace motherboard;

Motherboard::Motherboard()
: memorySize(0)
{ }

MemAddress Motherboard::getMemorySize() const {
    return this->memorySize;
}

void Motherboard::setMemorySize(MemAddress size)
{
    this->memorySize = size;
}
