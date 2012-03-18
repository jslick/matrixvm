/**
 * @file    timerdevice.h
 *
 * Matrix VM
 */

#ifndef TIMERDEVICE_H
#define TIMERDEVICE_H

#include <machine/device.h>

#define TIMER_INT_LINE   0

namespace machine
{

/**
 * @class TimerDevice
 */
class TimerDevice : public Device
{
public:

    static const MemAddress MIN_INTERVAL = 1000;

    /**
     * @return  Name of the device
     */
    std::string getName() const;

    void init(Motherboard& init);

    /**
     * Change the timer interval
     *
     * @param[in]   what    Number of microseconds between timer interrupts
     * @param[in]   port    Currently ignored
     */
    void write(MemAddress what, int port);

    /**
     * Stops the timer thread
     * @param[in]   thd     Thread to stop; ignored
     */
    void stopThread(boost::thread* thd);

protected:

    /**
     * Delegate callback to call runTimer()
     */
    static void runTimer(Device* dev, Motherboard& mb);

    /**
     * Generates interrupts every `interval` microseconds
     */
    void runTimer();

private:

    Motherboard* mb;

    bool        run;
    MemAddress  interval;

};

}   // namespace machine

#endif // TIMERDEVICE_H
