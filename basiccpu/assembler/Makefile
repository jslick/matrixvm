LEX=lex
YACC=yacc
CC=gcc
CPP=g++

YFLAGS=-t -v

## Compiler options
# necessary options
CFLAGS:=$(CFLAGS) -I ../../
# debug flags
CFLAGS:=$(CFLAGS) -g3
# warnings
CFLAGS:=$(CFLAGS) -Wall -Wno-unused-label -Wno-unused

CPPFLAGS:=$(CFLAGS) -std=c++0x

EXE=assembler
OBJS=$(EXE).yacc.o $(EXE).lex.o assembler_util.o program.o isa.o
BASIC_OS_PROG=looping_funcs.s

all:  $(EXE)

$(EXE):  $(OBJS)
	$(CPP) $(LFLAGS) $+ -o $@

assembler.lex.o:  assembler.lex.cpp
	[ -f assembler.yacc.hpp ] || ln -s assembler.yacc.h{,pp}
	$(CPP) $(CPPFLAGS) -c $< -o $@
assembler.yacc.o:  assembler.yacc.cpp assembler.yacc.hpp
	$(CPP) $(CPPFLAGS) -c $< -o $@
assembler_util.o:  assembler_util.cpp assembler_util.hpp
	$(CPP) $(CPPFLAGS) -c $< -o $@
program.o:  program.cpp program.hpp
	$(CPP) $(CPPFLAGS) -c $< -o $@
isa.o:  isa.cpp isa.hpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

%.lex.cpp: %.l
	$(LEX) $(LEXFLAGS) -o $@ $<

%.yacc.cpp: %.ypp
	$(YACC) $(YFLAGS) -d -o $@ $<

assembler.lex.cpp:  assembler.yacc.hpp

assembler.yacc.hpp: assembler.yacc.cpp

basic_os:  $(EXE) basic_os.s $(BASIC_OS_PROG)
	cat basic_os.s $(BASIC_OS_PROG) | ./$(EXE) --debug -o ../bios

clean:
	$(RM) *.o
	$(RM) *.lex.cpp
	$(RM) *.yacc.cpp
	$(RM) *.yacc.hpp
	$(RM) *.yacc.output
	$(RM) $(EXE)

.phony:  all clean
