#include "assembler_util.hpp"

#include <cstring>
#include <queue>
#include <stdexcept>
#include <cassert>

using namespace std;

Options options;

Isa isa;
Program program(isa, 7000000 /* offset */);

std::queue<char*> heapStrings;  // heap-allocated strings to free
std::queue<Argument*> args;     // heap-allocated Arguments to delete

// An address can have multiple labels.  Example:
// mystring:
//     db "Hello World!" 0
// mystring_length:
// main_routine:
//     ... code ...
std::queue<std::string> currentLabels;

void setSymbol(const char* symbolName, Argument* value)
{
    program.setSymbol(symbolName, program.solveArgumentAddress(value));
}

Instruction* addInstruction(const char* opcode, Argument* args)
{
    if (options.debugFlag)
        printf("Instruction:  %s\n", opcode);

    Instruction* instr = program.createInstruction(opcode);
    instr->args = args;

    // Consume all current labels
    while (!currentLabels.empty())
    {
        program.setSymbol(currentLabels.front(), instr);
        currentLabels.pop();
    }

    return instr;
}

Instruction* addDataInstruction(const char* directive, Argument* dataArgs)
{
    DataArgument* dataArgList = dynamic_cast<DataArgument*>( dataArgs );
    assert(dataArgList);
    DataArgument* argList = collapseDataArguments(dataArgList);

    if (strncmp(directive, "db", 2) == 0)
        compress8DataArguments(*dataArgList);
    else if (strncmp(directive, "dw", 2) == 0)
        assert(0 /* not yet implemented */);

    return addInstruction(directive, argList);
}

Argument* createBytes(Argument* symbolArg)
{
    MemAddress numBytes = program.solveArgumentAddress(symbolArg);
    DataArgument* argList = new DataArgument(vector<uint32_t>(numBytes, 0));
    return inventoryArgument(argList);
}

void addCurrentLabel(const std::string& labelName)
{
    if (options.debugFlag)
        printf("Label:  %s\n", labelName.c_str());

    currentLabels.push(labelName);
}

Argument* inventoryArgument(Argument* newArg)
{
    args.push(newArg);
    return newArg;
}

Argument* appendArgument(Argument* list, Argument* arg)
{
    if (!list)
        throw runtime_error("appendArgument list cannot be null");

    Argument* ptr = list;
    while (ptr->next)
        ptr = ptr->next;
    ptr->next = arg;

    return list;
}

DataArgument* collapseDataArguments(DataArgument* list)
{
    assert(list);

    for (Argument* arg = list->next; arg; arg = arg->next)
    {
        DataArgument* dataArg = dynamic_cast<DataArgument*>( arg );
        assert(dataArg);

        for (unsigned int i = 0; i < dataArg->data.size(); i++)
        {
            list->data.push_back(dataArg->data[i]);
        }
    }

    list->next = 0;
    // Assuming orphaned Arguments will be cleaned-up by caller; so not freeing
    // memory here

    return list;
}

void compress8DataArguments(DataArgument& arg)
{
    int idx = 0;
    for (unsigned int i = 0; i < arg.data.size();)
    {
        uint8_t bit3 = static_cast<uint8_t>( arg.data[i++] );
        uint8_t bit2 = 0;
        uint8_t bit1 = 0;
        uint8_t bit0 = 0;
        if (i < arg.data.size())
        {
            bit2 = static_cast<uint8_t>( arg.data[i++] );
            if (i < arg.data.size())
            {
                bit1 = static_cast<uint8_t>( arg.data[i++] );
                if (i < arg.data.size())
                    bit0 = static_cast<uint8_t>( arg.data[i++] );
            }
        }

        arg.data[idx++] = bit3 << 24 | bit2 << 16 | bit1 << 8 | bit0;
    }
    arg.data.erase(arg.data.begin() + idx, arg.data.end());
}

std::vector<uint32_t> stringToVector(const char* str, bool ensureNull)
{
    vector<uint32_t> rv;

    for (const char* ch = str; *ch; ch++)
        rv.push_back(*ch);

    if (ensureNull)
        rv.push_back(0);

    return rv;
}

char* inventoryString(char* heapString)
{
    heapStrings.push(heapString);
    return heapString;
}

void cleanupStrings()
{
    while (!heapStrings.empty())
    {
        free(heapStrings.front());
        heapStrings.pop();
    }
}

void cleanupArgs()
{
    while (!args.empty())
    {
        delete args.front();
        args.pop();
    }
}
