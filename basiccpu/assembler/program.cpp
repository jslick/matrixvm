#include "program.h"

#include <sstream>
#include <stdexcept>

using namespace std;

Program::Program(const Isa& isa, int offset /* = 0 */)
: isa(isa), offset(offset)
{ }

Program::~Program()
{
    for (unsigned int i = 0; i < this->instructions.size(); i++)
        delete this->instructions[i];
}

Instruction* Program::createInstruction(
    const string&           instruction,
    Instruction::AddrMode   addrMode /* = NumAddrModes */
    )
{
    Instruction* instr = new Instruction(instruction, addrMode);
    this->instructions.push_back(instr);
    return instr;
}

void Program::setSymbol(const string& symbolName, Instruction* instr)
{
    this->symbols[symbolName] = instr;
}

const Instruction* Program::getInstructionAtLabel(const string& symbolName) const
{
    try {
        return this->symbols.at(symbolName);
    }
    catch (out_of_range& e) {
        stringstream msg;
        msg << "Label not found:  " << symbolName;
        throw runtime_error(msg.str());
    }
}

void Program::assemble(FILE* stream)
{
    this->calcAddresses();  // first pass

    for (unsigned int i = 0; i < this->instructions.size(); i++)
    {
        Instruction* instr = this->instructions[i];
        #if DEBUG
        fprintf(stream, "%08x:  ", instr->address);
        #endif

        vector<MemAddress> generated = this->isa.generateInstructions(*this, instr);
        for (unsigned int j = 0; j < generated.size(); j++)
        {
            MemAddress ins = generated[j];
            #if DEBUG
            if (j > 0)
                fputc(' ', stream);
            fprintf(stream, "0x%08x", ins);
            #else
            // Writes in big-endian
            fputc((ins & 0xFF000000) >> 24, stream);
            fputc((ins & 0x00FF0000) >> 16, stream);
            fputc((ins & 0x0000FF00) >>  8, stream);
            fputc((ins & 0x000000FF) >>  0, stream);
            #endif
        }
        #if DEBUG
        fputc('\n', stream);
        #endif
    }
}

void Program::calcAddresses()
{
    MemAddress ip = this->offset;
    for (unsigned int i = 0; i < this->instructions.size(); i++)
    {
        Instruction* instr = this->instructions[i];
        instr->address = ip;
        ip += this->isa.calcInstructionSize(instr);
        ip += ip % 4;   // align ip
    }
}
