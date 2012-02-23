#include "assembler_util.hpp"

#include <cstring>
#include <queue>
#include <stdexcept>
#include <cassert>

using namespace std;

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
    #if DEBUG
    printf("Instruction:  %s\n", opcode);
    #endif

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
    Argument* argList = collapseDataArguments(dataArgList, true /* pad to align */);
    return addInstruction(directive, argList);
}

void addCurrentLabel(const std::string& labelName)
{
    #if DEBUG
    printf("Label:  %s\n", labelName.c_str());
    #endif

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

DataArgument* collapseDataArguments(DataArgument* list, bool ensureAlignment)
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

    if (ensureAlignment)
    {
        int mod = list->data.size() % 4;
        for (int i = 0; i < 4 - mod; i++)
            list->data.push_back(0);
    }

    return list;
}

std::vector<uint8_t> stringToVector(const char* str, bool ensureNull, bool ensureAlignment)
{
    vector<uint8_t> rv;

    for (const char* ch = str; *ch; ch++)
        rv.push_back(*ch);

    if (ensureNull)
        rv.push_back(0);

    if (ensureAlignment)
    {
        int mod = rv.size() % 4;
        for (int i = 0; i < 4 - mod; i++)
            rv.push_back(0);
    }

    return rv;
}

std::vector<uint8_t> int32ToVector(uint32_t val)
{
    vector<uint8_t> rv(4);
    // remember:  big-endian
    rv.push_back((val & 0xFF000000) >> 24);
    rv.push_back((val & 0x00FF0000) >> 16);
    rv.push_back((val & 0x0000FF00) >>  8);
    rv.push_back((val & 0x000000FF) >>  0);

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
