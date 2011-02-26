/**
 * @file    main.cpp
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#include <stdexcept>
#include <getopt.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include "rapidxml/rapidxml.hpp"

#include "motherboard.h"

using namespace std;
using namespace motherboard;

void displayUsage(char* progName);

/**
 * Parse command line arguments
 *
 * @param[in]   argc
 * @param[in]   argv
 * @param[out]  bios  Filename of Motherboard BIOS to use
 * @return  True if program should continue; otherwise false
 */
bool parseArgs(int argc, char** argv, string& bios);

/**
 * Read a file
 * @param[in]   filename
 * @param[out]  contents
 */
void readFile(const string& filename, string& contents);

/**
 * Read BIOS XML and load it into the Motherboard
 * @param[in]       bios_file   XML BIOS file to load
 * @param[in,out]   mb          Motherboard to load BIOS into
 */
void loadBios(const string& bios_file, Motherboard& mb);

int main(int argc, char** argv)
{
    string bios;
    if (!parseArgs(argc, argv, bios))
        return 0;

    Motherboard mb;
    loadBios(bios, mb);

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

void readFile(const string& filename, string& contents)
{
    ifstream in(filename.c_str());

    if (!in.is_open())
        throw runtime_error("Could not read BIOS file");

    while (in.good())
    {
        string line;
        getline(in, line);
        contents += line;
    }

    in.close();
}

void loadBios(const string& bios_file, Motherboard& mb)
{
    using namespace rapidxml;

    string contents;
    readFile(bios_file, contents);

    /* parse BIOS */
    xml_document<> doc;
    char* contentsBuf = new char[contents.length() + 1];    // deleted @ bottom
    contents.copy(contentsBuf, contents.length() + 1);
    doc.parse<0>(contentsBuf);

    /* load BIOS */
    xml_node<>* mbNode = doc.first_node();

    // memory
    xml_node<>* memNode = mbNode->first_node("memory");
    if (memNode)
    {
        xml_attribute<>* attrSize = memNode->first_attribute("size");
        if (attrSize)
            mb.setMemorySize(atoi(attrSize->value()));
    }
    if (mb.getMemorySize() <= 0)
        cout << "Warning:  Motherboard memory is not set" << endl;

    delete[] contentsBuf;
}
