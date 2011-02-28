/**
 * @file    motherboard.cpp
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#include "motherboard.h"
#include "cpu.h"

using namespace std;
using namespace motherboard;

Motherboard::Motherboard()
: memorySize(0), masterCpu(0)
{ }

Motherboard::~Motherboard()
{
    for (vector<Cpu*>::size_type i = 0; i < this->cpus.size(); ++i)
    {
        delete this->cpus[i];
        this->cpus[i] = 0;
    }
}

MemAddress Motherboard::getMemorySize() const {
    return this->memorySize;
}

void Motherboard::setMemorySize(MemAddress size)
{
    this->memorySize = size;
}

void Motherboard::addCpu(Cpu* cpu, bool master /* = false */)
{
    this->cpus.push_back(cpu);
    if (master)
        this->masterCpu = this->cpus.size() - 1;
}
