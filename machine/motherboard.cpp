/**
 * @file    motherboard.cpp
 *
 * Matrix VM
 */

#include "motherboard.h"
#include "cpu.h"
#include "device.h"
#include <dev/interruptcontroller.h>

#include <sstream>
#include <cassert>
#include <stdexcept>

using namespace std;
using namespace machine;

/* public Motherboard */

Motherboard::Motherboard()
: memorySize(0), ic(0), started(false), aborted(false), exeStart(0), masterCpu(0),
  reservedSize(4 /* reserve 0 */),
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

InterruptController* Motherboard::getInterruptController()
{
    return this->ic;
}

void Motherboard::setInterruptController(InterruptController* ic)
{
    this->ic = ic;
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

Cpu* Motherboard::getMasterCpu()
{
    try
    {
        return this->cpus.at(this->masterCpu);
    }
    catch (out_of_range& e)
    {
        return 0;
    }
}

void Motherboard::addDevice(Device* dev)
{
    assert(dev);
    if (!dev)
        throw runtime_error("Cannot add null device to Motherboard");

    this->devices.push_back(dev);
}

bool Motherboard::start()
{
    if (this->cpus.size() < 1)
        throw runtime_error("There are no CPUs to run on");
    if (this->memorySize < MIN_MEMORY)
        throw runtime_error("There is not enough available memory");

    // get master CPU
    Cpu* masterCpu = this->cpus[this->masterCpu];

    // initialize memory
    this->memory = vector<uint8_t>(this->memorySize, 0);

    /* Initialize each device */
    // Initialize CPUs first
    // TODO

    // Initialize interrupt controller
    if (this->ic) {
        try
        {
            this->ic->init(*this);
        }
        catch (exception& e)
        {   // don't crash VM
            this->reportException(e);
        }
    }

    // This is serial so that there are no race conditions.  Guest code may
    // rely on devices requesting particular DMA regions, which my be
    // influenced by the order that a device is able to request DMA.
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

        if (this->aborted)
            break;
    }

    if (this->aborted)
    {
        // TODO:  uninitialize devices
        return false;
    }

    // Start device-requested threads
    for (list<DeviceThread>::iterator iter = this->deviceThreads.begin();
            iter != this->deviceThreads.end();
            ++iter)
    {
        (*iter).thd = new boost::thread(&Motherboard::runThread, this, *iter);
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
        this->started = true;
        sleep(1);
        masterCpu->start(*this, exeStart);

        sleep(3);  // temporary, until interrupts and timers are implemented
    } catch (exception& e)
    {   // don't crash VM while other threads can be running
        this->reportException(e);
    }

    printf("Stopping devices\n");

    // Tell each thread to stop
    for (list<DeviceThread>::iterator iter = this->deviceThreads.begin();
         iter != this->deviceThreads.end();
         ++iter)
    {
        DeviceThread& dt = *iter;
        if (dt.thd)
        {
            assert(dt.dev);
            dt.dev->stopThread(dt.thd);
        }
    }

    // join threads
    for (list<DeviceThread>::iterator iter = this->deviceThreads.begin();
         iter != this->deviceThreads.end();
         ++iter)
    {
        if (boost::thread* thd = (*iter).thd)
        {
            thd->join();
            delete thd;
        }
    }

    return true;
}

void Motherboard::abort()
{
    if (this->started)
    {
        // I refuse
    }
    else
    {
        this->aborted = true;
    }
}

inline void Motherboard::reportException(exception& e)
{
    if (this->reportCb)
        reportCb(*this, e);
}

bool Motherboard::requestThread(Device* dev, DeviceCallFunc cb)
{
    if (!dev)
        throw runtime_error("Cannot request device thread for null device");
    if (!cb)
        throw runtime_error("Cannot request device thread with null callback");

    DeviceThread dt;
    dt.dev = dev;
    dt.cb  = cb;
    dt.thd = 0;
    this->deviceThreads.push_back(dt);

    return true;
}

/* protected Motherboard */

vector<uint8_t>& Motherboard::getMemory()
{
    return this->memory;
}

MemAddress Motherboard::reserveMemIO(Device& dev, MemAddress size)
{
    assert(size > 0);
    if (size <= 0)
        throw runtime_error("Cannot request non-positive memory");

    if (this->memorySize - this->reservedSize - size < MIN_AVAIL_MEMORY)
    {
        return -1;  // not enough mem
    }
    else
    {
        printf("Memory for %-32s:  0x%08x - 0x%08x\n",
            dev.getName().c_str(),
            this->reservedSize,
            this->reservedSize + size - 1);
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

void Motherboard::runThread(Motherboard* mb, DeviceThread& dt)
{
    if (!dt.cb)
        return;

    try
    {
        (dt.cb)(dt.dev, *mb);
    }
    catch (exception& e)
    {
        mb->reportException(e);
    }
}
