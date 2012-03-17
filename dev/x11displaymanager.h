#ifndef X11DISPLAYMANAGER_H
#define X11DISPLAYMANAGER_H

#include "displaymanager.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

class X11DisplayManager : public DisplayManager
{
public:

    void init(
        std::vector<uint8_t>&   memory,
        MemAddress              videoAddress,
        int                     width,
        int                     height
        );

    void show();

    void flush();

    void destroy();

protected:

    void createWindow(int width, int height);

    void eventListen();

    void redraw();

    void closeWindow();

private:

    std::vector<uint8_t>*   memory;
    MemAddress              videoAddress;

    int width;
    int height;

    bool toFlush;   //<! Tells the event loop to flush display
    bool toDestroy; //<! Tells the event loop to quit and destroy window

    /* X variables */
    Display*    display;
    int         screen;
    Window      win;
    GC          gc;

};

#endif // X11DISPLAYMANAGER_H
