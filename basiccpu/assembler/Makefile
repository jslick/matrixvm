LEX=lex
YACC=yacc
CC=gcc

LEXFLAGS=
#YFLAGS=
YFLAGS=-t -v
CFLAGS=-Wall -Wno-unused-label -Wno-unused -DNDEBUG
#CFLAGS=-Wall -Wno-unused-label -Wno-unused

EXE=assembler
OBJS=$(EXE).yacc.o $(EXE).lex.o

all:  $(EXE)

$(EXE):  $(OBJS)
	$(CC) $(LFLAGS) $+ -o $@

%.o:  %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

%.lex.c: %.l
	$(LEX) $(LEXFLAGS) -o $@ $<

%.yacc.c: %.y
	$(YACC) $(YFLAGS) -d -o $@ $<

clean:
	$(RM) *.o
	$(RM) *.lex.c
	$(RM) *.yacc.c
	$(RM) *.yacc.h
	$(RM) *.yacc.output
	$(RM) $(EXE)

.phony:  all clean
