# Makefile
# Requires that the object definitions be in wl.h and wl.cpp

CXX =           gcc
CXXFLAGS =     -O -Wall -g

# During debugging you may want to used the compiler flags listed below
#CXXFLAGS =      -g -Wall

all: mysh

wl: mysh.c
	$(CXX) $(CXXFLAGS) mysh.c -o $@

clean:
	rm -f core *.o mysh