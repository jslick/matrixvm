#include "isa.hpp"
#include "program.hpp"
#include "../opcodes.h"

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cassert>

using namespace std;

// Pseudo-opcodes
#define DB 0x01
#define DW 0x02
#define DD 0x03

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
    return regStr == "r1" ? REG_R1 :
           regStr == "r2" ? REG_R2 :
           regStr == "r3" ? REG_R3 :
           regStr == "r4" ? REG_R4 :
           regStr == "r5" ? REG_R5 :
           regStr == "r6" ? REG_R6 :
           regStr == "r7" ? REG_R7 :
           regStr == "sp" ? REG_SP :
           regStr == "lr" ? REG_LR :
           regStr == "dl" ? REG_DL :
           regStr == "st" ? REG_ST :
           0;
}

Isa::Isa()
: opcodeTableLoaded(false), instructionSizeTableLoaded(false)
{ }

MemAddress Isa::getOpcode(const std::string& instruction)
{
    if (!this->opcodeTableLoaded)
        this->loadOpcodeTable();

    try
    {
        return this->opcodeTable.at(instruction);
    }
    catch (out_of_range& e)
    {
        stringstream msg;
        msg << "No opcode `" << instruction << "` in opcode table";
        throw runtime_error(msg.str());
    }
}

int Isa::calcInstructionSize(Instruction& instr)
{
    if (!this->instructionSizeTableLoaded)
        this->loadInstructionSizeTable();

    try
    {
        int size = this->instructionSizeTable.at(instr.opcode);
        if (size)
            return size;
        // else:  it needs to be determined according to the instructions operands
    }
    catch (out_of_range& e)
    {
        stringstream msg;
        msg << "Missing instruction size for `" << reverseOpcodeTable.at(instr.opcode) << "`";
        throw runtime_error(msg.str());
    }

    auto arg1IsRegister = [this,&instr]() -> bool {
        if (!instr.args)
        {
            stringstream msg;
            msg << "Missing argument for `" << reverseOpcodeTable.at(instr.opcode) << "`";
            throw runtime_error(msg.str());
        }
        return dynamic_cast<RegisterArgument*>( instr.args ) ? true : false;
    };

    auto arg2IsRegister = [this,&instr]() -> bool {
        if (!instr.args)
        {
            stringstream msg;
            msg << "Missing argument for `" << reverseOpcodeTable.at(instr.opcode) << "`";
            throw runtime_error(msg.str());
        }
        if (!instr.args->next)
        {
            stringstream msg;
            msg << "Missing 2nd argument for " << reverseOpcodeTable.at(instr.opcode);
            throw runtime_error(msg.str());
        }
        return dynamic_cast<RegisterArgument*>( instr.args->next ) ? true : false;
    };

    switch (instr.opcode)
    {
    case DB:
    case DW:
    case DD:
    {
        DataArgument* data = dynamic_cast<DataArgument*>( instr.args );
        if (!data)
            throw runtime_error("db/dw/dd requires at least 1 data argument");
        return data->data.size() * 4;
    }

    case CMP:
        return arg2IsRegister() ? 4 : 8;

    case MOV:
        return arg2IsRegister() ? 4 : 8;

    case LOAD:
    case LOADW:
    case LOADB:
    case STR:
        return arg2IsRegister() ? 4 : 8;
    case PUSH:
        return arg1IsRegister() ? 4 : 8;
    case CLRSET:
    case CLRSETV:
    case DRWSQ:
        return arg1IsRegister() ? 4 : 8;

    case ADD:
    case SUB:
    case MUL:
    case AND:
    case OR:
    case NOT:
        return arg2IsRegister() ? 4 : 8;

    default:
        stringstream msg;
        msg << "Missing dynamic instruction size for `" << reverseOpcodeTable.at(instr.opcode) << "`";
        throw runtime_error(msg.str());
        assert(0 /* should have returned before entering the switch */);
    }
}

