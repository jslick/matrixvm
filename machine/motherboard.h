/**
 * @file    motherboard.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#ifndef MOTHERBOARD_H
#define MOTHERBOARD_H

#include <vector>
#include <list>
#include <string>
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

typedef std::string Memory;
typedef int MemAddress;

typedef enum DeviceSpeed_enum
{
    HighSpeed,
    NormalSpeed,
    LowSpeed,
    VeryLowSpeed
} DeviceSpeed;

typedef void (*DeviceCallFunc)(Device* dev, Motherboard& mb);
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
    Memory& getMemory();

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

    // (A real OS thread)
    /**
     * Request a real OS thread to manage devices
     *
     * This is used to handle I/O.
     * @param[in]   dev     Device that is requesting the thread
     * @param[in]   cb      Callback function that is called when the thread
     *                      desires to perform the I/O
     * @param[in]   speed   The relative speed of the device
     * @return  true on success; otherwise false
     */
    bool requestThread(Device* dev, DeviceCallFunc cb, DeviceSpeed speed);

private:

    struct DeviceThread
    {
        Device*         dev;
        DeviceCallFunc  cb;
        DeviceSpeed     speed;
        boost::thread*  thd;    // deleted on DeviceThread destruction
        bool            run;    //< semaphore - run=false -> quit thread

        DeviceThread() : dev(0), cb(0), speed(NormalSpeed), thd(0), run(true)
        { }

        ~DeviceThread() { delete thd; }
    };

    Motherboard(const Motherboard& mb) { }; /* copy not permitted */

    /**
     * Start a device thread
     * @param[in] dt    DeviceThread information
     */
    void startThread(DeviceThread& dt);

    /**
     * Multi-threaded function to run a device thread
     *
     * This function periodically calls the DeviceThread callback.  It contains
     * a loop, which sleeps accordingly to the DeviceThread speed.
     * @param[in]   mb
     * @param[in]   dt  DeviceThread info; the dt.run flag is checked to
     *                  terminate the thread; dt.cb contains the callback to
     *                  the Device's choice.  dt.speed contains the speed of
     *                  device thread.
     */
    static void runDeviceThread(Motherboard* mb, DeviceThread& dt);

    MemAddress memorySize;

    std::vector<Cpu*> cpus;

    std::vector<Device*> devices;

    int masterCpu;              //< Cpu index to boot from

    Memory memory;              //< Main memory

    MemAddress reservedSize;    //< Size of reserved memory (the front)

    std::list<DeviceThread> deviceThreads;

    ReportExceptionFunc reportCb;
};

}   // namespace motherboard

#endif // MOTHERBOARD_H
