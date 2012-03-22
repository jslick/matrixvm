#ifndef ISA_HPP
#define ISA_HPP

#include <common.h>

#include <string>
#include <vector>
#include <unordered_map>

class Program;
class Instruction;

class Isa
{
public:

    Isa();

    /**
     * Get the binary opcode corresponding to the string representation
     * e.g. "jmp" -> JMP
     * @param[in]   instruction
     * @return  opcode
     */
    MemAddress getOpcode(const std::string& instruction);

    /**
     * Calculate size of the given instruction
     * @param[in]   instr
     * @return Size of the instruction
     */
    int calcInstructionSize(Instruction& instr);

    /**
     * Generate code from an instruction
     * @param[in]   program     Program that the instruction belongs to; used
     *                          to get addresses of referenced instructions
     * @param[in]   instr       Instruction to generate code from
     */
    std::vector<MemAddress> generateInstructions(const Program& program, Instruction& instr);

private:

    Isa(const Isa& isa) { } // copy not permitted

    void loadOpcodeTable();

    void loadInstructionSizeTable();

    //<! opcode string to binary mappings
    std::unordered_map<std::string,MemAddress> opcodeTable;

    //<! reverse of opcodeTable for use in error messages
    std::unordered_map<MemAddress,std::string> reverseOpcodeTable;

    bool opcodeTableLoaded; //!< is the opcodeTable initialized yet?

    //<! opcode to instruction size mappings
    std::unordered_map<MemAddress,int> instructionSizeTable;

    bool instructionSizeTableLoaded;    //!< is the instructionSizeTabl initialized yet?

};

#endif // ISA_HPP
