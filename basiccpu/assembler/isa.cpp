#include "isa.hpp"
#include "program.hpp"
#include "../opcodes.h"

#include <sstream>
#include <stdexcept>
#include <cassert>

using namespace std;

static MemAddress modeEnumToConstant(Instruction::AddrMode mode)
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

static inline MemAddress fourCharsToAddress(const vector<uint8_t>& value)
{
    return value[0] << 24 | value[1] << 16 | value[2] << 8 | value[3] << 0;
}

static inline MemAddress regStringToAddress(const string& regStr)
{
    return regStr == "r1" ? R1 :
           regStr == "r2" ? R2 :
           regStr == "r3" ? R3 :
           regStr == "r4" ? R4 :
           regStr == "r5" ? R5 :
           regStr == "r6" ? R6 :
           0;
}

int Isa::calcInstructionSize(Instruction* instr)
{
    const string& instruction = instr->instruction;
    if (instruction == "cmp")
    {
        if (!instr->args || !instr->args->next)
            throw runtime_error("cmp requires 2 arguments");

        return dynamic_cast<RegisterArgument*>( instr->args->next ) ? 4 : 8;
    }
    else if (instruction == "jmp" || instruction == "je")
    {
        return 4;
    }
    else if (instruction == "call")
    {
        return 4;
    }
    else if (instruction == "ret" || instruction == "sti" || instruction == "rti")
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
        if (!instr->args || !instr->args->next)
            throw runtime_error("mov requires 2 arguments");

        return dynamic_cast<RegisterArgument*>( instr->args->next ) ? 4 : 8;
    }
    else if (instruction == "str")
    {
        if (!instr->args || !instr->args->next)
            throw runtime_error("str requires 2 arguments");

        return dynamic_cast<RegisterArgument*>( instr->args->next ) ? 4 : 8;
    }
    else if (instruction == "memcpy")
    {
        return 8;
    }
    else if (instruction == "clrset" || instruction == "clrsetv")
    {
        return dynamic_cast<RegisterArgument*>( instr->args->next ) ? 4 : 8;
    }
    else if (instruction == "read")
    {
        return 4;
    }
    else if (instruction == "write")
    {
        return 8;
    }
    else if (instruction == "halt" || instruction == "idle")
    {
        return 4;
    }
    else if (instruction == "add")
    {
        return dynamic_cast<RegisterArgument*>( instr->args->next ) ? 4 : 8;
    }
    else if (instruction == "mul")
    {
        return dynamic_cast<RegisterArgument*>( instr->args->next ) ? 4 : 8;
    }
    else if (instruction == "mulw")
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
    if (instruction == "cmp")
    {
        if (!instr->args || !instr->args->next)
            throw runtime_error("cmp requires 2 arguments");

        RegisterArgument* destArg = dynamic_cast<RegisterArgument*>( instr->args );
        const string& reg = destArg->reg;
        MemAddress regBits = regStringToAddress(reg);
        if (!destArg)
            throw runtime_error("First argument must be a register");

        if (RegisterArgument* reg_arg = dynamic_cast<RegisterArgument*>( instr->args->next ))
        {
            MemAddress srcRegArg = regStringToAddress(reg_arg->reg) >> INS_REG;
            generated.push_back(CMP | regBits | REGISTER | srcRegArg);
        }
        else
        {
            generated.push_back(CMP | regBits | IMMEDIATE);
            generated.push_back(program.solveArgumentAddress(instr->args->next));
        }
    }
    else if (instruction == "jmp" || instruction == "je" || instruction == "call")
    {
        if (!instr->args)
        {
            stringstream msg;
            msg << instruction << " must have a destination";
            throw runtime_error(msg.str());
        }

        SymbolArgument* destSymbol = dynamic_cast<SymbolArgument*>( instr->args );
        // TODO:  support other types (like integers)
        if (!destSymbol)
            throw runtime_error("First argument of jmp must be a symbol");

        const MemAddress& destAddress = program.getSymbol(destSymbol->symbolName).getAddress();

        MemAddress diff = destAddress - instr->address;
        if (diff > 0xFFFF || diff < -0xFFFF)
            throw runtime_error("jmp out of range");

        uint16_t bitableDiff = static_cast<uint16_t>( diff );
        MemAddress opcode = instruction == "jmp"  ? JMP :
                            instruction == "je"   ? JE :
                            instruction == "call" ? CALL :
                            0;
        assert(opcode /* don't forget to set `opcode` */);
        generated.push_back(opcode | RELATIVE | bitableDiff);
    }
    else if (instruction == "ret")
    {
        generated.push_back(RET);
    }
    else if (instruction == "sti")
    {
        generated.push_back(STI);
    }
    else if (instruction == "rti")
    {
        generated.push_back(RTI);
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
        MemAddress regBits = regStringToAddress(reg);
        if (!regBits)
        {
            stringstream msg;
            msg << "Invalid register given to `mov`:  " << reg;
            throw runtime_error(msg.str());
        }

        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( regArg->next ))
        {
            // The value returned by regStringToAddress is the register value
            // if the register were a destination.  This register, however, is
            // the source register, which is placed at bit 0.  So, we shift the result back
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(regBits | MOV | REGISTER | srcRegArg);
        }
        else
        {
            generated.push_back(regBits | MOV | IMMEDIATE);
            generated.push_back(program.solveArgumentAddress(regArg->next));
        }
    }
    else if (instruction == "str")
    {
        RegisterArgument* regArg = dynamic_cast<RegisterArgument*>( instr->args );
        if (!regArg)
            throw runtime_error("First argument of str must be a register");
        else if (!regArg->next)
            throw runtime_error("str requires 2 arguments");

        const string& reg = regArg->reg;
        MemAddress regBits = regStringToAddress(reg);
        if (!regBits)
        {
            stringstream msg;
            msg << "Invalid register given to `str`:  " << reg;
            throw runtime_error(msg.str());
        }

        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( regArg->next ))
        {
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(regBits | STR | REGISTER | srcRegArg);
        }
        else
        {
            generated.push_back(regBits | STR | IMMEDIATE);
            generated.push_back(program.solveArgumentAddress(regArg->next));
        }
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
    else if (instruction == "clrset")
    {
        if (!instr->args)
            throw runtime_error("clrset requires 1 argument");

        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr->args ))
        {
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(CLRSET | REGISTER | srcRegArg);
        }
        else if (IntegerArgument* reg_int = dynamic_cast<IntegerArgument*>( instr->args ))
        {
            generated.push_back(CLRSET | IMMEDIATE);
            generated.push_back(reg_int->data);
        }
        else
        {
            throw runtime_error("Invalid argument to clrset instruction");
        }
    }
    else if (instruction == "clrsetv")
    {
        if (!instr->args)
            throw runtime_error("clrsetv requires 1 argument");

        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr->args ))
        {
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(CLRSETV | REGISTER | srcRegArg);
        }
        else if (IntegerArgument* reg_int = dynamic_cast<IntegerArgument*>( instr->args ))
        {
            generated.push_back(CLRSETV | IMMEDIATE);
            generated.push_back(reg_int->data);
        }
        else
        {
            throw runtime_error("Invalid argument to clrset instruction");
        }
    }
    else if (instruction == "read")
    {
        if (!instr->args || !instr->args->next)
            throw runtime_error("read requires 2 arguments");

        RegisterArgument* destArg = dynamic_cast<RegisterArgument*>( instr->args );
        if (!destArg)
            throw runtime_error("First argument of read must be a register");

        MemAddress regBits = regStringToAddress(destArg->reg);

        if (RegisterArgument* reg_arg = dynamic_cast<RegisterArgument*>( destArg->next ))
        {
            throw runtime_error("Second argument cannot be register [not implemented]");
        }
        else if (SymbolArgument* sym_arg = dynamic_cast<SymbolArgument*>( destArg->next ))
        {
            const ImmediateValue& imm = program.getSymbol(sym_arg->symbolName);
            if (imm.instruction)
                throw runtime_error("First argument of `read` cannot be a label");

            MemAddress port = imm.value;
            if (port > 0xFFFF)
                throw runtime_error("The maximum port number given to `read` is 0xFFFF");

            generated.push_back(READ | regBits | IMMEDIATE | port);
        }
        else
            throw runtime_error("Invalid operand to read instruction");
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
    else if (instruction == "idle")
    {
        generated.push_back(IDLE);
    }
    else if (instruction == "add")
    {
        if (!instr->args || !instr->args->next)
            throw runtime_error("add requires 2 arguments");

        RegisterArgument* arg = dynamic_cast<RegisterArgument*>( instr->args );
        if (!arg)
            throw runtime_error("First argument of add must be the destination register");

        const string& reg = arg->reg;
        MemAddress regBits = regStringToAddress(reg);

        Argument* operand = instr->args->next;
        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( operand ))
        {
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(ADD | regBits | REGISTER | srcRegArg);
        }
        else if (SymbolArgument* reg_sym = dynamic_cast<SymbolArgument*>( operand ))
        {
            generated.push_back(ADD | regBits | IMMEDIATE);
            generated.push_back(program.solveArgumentAddress(reg_sym));
        }
        else if (IntegerArgument* reg_int = dynamic_cast<IntegerArgument*>( operand ))
        {
            generated.push_back(ADD | regBits | IMMEDIATE);
            generated.push_back(reg_int->data);
        }
        else
        {
            throw runtime_error("Invalid operand to add instruction");
        }
    }
    else if (instruction == "mul")
    {
        if (!instr->args || !instr->args->next)
            throw runtime_error("mul requires 2 arguments");

        RegisterArgument* arg = dynamic_cast<RegisterArgument*>( instr->args );
        if (!arg)
            throw runtime_error("First argument of mul must be the destination register");

        const string& reg = arg->reg;
        MemAddress regBits = regStringToAddress(reg);

        Argument* operand = instr->args->next;
        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( operand ))
        {
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(MUL | regBits | REGISTER | srcRegArg);
        }
        else if (IntegerArgument* reg_int = dynamic_cast<IntegerArgument*>( operand ))
        {
            generated.push_back(MUL | regBits | IMMEDIATE);
            generated.push_back(reg_int->data);
        }
        else
        {
            throw runtime_error("Invalid operand to mul instruction");
        }
    }
    else if (instruction == "mulw")
    {
        if (!instr->args || !instr->args->next)
            throw runtime_error("mulw requires 2 arguments");

        RegisterArgument* arg = dynamic_cast<RegisterArgument*>( instr->args );
        if (!arg)
            throw runtime_error("First argument of mulw must be the destination register");

        Argument* operand = instr->args->next;
        if (IntegerArgument* reg_int = dynamic_cast<IntegerArgument*>( operand ))
        {
            generated.push_back(MULW | IMMEDIATE | reg_int->data);
        }
        else
        {
            throw runtime_error("Invalid operand to mulw instruction");
        }
    }
    else
    {
        stringstream msg;
        msg << "Unrecognized instruction:  " << instruction;
        throw runtime_error(msg.str());
    }

    return generated;
}