vector<MemAddress> Isa::generateInstructions(const Program& program, Instruction& instr)
{
    vector<MemAddress> generated;

    auto validateNumArguments = [this,&instr](int num) {
        Argument* cur = instr.args;
        for (int i = 0; i < num; i++)
        {
            if (!cur)
            {
                stringstream msg;
                msg << "Missing argument " << (i + 1) << " to " << reverseOpcodeTable.at(instr.opcode);
                throw runtime_error(msg.str());
            }
            cur = cur->next;
        }
    };

    auto getFirstReg = [this,&instr]() -> RegisterArgument* {
        RegisterArgument* arg = dynamic_cast<RegisterArgument*>( instr.args );
        if (!arg)
        {
            stringstream msg;
            msg << "First argument of `" << reverseOpcodeTable.at(instr.opcode) << "` must be a register";
            throw runtime_error(msg.str());
        }
        return arg;
    };

    auto argToRegBits = [this,&instr](RegisterArgument& arg) -> MemAddress {
        const string& reg = arg.reg;
        MemAddress regBits = regStringToAddress(reg);
        if (!regBits)
        {
            stringstream msg;
            msg << "Invalid register given to `" << reverseOpcodeTable.at(instr.opcode)
                << "`:  " << reg;
            throw runtime_error(msg.str());
        }

        return regBits;
    };

    switch (instr.opcode)
    {
    case DB:
    case DW:
    case DD:
    {
        DataArgument* data = dynamic_cast<DataArgument*>( instr.args );
        assert(data);
        generated.insert(generated.end(), data->data.begin(), data->data.end());
        break;
    }

    case HALT:
    case IDLE:
    case CLI:
    case STI:
        generated.push_back(instr.opcode);
        break;

    case RSTR:
    {
        RegisterArgument* destReg = getFirstReg();
        MemAddress regBits = argToRegBits(*destReg);
        generated.push_back(instr.opcode | regBits);
        break;
    }

    case CMP:
    {
        RegisterArgument* destReg = getFirstReg();
        MemAddress regBits = argToRegBits(*destReg);
        validateNumArguments(2);
        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr.args->next ))
        {
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(instr.opcode | regBits | REGISTER | srcRegArg);
        }
        else
        {
            generated.push_back(instr.opcode | regBits | IMMEDIATE);
            generated.push_back(program.solveArgumentAddress(instr.args->next));
        }
        break;
    }

    case TST:
    {
        RegisterArgument* destReg = getFirstReg();
        MemAddress regBits = argToRegBits(*destReg);
        generated.push_back(instr.opcode | regBits);
        break;
    }

    case JMP:
    case JE:
    case JNE:
    case JGE:
    case JG:
    case JLE:
    case JL:
    case CALL:
    {
        validateNumArguments(1);

        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr.args ))
        {
            // NOTE: TODO:
            // Indirect mode is only implemented for CALL in the emulator

            MemAddress regBits = argToRegBits(*arg_reg);
            generated.push_back(instr.opcode | INDIRECT | regBits);
        }
        else
        {
            MemAddress destination = program.solveArgumentAddress(instr.args);
            MemAddress diff = destination - instr.address;
            if (diff > 0xFFFF || diff < -0xFFFF)
                throw runtime_error("jmp out of range");
            generated.push_back(instr.opcode | RELATIVE | static_cast<uint16_t>( diff ));
        }
        break;
    }

    case RET:
    case RTI:
        generated.push_back(instr.opcode);
        break;

    case MOV:
    {
        RegisterArgument* destReg = getFirstReg();
        MemAddress regBits = argToRegBits(*destReg);
        validateNumArguments(2);
        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr.args->next ))
        {
            // The value returned by regStringToAddress is the register value
            // if the register were a destination.  This register, however, is
            // the source register, which is placed at bit 0.  So, we shift the result back
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(instr.opcode | regBits | REGISTER | srcRegArg);
        }
        else
        {
            generated.push_back(instr.opcode | regBits | IMMEDIATE);
            generated.push_back(program.solveArgumentAddress(instr.args->next));
        }
        break;
    }

    case LOAD:
    case LOADW:
    case LOADB:
    {
        RegisterArgument* destReg = getFirstReg();
        MemAddress regBits = argToRegBits(*destReg);
        validateNumArguments(2);
        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr.args->next ))
        {
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(instr.opcode | regBits | INDIRECT | srcRegArg);
        }
        else
        {
            generated.push_back(instr.opcode | regBits | ABSOLUTE);
            generated.push_back(program.solveArgumentAddress(instr.args->next));
        }
        break;
    }

    case STR:
    case STRW:
    case STRB:
    {
        RegisterArgument* destReg = getFirstReg();
        MemAddress regBits = argToRegBits(*destReg);
        validateNumArguments(2);

        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr.args->next ))
        {
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(instr.opcode | regBits | REGISTER | srcRegArg);
        }
        else
        {
            if (instr.opcode == STR)
            {
                generated.push_back(instr.opcode | regBits | IMMEDIATE);
                generated.push_back(program.solveArgumentAddress(instr.args->next));
            }
            else
                generated.push_back(instr.opcode | regBits | IMMEDIATE | program.solveArgumentAddress(instr.args->next));
        }
        break;
    }

    case PUSH:
    case PUSHW:
    case PUSHB:
    {
        validateNumArguments(1);

        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr.args ))
        {
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(instr.opcode | REGISTER | srcRegArg);
        }
        else
        {
            if (instr.opcode == PUSH)
            {
                generated.push_back(instr.opcode | IMMEDIATE);
                generated.push_back(program.solveArgumentAddress(instr.args));
            }
            else
            {
                uint16_t operand = program.solveArgumentAddress(instr.args) & 0xFFFF;
                generated.push_back(instr.opcode | IMMEDIATE | operand);
            }
        }

        break;
    }

    case POP:
    case POPW:
    case POPB:
    {
        RegisterArgument* destReg = getFirstReg();
        MemAddress regBits = argToRegBits(*destReg);
        generated.push_back(instr.opcode | regBits);
        break;
    }

    case MEMCPY:
    case MEMSET:
    {
        validateNumArguments(3);
        RegisterArgument* destReg = dynamic_cast<RegisterArgument*>( instr.args );
        RegisterArgument* srcReg = dynamic_cast<RegisterArgument*>( instr.args->next );
        RegisterArgument* lenReg = dynamic_cast<RegisterArgument*>( instr.args->next->next );
        if (!destReg || !srcReg || !lenReg)
            throw runtime_error("Invalid argument given to memcpy");

        MemAddress destBits = regStringToAddress(destReg->reg);
        MemAddress srcBits = regStringToAddress(srcReg->reg) >> (INS_REG - 8);
        MemAddress lenBits = regStringToAddress(lenReg->reg) >> INS_REG;

        generated.push_back(instr.opcode | REGISTER | destBits | srcBits | lenBits);
        break;
    }

    case CLRSET:
    case CLRSETV:
    case DRWSQ:
    {
        validateNumArguments(1);
        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr.args ))
        {
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(instr.opcode | REGISTER | srcRegArg);
        }
        else if (IntegerArgument* reg_int = dynamic_cast<IntegerArgument*>( instr.args ))
        {
            generated.push_back(instr.opcode | IMMEDIATE);
            generated.push_back(reg_int->data);
        }
        else
        {
            stringstream msg;
            msg << "Invalid argument to `" << reverseOpcodeTable.at(instr.opcode) << "` instruction";
            throw runtime_error(msg.str());
        }

        break;
    }

    case READ:
    {
        validateNumArguments(2);
        RegisterArgument* destReg = getFirstReg();
        MemAddress regBits = argToRegBits(*destReg);
        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr.args->next ))
        {
            throw runtime_error("Second argument cannot be register [not implemented]");
        }
        else if (SymbolArgument* arg_sym = dynamic_cast<SymbolArgument*>( instr.args->next ))
        {
            const ImmediateValue& imm = program.getSymbol(arg_sym->symbolName);
            if (imm.instruction)
                throw runtime_error("First argument of `read` cannot be a label");

            MemAddress port = imm.value;
            if (port > 0xFFFF)
                throw runtime_error("The maximum port number given to `read` is 0xFFFF");

            generated.push_back(instr.opcode | regBits | IMMEDIATE | port);
        }
        else
            throw runtime_error("Invalid operand to read instruction");

        break;
    }

    case WRITE:
    {
        SymbolArgument* portArg = dynamic_cast<SymbolArgument*>( instr.args );
        if (!portArg)
            throw runtime_error("First argument of write must be a symbol");

        const ImmediateValue& imm = program.getSymbol(portArg->symbolName);
        if (imm.instruction)
            throw runtime_error("First argument of `write` cannot be a label");

        MemAddress port = imm.value;
        if (port > 0xFFFF)
            throw runtime_error("The maximum port number given to `write` is 0xFFFF");

        generated.push_back(instr.opcode | port);
        generated.push_back(program.solveArgumentAddress(portArg->next));
        break;
    }

    case ADD:
    case SUB:
    case MUL:
    case AND:
    case OR:
    case NOT:
    {
        RegisterArgument* destReg = getFirstReg();
        MemAddress regBits = argToRegBits(*destReg);
        validateNumArguments(2);

        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr.args->next ))
        {
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(instr.opcode | regBits | REGISTER | srcRegArg);
        }
        else
        {
            generated.push_back(instr.opcode | regBits | IMMEDIATE);
            generated.push_back(program.solveArgumentAddress(instr.args->next));
        }
        break;
    }

    case INC:
    case DEC:
    {
        RegisterArgument* destReg = getFirstReg();
        MemAddress regBits = argToRegBits(*destReg);
        generated.push_back(instr.opcode | regBits);
        break;
    }

    case SHR:
    case SHL:
    {
        RegisterArgument* destReg = getFirstReg();
        MemAddress regBits = argToRegBits(*destReg);
        validateNumArguments(2);
        if (RegisterArgument* arg_reg = dynamic_cast<RegisterArgument*>( instr.args->next ))
        {
            MemAddress srcRegArg = regStringToAddress(arg_reg->reg) >> INS_REG;
            generated.push_back(instr.opcode | regBits | REGISTER | srcRegArg);
        }
        else
        {
            MemAddress value = program.solveArgumentAddress(instr.args->next);
            if (value > 32)
                throw runtime_error("shr/shl operand cannot be greater than 32");
            generated.push_back(instr.opcode | regBits | IMMEDIATE | value);
        }
        break;
    }

    default:
        stringstream msg;
        msg << "Missing code generation for `" << reverseOpcodeTable.at(instr.opcode) << "`";
        throw runtime_error(msg.str());
    }

    return generated;
}

