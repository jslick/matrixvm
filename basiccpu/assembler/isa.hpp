#ifndef ISA_HPP
#define ISA_HPP

#include <common.h>

#include <vector>

class Program;
class Instruction;

class Isa
{
public:

    /**
     * Calculate size of the given instruction
     * @param[in]   instr
     * @return Size of the instruction
     */
    int calcInstructionSize(Instruction* instr);

    /**
     * Generate code from an instruction
     * @param[in]   program     Program that the instruction belongs to; used
     *                          to get addresses of referenced instructions
     * @param[in]   instr       Instruction to generate code from
     */
    std::vector<MemAddress> generateInstructions(const Program& program, Instruction* instr);

};

#endif // ISA_HPP
