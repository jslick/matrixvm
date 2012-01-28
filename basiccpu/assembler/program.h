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
    virtual void _dummy_() { /* me is polymorphic */ }
};

struct SymbolArgument : Argument
{
    std::string symbolName;

    SymbolArgument(const std::string& symbolName)
    : symbolName(symbolName)
    {}
};

struct DataArgument : Argument
{
    std::vector<uint8_t> data;

    DataArgument(const std::vector<uint8_t>& data)
    : data(data)
    {}

    DataArgument(const uint8_t* data, int length)
    : data(data, data+length)
    {}
};

struct RegisterArgument : Argument
{
    std::string reg;

    RegisterArgument(const std::string& reg)
    : reg(reg)
    {}
};

struct MathArgument : Argument
{
};

struct DifferenceArgument : MathArgument
{
    std::string symbols[2];

    DifferenceArgument(const std::string& symbol1, const std::string& symbol2)
    {
        symbols[0] = symbol1;
        symbols[1] = symbol2;
    }
};


struct Instruction
{
    friend class Program;

    enum AddrMode {
        Immediate, Absolute, Relative, Indirect,
        NumAddrModes
    };

    std::string             instruction;    //!< Name of instruction
    int                     address;        //!< To be calculated at assemble time
    std::vector<Argument*>  args;
    AddrMode                addrMode;

private:

    /**
     * @param[in]   instruction     Name of instruction
     */
    Instruction(const std::string& instruction, AddrMode addrMode = NumAddrModes)
    : instruction(instruction), address(-1), addrMode(addrMode)
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
    Instruction* createInstruction(
        const std::string&      instruction,
        Instruction::AddrMode   addrMode = Instruction::AddrMode::NumAddrModes
        );

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
