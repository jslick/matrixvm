#include "program.h"
#include "isa.h"

#include <string>

using namespace std;

vector<uint8_t> stringToVector(const char* str, bool ensureNull)
{
    vector<uint8_t> rv;
    for (const char* ch = str; *ch; ch++)
        rv.push_back(*ch);
    if (ensureNull)
        rv.push_back(0);
    int mod = rv.size() % 4;
    for (int i = 0; i < 4 - mod; i++)
        rv.push_back(0);

    return rv;
}

Argument* addArg(vector<Argument*>& args, Argument* arg)
{
    args.push_back(arg);
    return arg;
}

int main()
{
    Isa isa;
    Program program(isa, 1000 /* offset */);

    vector<Argument*> args; // hold arguments for cleanup
    #define NEWARG(arg) ( addArg(args, arg) )

    // jmp main
    Instruction* jmp_main = program.createInstruction("jmp");
    jmp_main->args.push_back(NEWARG(new SymbolArgument("main")));
    program.setSymbol("init", jmp_main);

    // db "Hello World!" 0x0a 0x00
    Instruction* db_hello = program.createInstruction("db");
    // \1 is the 'flags' byte to CharOutputDevice
    db_hello->args.push_back(NEWARG(new DataArgument(stringToVector("Hello World!\n", true))));
    program.setSymbol("S1", db_hello);

    // mov r1, #S1
    Instruction* mov_r1_s1 = program.createInstruction("mov", Instruction::AddrMode::Immediate);
    mov_r1_s1->args.push_back(NEWARG(new RegisterArgument("r1")));
    mov_r1_s1->args.push_back(NEWARG(new SymbolArgument("S1")));
    program.setSymbol("S1_LENGTH",  mov_r1_s1);
    program.setSymbol("main",       mov_r1_s1);

    // mov r1, #S1_LENGTH-#S1
    Instruction* mov_r2_s1 = program.createInstruction("mov", Instruction::AddrMode::Immediate);
    mov_r2_s1->args.push_back(NEWARG(new RegisterArgument("r2")));
    mov_r2_s1->args.push_back(NEWARG(new DifferenceArgument("S1_LENGTH", "S1")));

    // memcpy #OUTPUT_DMA
    const static uint8_t DMA_BUFFER = 2 /* CharOutputDevice uses memory @ 0x02 */;
    const static uint8_t OUTPUT_DMA[] = { 0, 0, 0, DMA_BUFFER }; // 32-bit argument
    Instruction* memcpy_dma = program.createInstruction("memcpy", Instruction::AddrMode::Immediate);
    memcpy_dma->args.push_back(NEWARG(new DataArgument(OUTPUT_DMA, 4)));

    // write #OUTPORT, 1
    const static uint8_t OUTPORT[] = { 0, 1 }; // 16-bit argument
    const static uint8_t WRITE_WHAT[] = { 0, 0, 0, 1 }; // 32-bit argument
    Instruction* write_outport = program.createInstruction("write", Instruction::AddrMode::Immediate);
    write_outport->args.push_back(NEWARG(new DataArgument(OUTPORT, 2)));
    write_outport->args.push_back(NEWARG(new DataArgument(WRITE_WHAT, 4)));

    // halt
    program.createInstruction("halt");

    program.assemble(stdout);

    // Cleanup
    for (unsigned int i = 0; i < args.size(); i++)
        delete args[i];

    return 0;
}
