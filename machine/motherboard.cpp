/**
 * @file    motherboard.cpp
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#include "motherboard.h"
#include "cpu.h"
#include "device.h"

#include <cassert>
#include <stdexcept>

using namespace std;
using namespace machine;

/* public Motherboard */

Motherboard::Motherboard()
: memorySize(0), masterCpu(0), reservedSize(1 /* reserve 0 */),
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
    this->memory = Memory(this->memorySize, 0);

    // temporary test code
    char bios[] = { 1, 24, 2, 48, 0 };
    for (int i = 0; i < 5; ++i)
        this->memory[i + this->reservedSize] = bios[i];

    // initialize each device
    for (vector<Device*>::size_type i = 0; i < this->devices.size(); ++i)
    {
        try
        {
            this->devices[i]->init(*this);
        } catch (exception& e)
        {   // don't crash VM; just ignore device
            this->reportException(e);
        }
    }

    // create device threads
    for (list<DeviceThread>::iterator iter = this->deviceThreads.begin();
         iter != this->deviceThreads.end();
         ++iter)
    {
        this->startThread(*iter);
    }

    try
    {
        masterCpu->start(*this, this->reservedSize);
    } catch (exception& e)
    {   // don't crash VM while other threads can be running
        this->reportException(e);
    }
    // join device threads
    for (list<DeviceThread>::iterator iter = this->deviceThreads.begin();
         iter != this->deviceThreads.end();
         ++iter)
    {
        boost::thread* thd = (*iter).thd;
        if (thd)
            thd->join();
    }
}

inline void Motherboard::reportException(exception& e)
{
    if (this->reportCb)
        reportCb(e);
}

/* protected Motherboard */

Memory& Motherboard::getMemory()
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

bool Motherboard::requestThread(Device*         dev,
                                DeviceCallFunc  cb,
                                DeviceSpeed     speed)
{
    assert(cb);
    if (!dev)
        throw runtime_error("Cannot request device thread for null device");
    assert(cb);
    if (!cb)
        throw runtime_error("Cannot request device thread with null callback");

    DeviceThread dt;
    dt.dev   = dev;
    dt.cb    = cb;
    dt.speed = speed;
    this->deviceThreads.push_back(dt);

    return true;
}

/* private Motherboard */

void Motherboard::startThread(DeviceThread& dt)
{
    dt.thd = new boost::thread(&Motherboard::runDeviceThread, this, dt);
}

/* static private Motherboard */

void Motherboard::runDeviceThread(Motherboard* mb, DeviceThread& dt)
{
    if (!dt.cb)
        return;

    try
    {
        // TODO:  loop
        (dt.cb)(dt.dev, *mb);
    } catch (exception& e)
    {   // don't crash thread/program
        mb->reportException(e);
    }
}
