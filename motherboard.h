/**
 * @file    motherboard.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#ifndef MOTHERBOARD_H
#define MOTHERBOARD_H

#include <vector>

class Cpu;

namespace motherboard
{

typedef int MemAddress;

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
     * Destroys the Motherboard's CPUs
     */
    ~Motherboard();

    /**
     * @return Size of memory (in bytes)
     */
    MemAddress getMemorySize() const;

    /**
     * @param[in]   size    Size of memory in bytes
     */
    void setMemorySize(MemAddress size);

    /**
     * Add a CPU instance to the motherboard
     * @param[in]   cpu     CPU to add to the motherboard
     * @param[in]   master  Whether or not the Cpu is the master CPU
     * @note    The Motherboard destructor will delete this CPU.
     */
    void addCpu(Cpu* cpu, bool master = false);

private:

    Motherboard(const Motherboard& mb) { }; /* copy not permitted */

    MemAddress memorySize;

    std::vector<Cpu*> cpus;

    int masterCpu;
};

}   // namespace motherboard

#endif // MOTHERBOARD_H
