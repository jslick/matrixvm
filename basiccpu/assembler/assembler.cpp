#include "program.h"
#include "isa.h"

#include <string>

using namespace std;

int main()
{
    Isa isa;
    Program program(isa, 1000 /* offset */);

    // jmp main
    Instruction* jmp_main = program.createInstruction("jmp");
    jmp_main->args.push_back(Argument(Argument::Type::Symbol, "main"));
    program.setSymbol("init", jmp_main);

    // db "Hello World!" 0x0a 0x00
    Instruction* db_hello = program.createInstruction("db");
    // \1 is the 'flags' byte to CharOutputDevice
    db_hello->args.push_back(Argument(Argument::Type::String, "\1Hello World!\n"));
    program.setSymbol("S1", db_hello);

    // load r1, S1
    Instruction* load_r1_s1 = program.createInstruction("load");
    load_r1_s1->args.push_back(Argument(Argument::Type::Register, "r1"));
    load_r1_s1->args.push_back(Argument(Argument::Type::Symbol, "S1"));
    program.setSymbol("S1_LENGTH",  load_r1_s1);
    program.setSymbol("main",       load_r1_s1);

    // load r1, S1_LENGTH-S1
    // cheating:  using immediate instead of difference of symbols
    const static uint8_t S1_LENGTH[] = { 0, 0, 0, 32 };
    Instruction* load_r2_s1 = program.createInstruction("load");
    load_r2_s1->args.push_back(Argument(Argument::Type::Register, "r2"));
    load_r2_s1->args.push_back(Argument(Argument::Type::Immediate, S1_LENGTH, 4));
    // TODO:  Change to LOADB and implement that opcode

    // memcpy OUTPUT_DMA
    const static uint8_t DMA_BUFFER = 1 /* CharOutputDevice uses memory @ 0x01 */;
    const static uint8_t OUTPUT_DMA[] = { 0, 0, 0, DMA_BUFFER }; // 32-bit argument
    Instruction* memcpy_dma = program.createInstruction("memcpy");
    memcpy_dma->args.push_back(Argument(Argument::Type::Immediate, OUTPUT_DMA, 4));

    // write OUTPORT, 1
    const static uint8_t OUTPORT[] = { 0, 1 }; // 16-bit argument
    const static uint8_t WRITE_WHAT[] = { 0, 0, 0, 1 }; // 32-bit argument
    Instruction* write_outport = program.createInstruction("write");
    write_outport->args.push_back(Argument(Argument::Type::Immediate, OUTPORT, 2));
    write_outport->args.push_back(Argument(Argument::Type::Immediate, WRITE_WHAT, 4));

    // halt
    program.createInstruction("halt");

    program.assemble(stdout);

    return 0;
}
