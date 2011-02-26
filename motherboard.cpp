/**
 * @file    motherboard.cpp
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#include "motherboard.h"

Motherboard::Motherboard()
: memorySize(0)
{ }

MemSize Motherboard::getMemorySize() const {
    return this->memorySize;
}

void Motherboard::setMemorySize(MemSize size)
{
    this->memorySize = size;
}
