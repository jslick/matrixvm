/**
 * @file    dladapter.h
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#ifndef DL_ADAPTER_H
#define DL_ADAPTER_H

#ifdef __linux
#include "linuxdladapter.h"
/**
 * @class DlAdapter
 *
 * A dynamic library adapter for the target system
 *
 * The adapter maintains and cleans up resources
 */
class DlAdapter : public LinuxDlAdapter
{ };
#else

#endif

#endif // DL_ADAPTER_H
