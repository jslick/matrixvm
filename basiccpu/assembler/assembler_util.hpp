#ifndef ASSEMBLER_UTIL_HPP
#define ASSEMBLER_UTIL_HPP

#include "program.hpp"

#include <vector>
#include <queue>

#define DEFAULT_OUTPUT_FILE "bcpu.bin"

struct Options
{
    const char* outputFilename;
    FILE*       outputFile;
    int         debugFlag;

    Options()
    : outputFilename(DEFAULT_OUTPUT_FILE), outputFile(0), debugFlag(0)
    { }
};

/**
 * Set symbol to a value in the program
 * @param   symbolName
 * @param   value       This is evaluated, then stored as a MemAddress
 */
void setSymbol(const char* symbolName, Argument* value);

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
 * Create a DataArgument, with many 0's
 * @param[in]   symbolArg   Resolved to a value; the number of 0's generated is
 *                          this value
 * @return  Return DataArgument full of 0's
 */
Argument* createBytes(Argument* symbolArg);

/**
 * Add label to the next incoming instruction
 * When the next instruction is encountered, it will assume all of the current
 * labels, and the current label list will be cleared.
 * @param[in]   labelName
 */
void addCurrentLabel(const std::string& labelName);

/**
 * Inventory a new heap-allocate Argument to delete later
 * @param[in]   newArg  Newly-created, heap-allocated Argument
 * @return  Param goes in, param goes out
 * @note  The param Argument must only be freed in cleanupArgs(); no other
 *        function should delete the Argument.
 */
Argument* inventoryArgument(Argument* newArg);

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
 * @return  The first Argument in the list, but with the rest of the list
 *          merged into the first element
 */
DataArgument* collapseDataArguments(DataArgument* list);

/**
 * Change a DataArgument full of 8-bit values to 32-bit values, each having 4
 * 8-bit values
 * @param[in]   arg     DataArgument full of 8-bit values
 * @return  The 8-bit values compressed big-endian into 32-bit values
 */
void compress8DataArguments(DataArgument& arg);

/**
 * Convert string to a vector of bytes
 * @param[in]   str                 String to convert to bytes
 * @param[in]   ensureNull          If true, the vector will have at least one
 *                                  null character at the tail
 * @return  The resulting vector
 */
std::vector<uint32_t> stringToVector(const char* str, bool ensureNull);

/**
 * Inventory a new heap-allocated string
 * @param[in]   heapString  Newly-created, heap-allocated buffer
 * @return  Param goes in, param goes out
 * @note  The param string must only be freed in cleanupStrings(); no other
 *        function should free the buffer.
 * @note  The string does not have to be a c-string
 */
char* inventoryString(char* heapString);

/**
 * Free all inventoried heap-allocated strings
 */
void cleanupStrings();

/**
 * Delete all inventoried heap-allocated Arguments
 */
void cleanupArgs();

#endif // ASSEMBLER_UTIL_HPP
