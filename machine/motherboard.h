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
#include <boost/thread.hpp>

// minimum amount of memory (in bytes) required to run the machine
#define MIN_MEMORY 1024

// minimum amount of memory (in bytes) after reserved memory
#define MIN_AVAIL_MEMORY 512

namespace machine
{

class Motherboard;
class Device;
class Cpu;
class InterruptController;

typedef void (*DeviceCallFunc)(Device* dev, Motherboard& mb);
typedef void (*ReportExceptionFunc)(Motherboard& mb, std::exception& e);

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
     * @return The InterruptController, or null
     */
    InterruptController* getInterruptController();

    /**
     * Set the interrupt controller
     * @param[in]   ic  The interrupt controller; can be null
     */
    void setInterruptController(InterruptController* ic);

    /**
     * Add a CPU instance to the motherboard
     * @param[in]   cpu     CPU to add to the motherboard
     * @param[in]   master  Whether or not the Cpu is the master CPU
     * @pre     cpu must not be null and must be heap-allocated
     * @note    The Motherboard destructor will delete this Cpu.
     */
    void addCpu(Cpu* cpu, bool master = false);

    Cpu* getMasterCpu();

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
     * @return  true if emulator stopped normally, false if emulation was
     *          aborted
     */
    bool start();

    /**
     * Abort the emulator, tell devices to quit
     */
    void abort();

    /**
     * Report an exception to the report callback, if set
     * @param[in]   e   The exception
     * @sa  Motherboard::setExceptionReport
     */
    inline void reportException(std::exception& e);

    /**
     * Request a thread
     *
     * All devices should request threads from the Motherboard instead of
     * generating its own threads.  This way, all threads are within control of
     * the Motherboard.
     * @param[in]   dev     Device that requests a thread
     * @param[in]   cb      Callback function to call to start the thread
     * @return  True if the thread was granted
     */
    bool requestThread(Device* dev, DeviceCallFunc cb);

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
     * @param[in] dev   Device that is reserving memory
     * @param[in] size  The size of memory needed for direct memory access
     * @return  The start address of the granted DMA memory, or -1 on failure
     */
    MemAddress reserveMemIO(Device& dev, MemAddress size);

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

protected:

    struct DeviceThread
    {
        Device*         dev;
        DeviceCallFunc  cb;
        boost::thread*  thd;
    };

    /**
     * Entry to new thread
     * @param[in]   mb  Motherboard
     * @param[in]   dt  DeviceThread
     */
    static void runThread(Motherboard* mb, DeviceThread& dt);

private:

    Motherboard(const Motherboard& mb) { }; /* copy not permitted */

    MemAddress memorySize;

    InterruptController* ic;

    bool started;                   //!< Whether or not a CPU has been started

    bool aborted;

    std::vector<uint8_t> bios;      //!< Program to execute at boot

    MemAddress exeStart;            //!< Address to start execution from at boot

    std::vector<Cpu*> cpus;

    std::vector<Device*> devices;

    int masterCpu;                  //!< Index of CPU to boot from

    std::vector<uint8_t> memory;    //!< Main memory

    MemAddress reservedSize;        //!< Size of reserved memory (the front)

    std::list<DeviceThread> deviceThreads;

    std::unordered_map<int,Device*> devicePorts;

    ReportExceptionFunc reportCb;   //!< Function to call to report errors to
};

}   // namespace motherboard

#endif // MOTHERBOARD_H
