#ifndef ASSEMBLER_UTIL_HPP
#define ASSEMBLER_UTIL_HPP

#include "program.hpp"

#include <vector>

/**
 * Create instruction
 * @param[in]   opcode
 * @param[in]   args    Linked list of Arguments included in the instruction
 * @return Instruction tied to the Program singleton
 */
Instruction* addInstruction(const char* opcode, Argument* args = 0);

/**
 * Create Instruction for assembler directive
 * @param[in]   directive   Name of assembler directive (e.g. `db`)
 * @param[in]   dataArgs    Linked list of arguments for assembler directive
 * @return Instruction representing the assembler directive
 * @note It's not _really_ an instruction; it's just data.  But, the program
 *       assembles the data using this structure.
 */
Instruction* addDataInstruction(const char* directive, Argument* dataArgs);

/**
 * Add label to the next incoming instruction
 * When the next instruction is encountered, it will assume all of the current
 * labels, and the current label list will be cleared.
 * @param[in]   labelName
 */
void addCurrentLabel(const std::string& labelName);

/**
 * Append arg to list
 * @param[in,out]   list    Linked list of Arguments
 * @param[in]       arg     Argument to append to list
 * @return The list
 */
Argument* appendArgument(Argument* list, Argument* arg);

/**
 * Collapses the list of DataArguments into a single DataArgument
 * @param[in,out]   list                List of DataArguments to collapse
 * @param[in]       ensureAlignment     If true, the data vector of the
 *                                      resulting argument will be padded so
 *                                      that it consumes a multiple of 4 bytes
 * @return  The first Argument in the list, but with the rest of the list
 *          merged into the first element
 */
DataArgument* collapseDataArguments(DataArgument* list, bool ensureAlignment);

/**
 * Convert string to a vector of bytes
 * @param[in]   str                 String to convert to bytes
 * @param[in]   ensureNull          If true, the vector will have at least one
 *                                  null character at the tail
 * @param[in]   ensureAlignment     If true, the vector will be tail-padded to
 *                                  align to 4 bytes
 * @return  The resulting vector
 */
std::vector<uint8_t> stringToVector(const char* str, bool ensureNull, bool ensureAlignment);

/**
 * Convert 32-bit integer to a vector of bytes, big-endian
 * @param[in]   val     32-bit integer
 * @return  4-byte vector, consisting of big-endian val
 */
std::vector<uint8_t> int32ToVector(uint32_t val);

void cleanupArgs(const Program& program);

#endif // ASSEMBLER_UTIL_HPP
