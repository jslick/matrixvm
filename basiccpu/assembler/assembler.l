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

%{
    /* Opcodes */
%}

"jmp" {
    DEBUGF("JMP\n");
    return JMP;
}

"call" {
    DEBUGF("CALL\n");
    return CALL;
}

"ret" {
    DEBUGF("RET\n");
    return RET;
}

"mov" {
    DEBUGF("MOV\n");
    return MOV;
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

"write" {
    DEBUGF("WRITE\n");
    return WRITE;
}

"halt" {
    DEBUGF("HALT\n");
    return HALT;
}

"add" {
    DEBUGF("ADD\n");
    return ADD;
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
