#ifndef PROGRAM_H
#define PROGRAM_H

#include "isa.h"

#include <string>
#include <vector>
#include <unordered_map>

/**
 * Instruction Argument
 */
struct Argument
{
    enum Type { Immediate, String, Register, Symbol };

    Type type;
    std::vector<uint8_t> value; //<! Data of varying relevance

    Argument(Type type, const std::vector<uint8_t>& value)
    : type(type), value(value)
    { }

    /**
     * @param[in]   type
     * @param[in]   value   Will be converted to vector, will null-terminator
     */
    Argument(Type type, const std::string& value)
    : type(type)
    {
        int length = value.length() + 1 /* null char */;
        int mod = length % 4;
        if (mod)
            length += (4 - mod);
        this->value.reserve(length);

        for (unsigned int i = 0; i < value.length() /* null char */; i++)
            this->value.push_back(value[i]);
        for (unsigned int i = value.length(); i < static_cast<unsigned int>( length ); i++)
            this->value.push_back(0);
    }

    /**
     * @param[in]   type
     * @param[in]   value
     * @param[in]   valueSize   Size of param value
     */
    Argument(Type type, const uint8_t* value, int valueSize)
    : type(type)
    {
        this->value.insert(this->value.end(), value, value + valueSize);
    }
};

struct Instruction
{
    friend class Program;

    std::string             instruction;    //!< Name of instruction
    std::vector<Argument>   args;
    int                     address;        //!< To be calculated at assemble time

private:

    /**
     * @param[in]   instruction     Name of instruction
     */
    Instruction(const std::string& instruction)
    : instruction(instruction), address(-1)
    { }
};

class Isa;

class Program
{
public:

    /**
     * @param[in]   isa     Instruction Set used to generate code
     * @param[in]   offset  Address offset to generate instruction addresses from
     */
    Program(const Isa& isa, int offset = 0);

    /**
     * Cleans up generated instructions
     */
    ~Program();

    /**
     * Create an instruction allocated on the heap
     * @param[in]   instruction     Name of instruction
     */
    Instruction* createInstruction(const std::string& instruction);

    /**
     * Designate a symbol that starts at a particular instruction
     * @param[in]   symbolName
     * @param[in]   instr       Instruction that the symbol points to / starts at
     */
    void setSymbol(const std::string& symbolName, Instruction* instr);

    /**
     * @param[in]   symbolName
     */
    const Instruction* getInstructionAtLabel(const std::string& symbolName) const;

    /**
     * Assemble the program, writing binary data to `stream`
     * @param[out]  stream  File to write the code to
     */
    void assemble(FILE* stream);

protected:

    // assembler first pass
    /**
     * Calculate addresses of each instruction
     *
     * This is the first pass of the assembler.
     */
    void calcAddresses();

private:

    // Copy not permitted
    Program(const Program& program) { }

    Isa isa;        //<! Instruction set of program to generate
    int offset;     //<! Offset of instructions to generate
    std::vector<Instruction*> instructions; //<! All instructions in the program, in sequential order
    std::unordered_map<std::string,Instruction*> symbols;   //<! Map of symbols -> instruction
};

#endif // PROGRAM_H