void Isa::loadOpcodeTable()
{
    string opname;

    #define MAP_OPCODE(OPCODE_CAPS) \
    opname = #OPCODE_CAPS; \
    transform(opname.begin(), opname.end(), opname.begin(), ::tolower); \
    this->opcodeTable[opname] = OPCODE_CAPS; \
    this->reverseOpcodeTable[OPCODE_CAPS] = opname;

    /* Map pseudo-opcodes (assembler directives) */
    MAP_OPCODE(DB);
    MAP_OPCODE(DD);

    /* Map real opcodes */
    // cpu modes
    MAP_OPCODE(HALT);
    MAP_OPCODE(IDLE);
    MAP_OPCODE(CLI);
    MAP_OPCODE(STI);
    MAP_OPCODE(RSTR);

    // control flow
    MAP_OPCODE(CMP);
    MAP_OPCODE(TST);
    MAP_OPCODE(JMP);
    MAP_OPCODE(LNGJMP);
    MAP_OPCODE(JE);
    MAP_OPCODE(JNE);
    MAP_OPCODE(JGE);
    MAP_OPCODE(JG);
    MAP_OPCODE(JLE);
    MAP_OPCODE(JL);
    MAP_OPCODE(CALL);
    MAP_OPCODE(LNGCALL);
    MAP_OPCODE(RET);
    MAP_OPCODE(RTI);

    // register movement
    MAP_OPCODE(MOV);

    // load & store
    MAP_OPCODE(LOAD);
    MAP_OPCODE(LOADW);
    MAP_OPCODE(LOADB);
    MAP_OPCODE(STR);
    MAP_OPCODE(STRW);
    MAP_OPCODE(STRB);
    MAP_OPCODE(PUSH);
    MAP_OPCODE(PUSHW);
    MAP_OPCODE(PUSHB);
    MAP_OPCODE(POP);
    MAP_OPCODE(POPW);
    MAP_OPCODE(POPB);
    MAP_OPCODE(MEMCPY);
    MAP_OPCODE(MEMSET);
    MAP_OPCODE(CLRSET);
    MAP_OPCODE(CLRSETV);
    MAP_OPCODE(DRWSQ);

    // I/O
    MAP_OPCODE(READ);
    MAP_OPCODE(WRITE);

    // Math
    MAP_OPCODE(ADD);
    MAP_OPCODE(INC);
    MAP_OPCODE(SUB);
    MAP_OPCODE(DEC);
    MAP_OPCODE(MUL);
    MAP_OPCODE(MULW);
    MAP_OPCODE(MULB);
    MAP_OPCODE(AND);
    MAP_OPCODE(OR);
    MAP_OPCODE(NOT);
    MAP_OPCODE(SHR);
    MAP_OPCODE(SHL);
}

