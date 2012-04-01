/**
 * @file    timerdevice.cpp
 *
 * Matrix VM
 */

#include "timerdevice.h"
#include <dev/interruptcontroller.h>

#include <unistd.h>

using namespace std;
using namespace machine;

/* public TimerDevice */

string TimerDevice::getName() const
{
    return "Timer";
}

void TimerDevice::init(Motherboard& mb)
{
    this->mb = &mb;
    this->run = true;
    this->interval = 999999;

    if (!Device::requestPort(mb, this, 1))
        throw runtime_error("Could not initiate device port for timer");

    mb.requestThread(this, &TimerDevice::runTimer);
}

void TimerDevice::write(MemAddress what, int port)
{
    #if !EMULATOR_BENCHMARK
    printf("Set timer interval to %d microseconds\n", what);
    #endif
    this->interval = what;
}

void TimerDevice::stopThread(boost::thread* thd)
{
    this->run = false;
}

void TimerDevice::runTimer(Device* dev, Motherboard& mb)
{
    TimerDevice* timer = dynamic_cast<TimerDevice*>( dev );
    assert(timer);

    timer->runTimer();
}

void TimerDevice::runTimer()
{
    InterruptController* ic = this->mb->getInterruptController();
    if (!ic)
        return; // pointless to loop if there's no interrupt controller

    while (this->run)
    {
        if (!this->interval)
        {
            // Need better solution to resume timer; we'll do with this hack for now
            usleep(500000);
            continue;
        }

        if (this->interval < TimerDevice::MIN_INTERVAL)
            this->interval = TimerDevice::MIN_INTERVAL;
        if (this->interval >= 1000000)
            this->interval = 1000000; // maximum value for usleep

        usleep(this->interval);
        if (this->interval)
            ic->interrupt(TIMER_INT_LINE);
    }
}
