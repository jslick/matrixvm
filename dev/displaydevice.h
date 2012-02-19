/**
 * @file    displaydevice.h
 *
 * Matrix VM
 */

#ifndef DISPLAYDEVICE_H
#define DISPLAYDEVICE_H

#include <machine/device.h>
#include "displaymanager.h"

/* 2 bytes for pixels wide + 2 bytes for pixels tall */
#define DISPLAY_SETUP_SIZE      4
#define DISPLAY_RESOLUTION      1920*1080
/* need 3 bytes to encode color */
#define DISPLAY_BYTES_PER_PIXEL 3
#define DISPLAY_BUFFER_SIZE     DISPLAY_SETUP_SIZE + DISPLAY_RESOLUTION * DISPLAY_BYTES_PER_PIXEL

#define DEFAULT_DISPLAY_PORT    8

namespace machine
{

struct DisplayDeviceArgs
{
    DisplayManager* displayManager;
};

/**
 * @class DisplayDevice
 *
 * This is display device that displays a pixel buffer in a host window.  It
 * reserves an area of memory which holds the pixel buffer.  Additionally, it
 * reserves a port, which the consuming code invokes to asynchronously redraw
 * the screen.
 *
 * The emulator passes in an object that does the drawing work.  The
 * DisplayDevice is merely an interface which handles uniform display
 * functionality.  In this way, the emulated code is decoupled from the display
 * server; if implemented, you can use X11, Wayland, Darwin, etc.
 */
class DisplayDevice : public Device
{
public:

    /**
     * @param[in]   display     DisplayManager to use
     */
    DisplayDevice(DisplayManager* display);

    /**
     * @return  Name of the device
     */
    std::string getName() const;

    void init(Motherboard& init);

    /**
     * Flush the display device (update the image)
     *
     * @param[in]   what    Ignored
     * @param[in]   port    Currently ignored
     */
    void write(MemAddress what, int port);

    /**
     * Show the display
     * @param[in]   mb  Motherboard
     */
    void showDisplay(Motherboard& mb);

    /**
     * Stop a thread
     * @param[in]   thd     Thread to stop
     */
    void stopThread(boost::thread* thd);

protected:

    /**
     * Callback called by a new thread, starting in the Motherboard
     *
     * This is a delegate to DisplayDevice::showDisplay(Motherboard&)
     * @param[in]   dev     A DisplayDevice
     * @param[in]   mb      Motherboard
     */
    static void showDisplay(Device* dev, Motherboard& mb);

private:

    int width;
    int height;
    DisplayManager* display;

    Motherboard* mb;

    MemAddress mappingAddr; //<! The DMA address of the obtained reserved memory

};

}   // namespace machine

#endif // DISPLAYDEVICE_H
