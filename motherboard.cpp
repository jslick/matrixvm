/**
 * @file    motherboard.cpp
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#include "motherboard.h"
#include "cpu.h"

#include <stdexcept>

using namespace std;
using namespace machine;

/* public Motherboard */

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

void Motherboard::start()
{
    if (this->cpus.size() < 1)
        throw runtime_error("There are no CPUs to run on");
    // get master CPU
    Cpu* masterCpu = this->cpus[this->masterCpu];

    this->memory = Memory(this->memorySize, 0);

    char bios[] = { 1, 24, 2, 48, 0 };
    for (int i = 0; i < 5; ++i)
        this->memory[i] = bios[i];

    masterCpu->start(*this, 0);
}

/* protected Motherboard */

Memory& Motherboard::getMemory()
{
    return this->memory;
}
