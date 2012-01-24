CXX=g++
CXXFLAGS:=-I ../../ $(CXXFLAGS) -g3 -std=c++0x -Wall
LFLAGS:=

EXE=assembler
OBJS=assembler.o program.o isa.o

all:  $(EXE)

$(EXE):  $(OBJS)
	$(CXX) $(LFLAGS) $+ -o $@

%.o:  %.cpp %.h ../../common.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(EXE) *.o

.phony:  all clean
