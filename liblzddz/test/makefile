#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -O0 -Wall
# CXXFLAGS = -O0 -frtti -fexceptions -Wall
CXXFLAGS = -g -O0 -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -Wall -DDEBUG
INCPATH = -I/usr/local/include -I../../hlddz  -I../  -I../../libxtgame -I../../libxtnet  -I../ 

LD = gcc
LFLAGS = #-static
LIBS = -L. -L../../liblzddz -L/usr/local/lib  -L../../libxtgame -L../../libxtnet -L../ -lxtgame  -llzddz -lxtnet -lstdc++ -lev -lhiredis 

test=test

testObj=test.o 

all: $(test)
	./$(test)

$(test) : $(testObj)
	$(LD) $(LFLAGS) -o $(test) $(testObj) $(LIBS) 
	ctags -R

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
	rm -f $(test) *.o
