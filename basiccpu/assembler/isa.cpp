#include "isa.h"
#include "program.h"
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
        if (instr->args.size() < 1)
            throw runtime_error("db must have arguments");

        DataArgument* data = dynamic_cast<DataArgument*>( instr->args[0] );
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
        if (instr->args.size() < 1)
            throw runtime_error("jmp must have a destination");

        SymbolArgument* destSymbol = dynamic_cast<SymbolArgument*>( instr->args[0] );
        const Instruction* destInstr = program.getInstructionAtLabel(destSymbol->symbolName);
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

        DataArgument* data = dynamic_cast<DataArgument*>( instr->args[0] );
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
        if (instr->args.size() < 2)
            throw runtime_error("mov requires 2 arguments");

        RegisterArgument* regArg = dynamic_cast<RegisterArgument*>( instr->args[0] );
        const string& reg = regArg->reg;
        MemAddress instruction = reg == "r1" ? R1 :
                                 reg == "r2" ? R2 : 0;
        if (!instruction)
        {
            stringstream msg;
            msg << "Invalid register given to `load`:  " << reg;
            throw runtime_error(msg.str());
        }
        instruction |= MOV | modeEnumToConstant(instr->addrMode);
        generated.push_back(instruction);

        MemAddress srcInstructionMem;

        if (SymbolArgument* arg_sym = dynamic_cast<SymbolArgument*>( instr->args[1] ))
        {
            const Instruction* operandIns = program.getInstructionAtLabel(arg_sym->symbolName);
            assert(operandIns);
            srcInstructionMem = operandIns->address;
        }
        else if (DifferenceArgument* arg_diff = dynamic_cast<DifferenceArgument*>( instr->args[1] ))
        {
            const Instruction* operandIns[2];
            operandIns[0] = program.getInstructionAtLabel(arg_diff->symbols[0]);
            operandIns[1] = program.getInstructionAtLabel(arg_diff->symbols[1]);
            assert(operandIns[0]);
            assert(operandIns[1]);

            srcInstructionMem = operandIns[0]->address - operandIns[1]->address;
        }
        else
        {
            assert(false /* not yet implemented */);
        }

        generated.push_back(srcInstructionMem);
    }
    else if (instruction == "memcpy")
    {
        if (instr->args.size() < 1)
            throw runtime_error("memcpy requires 1 argument");

        generated.push_back(MEMCPY | modeEnumToConstant(instr->addrMode));

        MemAddress destAddress = 0;

        if (DataArgument* arg_data = dynamic_cast<DataArgument*>( instr->args[0] ))
        {
            destAddress = fourCharsToAddress(arg_data->data);
        }
        else
        {
            assert(false /* not yet implemented */);
        }
        generated.push_back(destAddress);
    }
    else if (instruction == "write")
    {
        if (instr->args.size() < 2)
            throw runtime_error("write requires 2 arguments");

        DataArgument* portArg = dynamic_cast<DataArgument*>( instr->args[0] );
        assert(portArg);
        vector<uint8_t> writePort = portArg->data;
        MemAddress opcode = WRITE | (writePort[0] << 8) | writePort[1];
        generated.push_back(opcode);

        DataArgument* whatArg = dynamic_cast<DataArgument*>( instr->args[1] );
        generated.push_back(fourCharsToAddress(whatArg->data));
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
