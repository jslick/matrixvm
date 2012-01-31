%{

#include <stdio.h>
#include <string.h>

extern int yylineno;
int yydebug = 1;

extern void yyerror(const char*);
extern int yylex();

%}

%union {
    char*   id;
    /*double  dbl;*/
}

%token NEWLINE
%token DB
%token R1 R2
%token JMP MOV MEMCPY WRITE HALT
%token IMMEDIATE
%token <id> ID
%token INTVAL DBLVAL FLTVAL STRVAL CHARVAL

%%

program
    : line program2
    ;

program2
    : line program2
    |
    ;

line
    : label_dcl NEWLINE
    | directive_stmt NEWLINE
    | instruction NEWLINE
    | NEWLINE
    ;

label_dcl
    : ID ':' { /* TODO */ }
    ;

directive_stmt
    : directive directive_args
    ;

directive
    : DB
    ;

directive_args
    : directive_arg directive_args
    |
    ;

directive_arg
    : INTVAL
    |
    STRVAL
    ;

instruction
    : opcode instruction_args
    | opcode
    ;

opcode
    : JMP | MOV | MEMCPY | WRITE | HALT
    ;

instruction_args
    : instruction_arg
    | instruction_arg ',' instruction_args
    ;

instruction_arg
    : register
    | immediate
    ;

register
    : R1 | R2
    ;

immediate
    : immediate_expr
    ;

immediate_expr
    : immediate_expr bin_op simple_expr
    | simple_expr
    ;

simple_expr
    : ID
    | INTVAL
    ;

bin_op
    : '-'
    ;

%%

void yyerror(const char *str)
{
    fprintf(stderr,"error: %s\n",str);
}

int main()
{
    return yyparse();
}
