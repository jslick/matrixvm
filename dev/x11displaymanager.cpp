#include "x11displaymanager.h"

#include <string.h>
#include <stdio.h>
#include <stdexcept>

using namespace std;

void X11DisplayManager::init(
    std::vector<uint8_t>&           memory,
    MemAddress                      videoAddress,
    machine::InterruptController*   ic,
    int                             width,
    int                             height
    )
{
    this->memory = &memory;
    this->videoAddress = videoAddress;
    this->ic = ic;

    this->width  = width;
    this->height = height;

    this->toFlush = true;
    this->toDestroy = false;

    this->display = 0;
    this->screen = 0;
}

void X11DisplayManager::show()
{
    this->createWindow(this->width, this->height);
    this->eventListen();
}

void X11DisplayManager::flush()
{
    if (!this->toFlush)
        this->toFlush = true;
}

void X11DisplayManager::destroy()
{
    this->toDestroy = true; // tell event loop it's time to quit
}

void X11DisplayManager::createWindow(int width, int height)
{
    unsigned long black;
    unsigned long white;

    this->display = XOpenDisplay((char *) 0);
    if (!this->display)
        throw runtime_error("Could not open display");

    this->screen = DefaultScreen(this->display);
    black = BlackPixel(this->display, this->screen);
    white = WhitePixel(this->display, this->screen);

    win = XCreateSimpleWindow(
        this->display,
        DefaultRootWindow(this->display), // parent window
        0,      // x
        0,      // y
        width,  // width
        height, // height
        5,      // border width
        black,  // border
        black   // background
        );

    XSetStandardProperties(
        this->display,
        this->win,
        "MatrixVM", // window name
        "0",        // icon name
        None,       // icon pixmap
        NULL,       // argv
        0,          // argc
        NULL        // hints
        );

    XSelectInput(
        this->display,
        this->win,
        ExposureMask | StructureNotifyMask | ButtonPressMask | KeyPressMask   // event mask
        );

    this->gc = XCreateGC(
        this->display,
        this->win,  // drawable
        0,          // valuemask
        0           // values
        );

    XClearWindow(this->display, this->win);

    XMapRaised(this->display, this->win);   // maps window, puts it top of stack
}

void X11DisplayManager::eventListen()
{
    XEvent event;
    KeySym key;
    char text[255]; /* a char buffer for KeyPress Events */

    int x11_fd = ConnectionNumber(this->display);
    fd_set in_fds;
    struct timeval tv;

    // initial draw so that there isn't a delay showing window
    this->redraw();

    /* look for events forever... */
    while (1)
    {
        // Create a File Description Set containing x11_fd
        FD_ZERO(&in_fds);
        FD_SET(x11_fd, &in_fds);

        // Set timer
        tv.tv_usec = 0;
        tv.tv_sec = 1;

        // Wait for X Event or a Timer
        if (select(x11_fd+1, &in_fds, 0, 0, &tv))
        {
            // Event recevied
            // Nothing to do here
        }
        else
        {   // Timer fired
            if (this->toDestroy)
            {
                this->closeWindow();
                break;
            }
            else if (this->toFlush)
            {
                // This order is important, if guest code calls flush multiple times.
                // If these were reversed, a flush could be missed.
                this->toFlush = false;
                this->redraw();
            }
        }

        // Handle XEvents and flush the input
        while (XPending(this->display))
        {
            XNextEvent(this->display, &event);

            if (event.type == Expose && event.xexpose.count == 0)
                this->redraw();

            if (this->ic)
            {
                bool keypress   = event.type == KeyPress;
                bool keyrelease = event.type == KeyRelease;
                if (keypress || keyrelease)
                {
                    // Set CPU pin
                    this->ic->setPin(KEYBOARD_DATA_PIN, event.xkey.keycode | (keyrelease ? 0x100 : 0));

                    // Interrupt
                    this->ic->interrupt(KEYBOARD_INT_LINE);
                }
                else if (event.type == ButtonPress)
                {
                    //int x = event.xbutton.x,
                    //    y = event.xbutton.y;
                    // TODO:  interrupt
                }
            }
        }
    }
}

// I'm no X11 expert; someone needs to fix this
void X11DisplayManager::redraw()
{
    vector<uint8_t>& memref = *this->memory;

    MemAddress addr = videoAddress;
    for (int y = 0; y < 480; y++)
    {
        for (int x = 0; x < 640; x++)
        {
            unsigned long color = memref[addr + 0] << 16 |
                                  memref[addr + 1] <<  8 |
                                  memref[addr + 2] <<  0;
            XSetForeground(this->display, this->gc, color);
            XDrawPoint(this->display, this->win, this->gc, x, y);

            addr += 3;
        }

        XFlush(this->display);
    }
}

void X11DisplayManager::closeWindow()
{
    XFreeGC(this->display, this->gc);
    XDestroyWindow(this->display, this->win);
    XCloseDisplay(this->display);
}
