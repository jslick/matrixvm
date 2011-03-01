/**
 * @file    motherboard.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#ifndef MOTHERBOARD_H
#define MOTHERBOARD_H

#include <vector>
#include <string>


namespace machine
{

class Cpu;

typedef std::string Memory;
typedef int MemAddress;

/**
 * Central class to the Matrix VM
 *
 * Devices are loaded into an instance of this class.  Memory characteristics
 * are initialized here.  This class "boots" the VM.
 */
class Motherboard
{
    friend class Cpu;

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

    /**
     * Start virtual machine
     */
    void start();

protected:

    /**
     * @return reference to main memory
     */
    Memory& getMemory();

private:

    Motherboard(const Motherboard& mb) { }; /* copy not permitted */

    MemAddress memorySize;

    std::vector<Cpu*> cpus;

    int masterCpu;  //< Cpu index to boot from

    Memory memory;  //< Main memory
};

}   // namespace motherboard

#endif // MOTHERBOARD_H
