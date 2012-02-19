/**
 * @file    linuxdladapter.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#ifndef LINUX_DL_ADAPTER_H
#define LINUX_DL_ADAPTER_H

#include "device.h"
#include "motherboard.h"

#include <string>
#include <map>

// yes, hiding pointer; it is entirely contained
typedef void* LibHandle;

/**
 * @class LinuxDlAdapter
 *
 * Dynamic library adapter for Linux
 *
 * The destructor closes all loaded library handles.
 */
class LinuxDlAdapter
{
public:

    /**
     * Closes all loaded library handles
     */
    ~LinuxDlAdapter();

    /**
     * Create an instance of the device in a library
     * @param[in]   libraryPath Library to load device from
     * @param[in]   mb          Motherboard instance
     * @return  An instance of the device
     *
     * @note    The library at libraryPath must have the createDevice method
     *          with the proper function signature.
     */
    machine::Device* loadDevice(const std::string& libraryPath, machine::Motherboard& mb, void* args);

protected:

    LinuxDlAdapter() { }    /* enforce singleton */

    /**
     * Get the handle to the library
     * @param[in]   libraryPath
     * @return  A new or used handle to the loaded library
     */
    LibHandle getHandle(const std::string& libraryPath);

    /**
     * Load a library
     * @param[in]   libraryPath
     * @return  The handle to the newly loaded library
     * @pre The library should not be already loaded
     */
    LibHandle loadLibrary(const std::string& libraryPath);

    static LinuxDlAdapter* instance;    //< singleton adapter

private:

    std::map<std::string, LibHandle> libHandles;

public:

    /**
     * @return  The Adapter singleton
     * @note    If this function is called 1 or more times, cleanup() must be
     *          called at the end of the program to prevent memory leaks and
     *          open handles.
     */
    static LinuxDlAdapter* getInstance();

    /**
     * Delete the adapter singleton and close handles
     */
    static void cleanup();

    /**
     * @param[in]   baseName    The base name of the library.  For example, in
     *                          Linux, baseName="tea" would return
     *                          "libtea.so".
     * @return The system-specific library name from the base name
     */
    static std::string getLibraryName(const std::string& baseName);
};

#endif // LINUX_DL_ADAPTER_H
