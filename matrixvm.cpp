#include <machine/dladapter.h>
#include <machine/motherboard.h>
#include <machine/cpu.h>
#include <dev/basicinterruptcontroller.h>
#include <dev/timerdevice.h>
#include <dev/displaydevice.h>
#include <dev/x11displaymanager.h>
#include <dev/nulldisplaymanager.h>

#include <getopt.h>
#include <stdexcept>
#include <cassert>

using namespace std;
using namespace machine;

struct Options
{
    int graphics;

    Options()
    : graphics(1)
    { }
} options;

void readFile(const char* filename, uint8_t*& contents, int& fileSize);

int getFileLength(FILE* file);

void motherboardException(Motherboard& mb, exception& e)
{
    printf("Motherboard exception:  %s\n", e.what());
    mb.abort();
}

void parseArgs(int argc, char** argv)
{
    int c;

    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"nographic",   no_argument,    &options.graphics, 0},
            /* These options don't set a flag.
               We distinguish them by their indices. */
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;
        c = getopt_long(argc, argv, "", long_options, &option_index);
        /* Detect the end of the options. */
        if (c < 0)
            break;

        switch (c)
        {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
                break;
            printf("option %s", long_options[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;

        default:
            abort();
        }
    }

    if (optind < argc)
    {
        /* remaining command line arguments (not options) */
    }
}

int main(int argc, char** argv)
{
    parseArgs(argc, argv);

    const static int NUM_CPUS = 4;

    Motherboard* mb = new Motherboard;
    mb->setMemorySize(10 * 1024 * 1024);
    InterruptController* ic = new BasicInterruptController(*mb);
    mb->setInterruptController(ic);
    mb->setExceptionReport(motherboardException);

    /* Initialize dynamic library loader */
    DlAdapter* dlLoader = static_cast<DlAdapter*>( DlAdapter::getInstance() );
    assert(dlLoader);

    /* Create CPUs from dynamically linked libraries */
    for (int i = 0; i < NUM_CPUS; ++i)
    {
        Cpu* cpu = dynamic_cast<Cpu*>(
            dlLoader->loadDevice("basiccpu/" + DlAdapter::getLibraryName("basiccpu"), *mb, 0)
            );
        if (cpu)
            mb->addCpu(cpu, i == 0);
        else
            throw runtime_error("Could not load cpu device");
    }

    /* Initialize interval timer */
    TimerDevice* timer = new TimerDevice;
    mb->addDevice(timer);

    /* Initialize guest-to-host display output device */
    DisplayDeviceArgs ddargs;
    if (options.graphics)
        ddargs.displayManager = new X11DisplayManager;
    else
        ddargs.displayManager = new NullDisplayManager;
    Device* displayDevice = dynamic_cast<Device*>(
        dlLoader->loadDevice("dev/" + DlAdapter::getLibraryName("displaydevice"), *mb, &ddargs)
        );
    if (displayDevice)
        mb->addDevice(displayDevice);
    else
        throw runtime_error("Could not load display device");

    /* Initialize guest-to-host character output device */
    Device* charOutputDevice = dynamic_cast<Device*>(
        dlLoader->loadDevice("dev/" + DlAdapter::getLibraryName("charoutputdevice"), *mb, 0)
        );
    if (charOutputDevice)
        mb->addDevice(charOutputDevice);
    else
        throw runtime_error("Could not load character output device");

    /* read bios */
    uint8_t* bios;
    int      biosSize;
    readFile("basiccpu/bios", bios, biosSize);
    // convert to vector
    vector<uint8_t> biosProgram;
    biosProgram.reserve(biosSize);
    biosProgram.insert(biosProgram.end(), bios, bios + biosSize);
    // set motherboard bios
    mb->setBios(biosProgram, 7000000);

    /* Start the emulator */
    if (mb->start());
    else
        fprintf(stderr, "Emulator aborted\n");

    /* Cleanup */
    delete ddargs.displayManager;
    delete[] bios;
    delete ic;
    delete mb;
    dlLoader->cleanup();
    return 0;
}

// An abomination mix of C and C++; sry
void readFile(const char* filename, uint8_t*& contents, int& fileSize)
{
    FILE* file = fopen(filename, "rb");
    if (!file)
        throw runtime_error("Could not open bios file");

    fileSize = getFileLength(file);
    contents = new uint8_t[fileSize];

    size_t numItems = fread(contents, fileSize, 1, file);
    fclose(file);
    if (numItems < 1)
        throw runtime_error("Failed to read bios file");
}

int getFileLength(FILE* file)
{
    // Save current position
    int fpi = ftell(file);

    fseek(file, 0, SEEK_END);
    int length = ftell(file);

    // Return fpi
    fseek(file, fpi, SEEK_SET);

    return length;
}
