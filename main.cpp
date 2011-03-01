/**
 * @file    main.cpp
 * @author  Jason Eslick <jasoneslick@ku.edu>
 *
 * Matrix VM
 */

#include <stdexcept>
#include <getopt.h>
#include <string>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp> // path
#include "rapidxml/rapidxml.hpp"

#include "motherboard.h"
#include "cpu.h"
#include "dladapter.h"

using namespace std;
using namespace machine;

#define RET_ERR_RT      -1
#define RET_ERR_ARG     -2
#define RET_ERR_FIO     -3
#define RET_ERR_BIOS    -4

void displayUsage(char* progName);

/**
 * @param[in]   bios    Filename of BIOS XML file
 */
int boot(const string& bios);

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

/**
 * Load `count` CPUs into the motherboard
 * @param[in]       nativeLibPath   Complete native path to library of CPU
 * @param[in,out]   mb              Motherboard to load CPUs into
 * @param[in]       count           Number of instances to create of the
 *                                  particular CPU
 * @param[in]       master          The index of the 'master' CPU.  For
 *                                  example, if count=2, specify master=1 to
 *                                  make the second CPU instance the master CPU
 */
void loadCpu(const string& nativeLibPath,
             Motherboard& mb,
             int count, int master);

int main(int argc, char** argv)
{
    string bios;
    if (!parseArgs(argc, argv, bios))
        return 0;

    int rv = boot(bios);

    DlAdapter::cleanup();
    return rv;
}

void displayUsage(char* progName)
{
    cout << "Usage:  \n  " << progName << " bios_file" << endl;
}

int boot(const string& bios)
{
    Motherboard mb;
    try
    {
        loadBios(bios, mb);
        mb.start();
    } catch (rapidxml::parse_error e)
    {
        cerr << "BIOS could not be parsed:\n" << e.what() << endl;
        return RET_ERR_BIOS;
    } catch (runtime_error e)
    {
        cerr << "A runtime error occurred:\n" << e.what() << endl;
        return RET_ERR_RT;
    } catch (exception e)
    {
        cerr << "A runtime error occurred:\n" << e.what() << endl;
        return RET_ERR_RT;
    }

    return EXIT_SUCCESS;
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
    contentsBuf[contents.length()] = 0; // end with null character
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

    // CPUs
    for (xml_node<>* cpuNode = mbNode->first_node("cpu");
         cpuNode;
         cpuNode = cpuNode->next_sibling("cpu"))
    {
        // parse
        int count  = 1;
        int master = -1;
        string cpuLibDir;
        string cpuLibName;

        xml_attribute<>* attrDir = cpuNode->first_attribute("dir");
        if (attrDir && attrDir->value())
            cpuLibDir = attrDir->value();

        xml_attribute<>* attrLoad = cpuNode->first_attribute("load");
        if (attrLoad)
        {
            char* cpuLoad = attrLoad->value();
            if (cpuLoad)
                cpuLibName = cpuLoad;
        }
        else
            cout << "Warning:  CPU must have a load attribute" << endl;

        xml_attribute<>* attrCount = cpuNode->first_attribute("count");
        if (attrCount && attrCount->value())
            count = atoi(attrCount->value());

        xml_attribute<>* attrMaster = cpuNode->first_attribute("master");
        if (attrMaster && attrMaster->value())
            master = atoi(attrMaster->value());

        boost::filesystem::path libPath(cpuLibDir + '/' +
                                        DlAdapter::getLibraryName(cpuLibName));

        // create CPUs
        try
        {
            loadCpu(libPath.native_file_string(), mb, count, master);
        } catch (runtime_error e)
        {
            cout << "Could not load CPU:\n" << e.what();
        }
    }

    delete[] contentsBuf;
}

void loadCpu(const string& nativeLibPath, Motherboard& mb, int count, int master)
{
    DlAdapter* dlLoader = static_cast<DlAdapter*>( DlAdapter::getInstance() );
    assert(dlLoader);
    for (int i = 0; i < count; ++i)
    {
        Cpu* cpu = dynamic_cast<Cpu*>( dlLoader->loadDevice(nativeLibPath, mb) );
        if (cpu)
            mb.addCpu(cpu, i == master);
    }
}
