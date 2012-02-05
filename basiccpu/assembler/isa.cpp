#include "isa.hpp"
#include "program.hpp"
#include "../opcodes.h"

#include <sstream>
#include <stdexcept>
#include <cassert>

using namespace std;

MemAddress modeEnumToConstant(Instruction::AddrMode mode)
{
    switch (mode)
    {
    case Instruction::AddrMode::Immediate:
        return IMMEDIATE;
    case Instruction::AddrMode::Absolute:
        return ABSOLUTE;
    case Instruction::AddrMode::Relative:
        return RELATIVE;
    case Instruction::AddrMode::Indirect:
        assert(false /* NOT IMPLEMENTED */);
    default:
        assert(false);
    }

    return 0;
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
        if (!instr->args)
            throw runtime_error("db must have arguments");

        DataArgument* data = dynamic_cast<DataArgument*>( instr->args );
        assert(data);
        int size = data->data.size();
        int sizeMod = size % 4;
        if (sizeMod)
            size += 4 - sizeMod;
        return size;
    }
    else if (instruction == "mov")
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
        if (!instr->args)
            throw runtime_error("jmp must have a destination");

        SymbolArgument* destSymbol = dynamic_cast<SymbolArgument*>( instr->args );
        // TODO:  support other types (like integers)
        if (!destSymbol)
            throw runtime_error("First argument of jmp must be a symbol");

        const MemAddress& destAddress = program.getSymbol(destSymbol->symbolName).getAddress();

        MemAddress diff = destAddress - instr->address;
        if (diff > 0xFFFF || diff < -0xFFFF)
            throw runtime_error("jmp out of range");

        generated.push_back(JMP | RELATIVE | diff);
    }
    else if (instruction == "db")   // not really part of the isa, but that's okay
    {
        if (!instr->args)
            throw runtime_error("db must have arguments");

        DataArgument* data = dynamic_cast<DataArgument*>( instr->args );
        assert(data);
        const vector<uint8_t>& value = data->data;
        for (unsigned int i = 0; i < value.size(); i += 4)
        {
            MemAddress instruction = value[i+0] << 24 |
                                     value[i+1] << 16 |
                                     value[i+2] <<  8 |
                                     value[i+3] <<  0;
            generated.push_back(instruction);
        }
    }
    else if (instruction == "mov")
    {
        RegisterArgument* regArg = dynamic_cast<RegisterArgument*>( instr->args );
        if (!regArg)
            throw runtime_error("First argument of mov must be a register");
        else if (!regArg->next)
            throw runtime_error("mov requires 2 arguments");

        const string& reg = regArg->reg;
        MemAddress regBits = reg == "r1" ? R1 :
                             reg == "r2" ? R2 : 0;
        if (!regBits)
        {
            stringstream msg;
            msg << "Invalid register given to `load`:  " << reg;
            throw runtime_error(msg.str());
        }
        generated.push_back(regBits | MOV | IMMEDIATE);
        generated.push_back(program.solveArgumentAddress(regArg->next));
    }
    else if (instruction == "memcpy")
    {
        if (!instr->args)
            throw runtime_error("memcpy requires 1 argument");

        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr->args ))
        {
            assert(false /* not yet implemented */);
        }
        else
        {
            generated.push_back(MEMCPY | IMMEDIATE);
            generated.push_back(program.solveArgumentAddress(instr->args));
        }
    }
    else if (instruction == "write")
    {
        if (!instr->args || !instr->args->next)
            throw runtime_error("write requires 2 arguments");

        // TODO:  support register as either argument, instead of just immediate values

        // Generate instruction
        SymbolArgument* portArg = dynamic_cast<SymbolArgument*>( instr->args );
        assert(portArg);

        const ImmediateValue& imm = program.getSymbol(portArg->symbolName);
        if (imm.instruction)
            throw runtime_error("First argument of `write` cannot be a label");

        MemAddress port = imm.value;
        if (port > 0xFFFF)
            throw runtime_error("The maximum port number given to `write` is 0xFFFF");

        generated.push_back(WRITE | port);

        // Generate the 'what` operand (32-bit)
        // TODO:  support symbol as operand, instead of just integer
        IntegerArgument* intArg = dynamic_cast<IntegerArgument*>( portArg->next );
        assert(intArg);
        generated.push_back(intArg->data);
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
