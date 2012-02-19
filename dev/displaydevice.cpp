/**
 * @file    displaydevice.cpp
 *
 * Matrix VM
 */

#include "displaydevice.h"

#include <stdio.h>
#include <stdexcept>
#include <cassert>

using namespace std;
using namespace machine;

// declared, but not defined, in device.h
SLDECL Device* createDevice(void* args)
{
    DisplayDeviceArgs* ddaArgs = reinterpret_cast<DisplayDeviceArgs*>( args );
    return new DisplayDevice(ddaArgs->displayManager);
}

/* public DisplayDevice */

DisplayDevice::DisplayDevice(DisplayManager* display)
: display(display)
{ }

string DisplayDevice::getName() const
{
    return "HostDisplay";
}

void DisplayDevice::init(Motherboard& mb)
{
    this->mb = &mb;

    /* Reserve display memory */
    MemAddress dmaLoc = Device::reserveMemIO(mb, DISPLAY_BUFFER_SIZE);
    if (dmaLoc < 0)
        throw runtime_error("Could not request DMA memory for host display");
    else
        this->mappingAddr = dmaLoc;

    /* Reserve display port */
    if (!Device::requestPort(mb, this, DEFAULT_DISPLAY_PORT))
        throw runtime_error("Could not initiate device port for host display");

    /* Tell the display manager that it can start */
    if (this->display)
    {
        vector<uint8_t>& memory = Device::getMemory(mb);
        this->display->init(memory, dmaLoc, 640, 480);
        mb.requestThread(this, &DisplayDevice::showDisplay);
    }
    else
    {
        /* TODO:  what should we do? */
    }
}

void DisplayDevice::write(MemAddress what, int port)
{
    // TODO:  thread it
    this->display->flush();
    // TODO:  interrupt
}

void DisplayDevice::showDisplay(Motherboard& mb)
{
    printf("Opening display\n");
    this->display->show();
}

void DisplayDevice::stopThread(boost::thread* thd)
{
    printf("Closing display\n");
    this->display->destroy();
}

void DisplayDevice::showDisplay(Device* dev, Motherboard& mb)
{
    if (DisplayDevice* dd = dynamic_cast<DisplayDevice*>( dev ))
        dd->showDisplay(mb);
}
