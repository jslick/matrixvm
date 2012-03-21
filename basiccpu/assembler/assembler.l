%{

#include "program.hpp"
#include "assembler.yacc.hpp"
#include "assembler_util.hpp"

#ifdef DEBUG_LEX
#    define DEBUGF(...) printf(__VA_ARGS__)
#else
#    define DEBUGF(...)
#endif

#include <stdio.h>

%}

whitesp     [ \t\n\r]
intrawhtsp  [ \t]
newline     [\n\r]+
ident       [a-zA-Z][a-zA-Z0-9_]*
digit       [0-9]
hexdigit    [0-9a-fA-F]
octdigit    [0-7]
bindigit    [0-1]
string      \"[^"]*\"
char        \'\\?[^']\'

%option noyywrap nounput yylineno

%%

%{
    /* Syntax */
%}

","|"+"|"-"|"*"|":" {
    DEBUGF("%s\n", yytext);
    return yytext[0];
}

{newline}+ {
    DEBUGF("NEWLINE\n");
    return NEWLINE;
}

%{
    /* Directives */
%}

"db" {
    DEBUGF("DB\n");
    return DB;
}

"dd" {
    DEBUGF("DD\n");
    return DD;
}

"space" {
    DEBUGF("SPACE\n");
    return SPACE;
}

"define" {
    DEBUGF("DEFINE\n");
    return DEFINE;
}

%{
    /* Registers */
%}

"r"[1-6] {
    DEBUGF("R%d\n", yytext[1]);
    switch (yytext[1])
    {
    case '1':
        return R1;
    case '2':
        return R2;
    case '3':
        return R3;
    case '4':
        return R4;
    case '5':
        return R5;
    case '6':
        return R6;
    }
}

"sp" {
    DEBUGF("SP\n");
    return SP;
}

"lr" {
    DEBUGF("LR\n");
    return LR;
}

"dl" {
    DEBUGF("DL\n");
    return DL;
}

"st" {
    DEBUGF("ST\n");
    return ST;
}

%{
    /* Opcodes */
%}

"cmp" {
    DEBUGF("CMP\n");
    return CMP;
}

"tst" {
    DEBUGF("TST\n");
    return TST;
}

"jmp" {
    DEBUGF("JMP\n");
    return JMP;
}

"je" {
    DEBUGF("JE\n");
    return JE;
}

"jne" {
    DEBUGF("JNE\n");
    return JNE;
}

"call" {
    DEBUGF("CALL\n");
    return CALL;
}

"ret" {
    DEBUGF("RET\n");
    return RET;
}

"sti" {
    DEBUGF("STI\n");
    return STI;
}

"rstr" {
    DEBUGF("RSTR\n");
    return RSTR;
}

"rti" {
    DEBUGF("RTI\n");
    return RTI;
}

"mov" {
    DEBUGF("MOV\n");
    return MOV;
}

"load" {
    DEBUGF("LOAD\n");
    return LOAD;
}

"loadb" {
    DEBUGF("LOADB\n");
    return LOADB;
}

"str" {
    DEBUGF("STR\n");
    return STR;
}

"strb" {
    DEBUGF("STRB\n");
    return STRB;
}

"push" {
    DEBUGF("PUSH\n");
    return PUSH;
}

"pushw" {
    DEBUGF("PUSHW\n");
    return PUSHW;
}

"pushb" {
    DEBUGF("PUSHB\n");
    return PUSHB;
}

"pop" {
    DEBUGF("POP\n");
    return POP;
}

"popw" {
    DEBUGF("POPW\n");
    return POPW;
}

"popb" {
    DEBUGF("POPB\n");
    return POPB;
}

"memcpy" {
    DEBUGF("MEMCPY\n");
    return MEMCPY;
}

"clrset" {
    DEBUGF("CLRSET\n");
    return CLRSET;
}

"clrsetv" {
    DEBUGF("CLRSETV\n");
    return CLRSETV;
}

"read" {
    DEBUGF("READ\n");
    return READ;
}

"write" {
    DEBUGF("WRITE\n");
    return WRITE;
}

"halt" {
    DEBUGF("HALT\n");
    return HALT;
}

"idle" {
    DEBUGF("IDLE\n");
    return IDLE;
}

"add" {
    DEBUGF("ADD\n");
    return ADD;
}

"inc" {
    DEBUGF("INC\n");
    return INC;
}

"sub" {
    DEBUGF("SUB\n");
    return SUB;
}

"dec" {
    DEBUGF("DEC\n");
    return DEC;
}

"mulw" {
    DEBUGF("MULW\n");
    return MULW;
}

"mul" {
    DEBUGF("MUL\n");
    return MUL;
}

%{
    /* Generic */
%}

{ident} {
    DEBUGF("ID\n");
    yylval.id = inventoryString(strdup(yytext));
    return ID;
}

"0x"{hexdigit}+ {
    DEBUGF("INTVAL\n");
    yylval.int32val = static_cast<uint32_t>( strtoul(yytext+2, 0, 16) );
    return INTVAL;
}

"0"{octdigit}* {
    DEBUGF("INTVAL\n");
    yylval.int32val = static_cast<uint32_t>( strtoul(yytext+1, 0, 8) );
    return INTVAL;
}

"0b"{bindigit}* {
    DEBUGF("INTVAL\n");
    yylval.int32val = static_cast<uint32_t>( strtoul(yytext+2, 0, 2) );
    return INTVAL;
}

[1-9]{digit}* {
    DEBUGF("INTVAL\n");
    yylval.int32val = static_cast<uint32_t>( strtoul(yytext, 0, 10) );
    return INTVAL;
}

{digit}*\.{digit}*f {
    DEBUGF("DBLVAL\n");
    return DBLVAL;
}

{digit}*\.{digit}+ {
    DEBUGF("FLTVAL\n");
    return FLTVAL;
}

{string} {
    DEBUGF("STRVAL\n");
    int idLen = strlen(yytext) - 2 /* don't include quotations */;
    char* id = inventoryString(strndup(yytext+1, idLen + 1 /* null terminator */));
    id[idLen] = 0;
    yylval.id = id;
    return STRVAL;
}

{char} {
    DEBUGF("CHARVAL\n");
    return CHARVAL;
}

{intrawhtsp}+   { /* eat intra-line whitespace */  }

";".*           { /* eat comments */ }

. {
    fprintf(stderr, "Unexpected input at line %d: '%s'\n", yylineno, yytext);
    exit(1);
}
