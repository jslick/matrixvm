#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include "isa.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <stdexcept>

struct Argument;

struct ImmediateValue
{
    // only one of these two values shall be used
    Instruction*    instruction;
    MemAddress      value;

    ImmediateValue()
    : instruction(0)
    {}

    MemAddress getAddress() const;
};

/**
 * Instruction Argument
 */
struct Argument
{
    Argument* next;

    /* all subclasses must call this constructor or otherwise initialize `next` */
    Argument() : next(0) { }

    virtual void _dummy_() { /* me is polymorphic */ }
};

struct SymbolArgument : Argument
{
    std::string symbolName;

    SymbolArgument(const std::string& symbolName)
    : Argument(), symbolName(symbolName)
    {}
};

/**
 * Holds assembler directive arguments
 */
struct DataArgument : Argument
{
    std::vector<uint8_t> data;

    DataArgument(const std::vector<uint8_t>& data)
    : Argument(), data(data)
    {}

    DataArgument(const uint8_t* data, int length)
    : Argument(), data(data, data+length)
    {}
};

struct RegisterArgument : Argument
{
    std::string reg;

    RegisterArgument(const std::string& reg)
    : Argument(), reg(reg)
    {}
};

/**
 * Holds two arguments, and an operation to operate on the two elements
 */
struct BinaryArgument : Argument
{
    std::string op;
    Argument* arg1;
    Argument* arg2;

    BinaryArgument(const std::string& op, Argument* arg1, Argument* arg2)
    : Argument(), op(op), arg1(arg1), arg2(arg2)
    {}
};

struct IntegerArgument : Argument
{
    MemAddress data;

    IntegerArgument(MemAddress data)
    : Argument(), data(data)
    {}
};

struct Instruction
{
    friend class Program;

    enum AddrMode {
        Immediate, Absolute, Relative, Indirect,
        NumAddrModes
    };

    std::string instruction;    //!< Name of instruction
    int         address;        //!< To be calculated at assemble time
    Argument*   args;
    AddrMode    addrMode;

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
     * Designate a symbol that holds a particular value
     * @param[in]   symbolName
     * @param[in]   value
     */
    void setSymbol(const std::string& symbolName, MemAddress value);

    /**
     * Get a symbol or immediate value that has the given symbol name
     * @param[in]   symbolName
     * @return  A structure holding either the Instruction pointer, or
     *          immediate value for the symbol name
     */
    const ImmediateValue& getSymbol(const std::string& symbolName) const;

    /**
     * Calculate the address of the argument
     * @param[in]   arg     Can be symbol argument or binary (assembly-time
     *                      math) argument
     * @return  Memory address
     */
    MemAddress solveArgumentAddress(Argument* arg) const;

    /**
     * Assemble the program, writing binary data to `stream`
     * @param[out]  stream  File to write the code to
     * @param[in]   debug   Whether or not to print debug information about the
     *                      generated instructions.
     */
    void assemble(FILE* stream, bool debug = false);

    /**
     * Calculate the memory address of a binary operation
     * @param[in]   operation   Operation to perform on the two arguments
     *                          (e.g. subtraction "-" operator)
     * @param[in]   addr1       LHS address
     * @param[in]   addr2       RHS address
     * @return The address as a result of the binary operation
     */
    static MemAddress binaryOperation(
        const std::string&  operation,
        MemAddress          addr1,
        MemAddress          addr2
        )
    {
        if (operation == "+")
            return addr1 + addr2;
        else if (operation == "-")
            return addr1 - addr2;
        else if (operation == "*")
            return addr1 * addr2;
        else
        {
            std::stringstream msg;
            msg << "Unrecognized binary operator " << operation;
            throw std::runtime_error(msg.str());
        }
    }

protected:

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
    std::unordered_map<std::string,ImmediateValue> symbols;   //<! Map of symbols -> instruction/value
};

#endif // PROGRAM_HPP
