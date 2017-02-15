#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -O0 -Wall
# CXXFLAGS = -O0 -frtti -fexceptions -Wall
CXXFLAGS = -g -O0 -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -Wall -DDEBUG
INCPATH = -I/usr/local/include   -I../  -I../../libxtgame -I../../libxtnet  -I../ 

LD = gcc
LFLAGS = #-static
LIBS = -L. -L../../zjhsvr/libbull -L/usr/local/lib  -L../../libxtgame -L../../libxtnet -L../ -lxtgame  -lxtnet  -lstdc++ -lev -ljsoncpp -lhiredis  -L/usr/local/mysql/lib -lmysqlclient  







BET_CLIENT=lottery_client


OBJECTS = lottery_client.o 


all: $(BET_CLIENT)  



$(BET_CLIENT): $(OBJECTS) 
	$(LD) $(LFLAGS) -o $(BET_CLIENT) $(OBJECTS) $(LIBS) 






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
	rm -rf $(DESTDIR_TARGET) *.o
