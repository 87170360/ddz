#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -O0 -Wall
# CXXFLAGS = -O0 -frtti -fexceptions -Wall
CXXFLAGS = -g -O0 -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -Wall -DDEBUG
INCPATH = -I/usr/local/include -I../../zjhsvr  -I../  -I../../libxtgame -I../../libxtnet  -I../ 

LD = gcc
LFLAGS = #-static
LIBS = -L. -L../../zjhsvr/libddz -L/usr/local/lib  -L../../libxtgame -L../../libxtnet -L../ -lxtgame  -lddz -lxtnet -lstdc++ -lev -ljsoncpp -lhiredis 

shuffleDeckTest=shuffleDeckTest

shuffleDeckTestObj=shuffleDeckTest.o

all: $(shuffleDeckTest)
	ctags -R

$(shuffleDeckTest) : $(shuffleDeckTestObj)
	$(LD) $(LFLAGS) -o $(shuffleDeckTest) $(shuffleDeckTestObj) $(LIBS) 

####### Implicit rules ######

.SUFFIXES: .cpp .cc .cxx .c
.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

clean:
	rm -f $(shuffleDeckTest) *.o
