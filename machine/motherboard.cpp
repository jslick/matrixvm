/**
 * @file    motherboard.cpp
 *
 * Matrix VM
 */

#include "motherboard.h"
#include "cpu.h"
#include "device.h"

#include <sstream>
#include <cassert>
#include <stdexcept>

using namespace std;
using namespace machine;

/* public Motherboard */

Motherboard::Motherboard()
: memorySize(0), exeStart(0), masterCpu(0), reservedSize(1 /* reserve 0 */),
  reportCb(0)
{ }

Motherboard::~Motherboard()
{
    for (vector<Cpu*>::size_type i = 0; i < this->cpus.size(); ++i)
    {
        delete this->cpus[i];
        this->cpus[i] = 0;
    }
    for (vector<Device*>::size_type i = 0; i < this->devices.size(); ++i)
    {
        delete this->devices[i];
        this->devices[i] = 0;
    }
}

void Motherboard::setExceptionReport(ReportExceptionFunc reportCb)
{
    this->reportCb = reportCb;
}

MemAddress Motherboard::getMemorySize() const
{
    return this->memorySize;
}

void Motherboard::setMemorySize(MemAddress size)
{
    this->memorySize = size;
}

void Motherboard::setBios(vector<uint8_t>& program, MemAddress exeStart /* = 0 */)
{
    if (exeStart % 4)
    {
        stringstream msg;
        msg << "Invalid start address:  0x" << hex << exeStart;
        throw runtime_error(msg.str());
    }

    this->bios = program;
    this->exeStart = exeStart;
}

void Motherboard::addCpu(Cpu* cpu, bool master /* = false */)
{
    assert(cpu);
    if (!cpu)
        throw runtime_error("Cannot add null CPU to Motherboard");

    this->cpus.push_back(cpu);
    if (master)
        this->masterCpu = this->cpus.size() - 1;
}

void Motherboard::addDevice(Device* dev)
{
    assert(dev);
    if (!dev)
        throw runtime_error("Cannot add null device to Motherboard");

    this->devices.push_back(dev);
}

void Motherboard::start()
{
    if (this->cpus.size() < 1)
        throw runtime_error("There are no CPUs to run on");
    if (this->memorySize < MIN_MEMORY)
        throw runtime_error("There is not enough available memory");

    // get master CPU
    Cpu* masterCpu = this->cpus[this->masterCpu];

    // initialize memory
    this->memory = vector<uint8_t>(this->memorySize, 0);

    // initialize each device
    for (vector<Device*>::size_type i = 0; i < this->devices.size(); ++i)
    {
        try
        {
            this->devices[i]->init(*this);
        }
        catch (exception& e)
        {   // don't crash VM; just ignore device
            this->reportException(e);
        }
    }

    int exeStart = this->exeStart <= 0 ? this->reservedSize : this->exeStart;
    // align start point to instruction-length value
    int exeMod = exeStart % 4;
    if (exeMod)
        exeStart += (4 - exeMod);
    for (unsigned int i = 0; i < this->bios.size(); i++)
    {
        this->memory[i + exeStart] = bios[i];
    }

    try
    {
        // TODO:  thread it
        masterCpu->start(*this, exeStart);
    } catch (exception& e)
    {   // don't crash VM while other threads can be running
        this->reportException(e);
    }
}

inline void Motherboard::reportException(exception& e)
{
    if (this->reportCb)
        reportCb(e);
}

/* protected Motherboard */

vector<uint8_t>& Motherboard::getMemory()
{
    return this->memory;
}

MemAddress Motherboard::reserveMemIO(MemAddress size)
{
    assert(size > 0);
    if (size <= 0)
        throw runtime_error("Cannot request non-positive memory");

    if (this->memorySize - this->reservedSize < MIN_AVAIL_MEMORY)
        return -1;  // not enough mem
    else
    {
        this->reservedSize += size;
        return this->reservedSize - size;
    }
}

int Motherboard::requestPort(Device* dev, int port /* = 0 */)
{
    if (port == 0)
    {   // find first available port
        while(this->devicePorts.find(++port) != this->devicePorts.end());
    }
    else if (this->devicePorts.find(port) != this->devicePorts.end())
    {   // port already taken
        return 0 /* false */;
    }

    this->devicePorts[port] = dev;

    return port;
}

void Motherboard::write(int port, MemAddress what)
{
    std::unordered_map<int,Device*>::const_iterator iter = this->devicePorts.find(port);
    if (iter == this->devicePorts.end())
        throw runtime_error("Cannot write to port; no device is installed");
    // TODO:  generate fault instead of throwing error!

    Device* dev = (*iter).second;
    assert(dev);

    dev->write(what, port);
}
