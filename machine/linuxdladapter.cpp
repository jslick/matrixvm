/**
 * @file    linuxdladapter.cpp
 *
 * Matrix VM
 */

#include "linuxdladapter.h"
#include "motherboard.h"
#include "device.h"

#include <stdexcept>
#include <sstream>
#include <dlfcn.h>

using namespace std;
using namespace machine;

typedef Device* (*GetDeviceFunc)();

LinuxDlAdapter* LinuxDlAdapter::instance = 0;

/* public LinuxDlAdapter */

LinuxDlAdapter::~LinuxDlAdapter()
{
    // close all library handles
    for (map<string, LibHandle>::iterator iter = this->libHandles.begin();
         iter != this->libHandles.end();
         ++iter)
    {
        LibHandle handle = (*iter).second;
        dlclose(handle);
    }
}

Device* LinuxDlAdapter::loadDevice(const string& libraryPath, Motherboard& mb)
{
    LibHandle libHandle = this->getHandle(libraryPath);

    GetDeviceFunc fn = reinterpret_cast<GetDeviceFunc>( dlsym(libHandle, "createDevice") );
    if (dlerror())  // passing up rv
    {
        stringstream msg;
        msg << "Function 'createDevice' is missing from " << libraryPath << endl;
        throw std::runtime_error(msg.str());
    }

    Device* dev = (*fn)();

    return dev;
}

/* protected LinuxDlAdapter */

LibHandle LinuxDlAdapter::getHandle(const string& libraryPath)
{
    map<string, LibHandle>::iterator iter = this->libHandles.find(libraryPath);
    if (iter == this->libHandles.end()) // library is not yet loaded
        return this->loadLibrary(libraryPath);
    else    // library is already loaded
        return (*iter).second;
}

LibHandle LinuxDlAdapter::loadLibrary(const std::string& libraryPath)
{
    void* libHandle = dlopen(libraryPath.c_str(), RTLD_LAZY);
    if (!libHandle)
    {
        stringstream msg;
        msg << "Could not open library ";
        msg << libraryPath << ":\n" << dlerror() << endl;
        throw std::runtime_error(msg.str());
    }
    this->libHandles[libraryPath] = libHandle;

    return libHandle;
}

/* public static LinuxDlAdapter */

LinuxDlAdapter* LinuxDlAdapter::getInstance()
{
    if (!instance)
        instance = new LinuxDlAdapter();

    return instance;
}

void LinuxDlAdapter::cleanup()
{
    if (instance)
    {
        delete instance;
        instance = 0;
    }
}

string LinuxDlAdapter::getLibraryName(const string& baseName)
{
    return "lib" + baseName + ".so";
}
