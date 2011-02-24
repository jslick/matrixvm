/**
 * @file    main.cpp
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#include <getopt.h>
#include <cstring>
#include <iostream>

using namespace std;

void displayUsage(char* progName);

/**
 * Parse command line arguments
 *
 * @param[in]  argc
 * @param[in]  argv
 * @param[out] bios  Filename of Motherboard BIOS to use
 * @return  True if program should continue; otherwise false
 */
bool parseArgs(int argc, char** argv, string& bios);

int main(int argc, char** argv)
{
    string bios;
    if (!parseArgs(argc, argv, bios))
    {
        return 0;
    }

    return 0;
}

void displayUsage(char* progName)
{
    cout << "Usage:  \n  " << progName << " bios_file" << endl;
}

bool parseArgs(int argc, char** argv, string& bios)
{
    const char* OPT_STRING = "::h?";
    const struct option LONG_OPTS[] =
    {
        // long option, arg_type, flag, short option
        { 0, no_argument, 0, 0 }
    };
    int optint;

    while(true)
    {
        int opt = getopt_long(argc, argv, OPT_STRING, LONG_OPTS, &optint);
        if (opt < 0)
            break;

        switch(opt)
        {
        case 'h':
        case '?':
            displayUsage(argv[0]);
            break;

        case 0:     /* long option without a short arg */
            break;

        default:
            /* won't actually get here. */
            break;
        }
    }

    if (optind < argc)
    {
        bios = argv[optind];
        return true;
    }
    else
    {
        displayUsage(argv[0]);
        return false;
    }
}
