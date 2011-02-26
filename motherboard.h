/**
 * @file    motherboard.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#ifndef MOTHERBOARD_H
#define MOTHERBOARD_H

namespace motherboard
{

typedef int MemSize;

/**
 * Central class to the Matrix VM
 *
 * Devices are loaded into an instance of this class.  Memory characteristics
 * are initialized here.  This class "boots" the VM.
 */
class Motherboard
{
public:
    /**
     * Creates an unbootable Motherboard.  Use the setters to make it bootable.
     */
    Motherboard();

    /**
     * @return Size of memory (in bytes)
     */
    MemSize getMemorySize() const;

    /**
     * @param[in]   size    Size of memory in bytes
     */
    void setMemorySize(MemSize size);

private:
    Motherboard(const Motherboard& mb) { }; /* copy not permitted */

    MemSize memorySize;
};

}   // namespace motherboard

#endif // MOTHERBOARD_H
