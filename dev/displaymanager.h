#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <common.h>

#include <vector>
#include <cstdint>

class DisplayManager
{
public:

    /**
     * @param[in]   memory          Motherboard memory
     * @param[in]   videoAddress    Address in `memory` where video memory is
     *                              located
     * @param[in]   width           Display width
     * @param[in]   height          Display height
     */
    virtual void init(
        std::vector<uint8_t>&   memory,
        MemAddress              videoAddress,
        int                     width,
        int                     height
        ) = 0;

    /**
     * Show the display
     */
    virtual void show() = 0;

    /**
     * Flush the display (refresh image from memory)
     */
    virtual void flush() = 0;

    /**
     * Destroy the display
     */
    virtual void destroy() = 0;

};

#endif // DISPLAYMANAGER_H
