#include "nulldisplaymanager.h"

#include <string.h>
#include <stdio.h>
#include <stdexcept>

using namespace std;

void NullDisplayManager::init(
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
}

void NullDisplayManager::show()
{
}

void NullDisplayManager::flush()
{
}

void NullDisplayManager::destroy()
{
}
