#ifndef NULLDISPLAYMANAGER_H
#define NULLDISPLAYMANAGER_H

#include "displaymanager.h"

class NullDisplayManager : public DisplayManager
{
public:

    void init(
        std::vector<uint8_t>&           memory,
        MemAddress                      videoAddress,
        machine::InterruptController*   ic,
        int                             width,
        int                             height
        );

    void show();

    void flush();

    void destroy();

};

#endif // NULLDISPLAYMANAGER_H
