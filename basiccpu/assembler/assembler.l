%{

#include "assembler.yacc.h"

#include <stdio.h>

#ifndef NDEBUG
#    define DEBUGF(...) printf(__VA_ARGS__)
#else
#    define DEBUGF(...)
#endif

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

","|"-"|":" {
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

%{
    /* Registers */
%}

"r1" {
    DEBUGF("R1\n");
    return R1;
}

"r2" {
    DEBUGF("R2\n");
    return R2;
}

%{
    /* Opcodes */
%}

"jmp" {
    DEBUGF("JMP\n");
    return JMP;
}

"mov" {
    DEBUGF("MOV\n");
    return MOV;
}

"memcpy" {
    DEBUGF("MEMCPY\n");
    return MEMCPY;
}

"write" {
    DEBUGF("WRITE\n");
    return WRITE;
}

"halt" {
    DEBUGF("HALT\n");
    return HALT;
}

%{
    /* Generic */
%}

%{
    /*
"#"{ident} {
    DEBUGF("IMMEDIATE\n");
    yylval.id = strdup(yytext+1);
    return IMMEDIATE;
}
    */
%}

{ident} {
    DEBUGF("ID\n");
    yylval.id = strdup(yytext);
    return ID;
}

"0x"{hexdigit}+ {
    DEBUGF("INTVAL\n");
    return INTVAL;
}

"0"{octdigit}* {
    DEBUGF("INTVAL\n");
    return INTVAL;
}

[1-9]{digit}* {
    DEBUGF("INTVAL\n");
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
