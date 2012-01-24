#include "isa.h"
#include "program.h"
#include "../opcodes.h"

#include <sstream>
#include <stdexcept>
#include <cassert>

using namespace std;

inline string vectToString(vector<uint8_t>& vect)
{
    string str;
    str.append(reinterpret_cast<char*>( &vect.front() ));

    return str;
}

inline MemAddress fourCharsToAddress(const vector<uint8_t>& value)
{
    return value[0] << 24 | value[1] << 16 | value[2] << 8 | value[3] << 0;
}

int Isa::calcInstructionSize(Instruction* instr)
{
    const string& instruction = instr->instruction;
    if (instruction == "jmp")
    {
        return 4;
    }
    else if (instruction == "db")
    {
        if (instr->args.size() < 1)
            throw runtime_error("db must have arguments");
        int size = instr->args[0].value.size();
        int sizeMod = size % 4;
        if (sizeMod)
            size += (4 - sizeMod);
        return size;
    }
    else if (instruction == "load")
    {
        return 8;
    }
    else if (instruction == "memcpy")
    {
        return 8;
    }
    else if (instruction == "write")
    {
        return 8;
    }
    else if (instruction == "halt")
    {
        return 4;
    }
    else
    {
        stringstream msg;
        msg << "Unrecognized instruction:  " << instruction;
        throw runtime_error(msg.str());
    }
}

vector<MemAddress> Isa::generateInstructions(const Program& program, Instruction* instr)
{
    vector<MemAddress> generated;

    const string& instruction = instr->instruction;
    if (instruction == "jmp")
    {
        if (instr->args.size() < 1)
            throw runtime_error("jmp must have a destination");

        const Instruction* destInstr = program.getInstructionAtLabel(vectToString(instr->args[0].value));
        assert(destInstr);

        MemAddress diff = destInstr->address - instr->address;
        if (diff > 0xFFFF || diff < -0xFFFF)
            throw runtime_error("jmp out of range");

        generated.push_back(JMP | RELATIVE | diff);
    }
    else if (instruction == "db")   // not really part of the isa, but that's okay
    {
        if (instr->args.size() < 1)
            throw runtime_error("db must have arguments");

        const vector<uint8_t>& value = instr->args[0].value;
        for (unsigned int i = 0; i < value.size(); i += 4)
        {
            MemAddress instruction = value[i+0] << 24 |
                                     value[i+1] << 16 |
                                     value[i+2] <<  8 |
                                     value[i+3] <<  0;
            generated.push_back(instruction);
        }
    }
    else if (instruction == "load")
    {
        if (instr->args.size() < 2)
            throw runtime_error("Load requires 2 arguments");

        string reg = vectToString(instr->args[0].value);
        MemAddress instruction = reg == "r1" ? R1 :
                                 reg == "r2" ? R2 : 0;
        if (!instruction)
        {
            stringstream msg;
            msg << "Invalid register given to `load`:  " << reg;
            throw runtime_error(msg.str());
        }
        instruction |= LOAD;

        Argument::Type srcType = instr->args[1].type;

        string src;
        const Instruction* srcInstruction;
        MemAddress srcInstructionMem;

        switch (srcType)
        {
        case Argument::Type::Symbol:
            instruction |= IMMEDIATE;

            try {
                src = vectToString(instr->args[1].value);
                srcInstruction = program.getInstructionAtLabel(src);
            }
            catch (out_of_range& e) {
                stringstream msg;
                msg << "Label not found:  " << src;
                throw runtime_error(msg.str());
            }
            assert(srcInstruction);
            srcInstructionMem = srcInstruction->address;
            break;
        case Argument::Type::Immediate:
            instruction |= IMMEDIATE;

            srcInstructionMem = fourCharsToAddress(instr->args[1].value);
            break;
        default:
            assert(false /* not yet implemented */);
        }
        generated.push_back(instruction);
        generated.push_back(srcInstructionMem);
    }
    else if (instruction == "memcpy")
    {
        if (instr->args.size() < 1)
            throw runtime_error("memcpy requires 1 argument");

        Argument::Type destType = instr->args[0].type;
        const vector<uint8_t>& dest = instr->args[0].value;
        MemAddress destAddress = 0;
        switch (destType)
        {
        case Argument::Type::Immediate:
            generated.push_back(MEMCPY | IMMEDIATE);

            destAddress = fourCharsToAddress(dest);
            break;
        default:
            assert(false /* not yet implemented */);
        }
        generated.push_back(destAddress);
    }
    else if (instruction == "write")
    {
        if (instr->args.size() < 2)
            throw runtime_error("write requires 2 arguments");

        vector<uint8_t> writePort = instr->args[0].value;
        MemAddress opcode = WRITE | (writePort[0] << 8) | writePort[1];
        generated.push_back(opcode);
        generated.push_back(fourCharsToAddress(instr->args[1].value));
    }
    else if (instruction == "halt")
    {
        generated.push_back(HALT);
    }
    else
    {
        stringstream msg;
        msg << "Unrecognized instruction:  " << instruction;
        throw runtime_error(msg.str());
    }

    return generated;
}
