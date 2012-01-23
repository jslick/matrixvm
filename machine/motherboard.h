/**
 * @file    motherboard.h
 *
 * Matrix VM
 */

#ifndef MOTHERBOARD_H
#define MOTHERBOARD_H

#include <common.h>

#include <vector>
#include <unordered_map>

// minimum amount of memory (in bytes) required to run the machine
#define MIN_MEMORY 1024

// minimum amount of memory (in bytes) after reserved memory
#define MIN_AVAIL_MEMORY 512

namespace machine
{

class Motherboard;
class Device;
class Cpu;

typedef void (*ReportExceptionFunc)(std::exception& e);

/**
 * Central class to the Matrix VM
 *
 * Devices are loaded into an instance of this class.  Memory characteristics
 * are initialized here.  This class "boots" the VM.
 */
class Motherboard
{
    friend class Device;

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
     * Set callback to report exceptions
     * @param[in]   reportCb    Callback that accepts an exception, which it
     *                          somehow reports
     */
    void setExceptionReport(ReportExceptionFunc reportCb);

    /**
     * @return Size of memory (in bytes)
     */
    MemAddress getMemorySize() const;

    /**
     * @param[in]   size    Size of memory in bytes
     */
    void setMemorySize(MemAddress size);

    /**
     * Set program code that motherboard should execute on bootup
     * @param[in]   program     Program to execute
     * @param[in]   exeStart    Address to start executing from.  If 0,
     *                          execution will start at the address immediately
     *                          beyond reserved memory.  Defaults to 0.
     */
    void setBios(std::vector<uint8_t>& program, int exeStart = 0);

    /**
     * Add a CPU instance to the motherboard
     * @param[in]   cpu     CPU to add to the motherboard
     * @param[in]   master  Whether or not the Cpu is the master CPU
     * @pre     cpu must not be null and must be heap-allocated
     * @note    The Motherboard destructor will delete this Cpu.
     */
    void addCpu(Cpu* cpu, bool master = false);

    /**
     * Add device to Motherboard
     * @param[in] dev   Heap-allocated Device to add
     * @pre     dev must not be null and must be heap-allocated
     * @note    The Motherboard destructor will delete this Device.
     */
    void addDevice(Device* dev);

    /**
     * Start virtual machine
     * @sa setBios
     */
    void start();

    /**
     * Report an exception to the report callback, if set
     * @param[in]   e   The exception
     * @sa  Motherboard::setExceptionReport
     */
    inline void reportException(std::exception& e);

protected:

    /**
     * @return reference to main memory
     */
    std::vector<uint8_t>& getMemory();

    /**
     * Request a spot in memory for DMA (direct memory access)
     *
     * This is to be called from Devices for memory-mapped I/O.
     *
     * This function may fail if there is not enough memory for the requested
     * size.  In this case, -1 is returned.
     * @param[in] size  The size of memory needed for direct memory access
     * @return  The start address of the granted DMA memory, or -1 on failure
     */
    MemAddress reserveMemIO(MemAddress size);

    // TODO:  document

    /**
     * Obtain "hardware" port for a device
     *
     * Software can then `write` to this port.
     * Example:
     *   write 11
     * will write to the device that obtained port 11.
     *
     * @param[in]   dev     Device that is requesting the port.  It is this
     *                      device that will be written to.
     * @param[in]   port    Port to obtain, or 0 to request the next available
     *                      port.  Defaults to 0.
     * @return  The obtained port number, or 0 upon error.  If the requested
     *          port is already taken, 0 is returned to indicate error.
     */
    int requestPort(Device* dev, int port = 0);

    /**
     * Write to a port
     * @param[in]   port    Port number to write to
     * @param[in]   what    A 32-bit value to write to the port
     */
    void write(int port, MemAddress what);

private:

    Motherboard(const Motherboard& mb) { }; /* copy not permitted */

    MemAddress memorySize;

    std::vector<uint8_t> bios;      //!< Program to execute at boot

    MemAddress exeStart;            //!< Address to start execution from at boot

    std::vector<Cpu*> cpus;

    std::vector<Device*> devices;

    int masterCpu;                  //!< Index of CPU to boot from

    std::vector<uint8_t> memory;    //!< Main memory

    MemAddress reservedSize;        //!< Size of reserved memory (the front)

    std::unordered_map<int,Device*> devicePorts;

    ReportExceptionFunc reportCb;   //!< Function to call to report errors to
};

}   // namespace motherboard

#endif // MOTHERBOARD_H
