#include <machine/dladapter.h>
#include <machine/motherboard.h>
#include <machine/cpu.h>
#include <machine/charoutputdevice.h>

#include <stdexcept>
#include <cassert>

using namespace std;
using namespace machine;

void readFile(const char* filename, uint8_t*& contents, int& fileSize);

int getFileLength(FILE* file);

int main()
{
    const static int NUM_CPUS = 4;

    Motherboard* mb = new Motherboard;
    mb->setMemorySize(10240);

    DlAdapter* dlLoader = static_cast<DlAdapter*>( DlAdapter::getInstance() );
    assert(dlLoader);
    for (int i = 0; i < NUM_CPUS; ++i)
    {
        Cpu* cpu = dynamic_cast<Cpu*>( dlLoader->loadDevice("basiccpu/" + DlAdapter::getLibraryName("basiccpu"), *mb) );
        if (cpu)
            mb->addCpu(cpu, i == 0);
    }
    mb->addDevice(new CharOutputDevice);

    // read bios
    uint8_t* bios;
    int      biosSize;
    readFile("basiccpu/bios", bios, biosSize);
    // convert to vector
    vector<uint8_t> biosProgram;
    biosProgram.reserve(biosSize);
    biosProgram.insert(biosProgram.end(), bios, bios + biosSize);
    // set motherboard bios
    mb->setBios(biosProgram, 1000);

    mb->start();

    delete[] bios;
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

    fread(contents, fileSize, 1, file);
    fclose(file);
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