void Isa::loadInstructionSizeTable()
{
    // A value of 0 means that the instruction size needs to be determined
    // dynamically from the type of its arguments.
    // A value > 0 means that the size of the instruction is constant at that
    // value, regardless of argument types.

    // CPU modes / special
    this->instructionSizeTable[HALT] = 4;
    this->instructionSizeTable[IDLE] = 4;
    this->instructionSizeTable[CLI]  = 4;
    this->instructionSizeTable[STI]  = 4;
    this->instructionSizeTable[RSTR] = 4;

    // Pseudo-opcodes
    this->instructionSizeTable[DB] = 0;
    this->instructionSizeTable[DW] = 0;
    this->instructionSizeTable[DD] = 0;

    // Control flow
    this->instructionSizeTable[CMP]  = 0;
    this->instructionSizeTable[TST]  = 4;
    this->instructionSizeTable[JMP]  = 4;
    this->instructionSizeTable[JE]   = 4;
    this->instructionSizeTable[JNE]  = 4;
    this->instructionSizeTable[JGE]  = 4;
    this->instructionSizeTable[JG]   = 4;
    this->instructionSizeTable[JLE]  = 4;
    this->instructionSizeTable[JL]   = 4;
    this->instructionSizeTable[CALL] = 4;
    this->instructionSizeTable[RET]  = 4;
    this->instructionSizeTable[RTI]  = 4;

    // Register movement
    this->instructionSizeTable[MOV] = 0;

    // Load & store
    this->instructionSizeTable[LOAD]    = 0;
    this->instructionSizeTable[LOADW]   = 0;
    this->instructionSizeTable[LOADB]   = 0;
    this->instructionSizeTable[STR]     = 0;
    this->instructionSizeTable[STRW]    = 4;
    this->instructionSizeTable[STRB]    = 4;
    this->instructionSizeTable[PUSH]    = 0;
    this->instructionSizeTable[PUSHW]   = 4;
    this->instructionSizeTable[PUSHB]   = 4;
    this->instructionSizeTable[POP]     = 4;
    this->instructionSizeTable[POPW]    = 4;
    this->instructionSizeTable[POPB]    = 4;
    this->instructionSizeTable[MEMCPY]  = 4;
    this->instructionSizeTable[MEMSET]  = 4;
    this->instructionSizeTable[CLRSET]  = 0;
    this->instructionSizeTable[CLRSETV] = 0;
    this->instructionSizeTable[DRWSQ]   = 0;

    // I/O
    this->instructionSizeTable[READ]  = 4;
    this->instructionSizeTable[WRITE] = 8;

    // Math
    this->instructionSizeTable[ADD] = 0;
    this->instructionSizeTable[INC] = 4;
    this->instructionSizeTable[SUB] = 0;
    this->instructionSizeTable[DEC] = 4;
    this->instructionSizeTable[MUL] = 0;
    this->instructionSizeTable[AND] = 0;
    this->instructionSizeTable[OR]  = 0;
    this->instructionSizeTable[NOT] = 0;
    this->instructionSizeTable[SHR] = 4;
    this->instructionSizeTable[SHL] = 4;
}
