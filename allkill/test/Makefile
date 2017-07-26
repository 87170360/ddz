#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -O0 -Wall
# CXXFLAGS = -O0 -frtti -fexceptions -Wall
CXXFLAGS = -g -O0 -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -Wall -DDEBUG
INCPATH = -I/download/jsoncpp-src-0.5.0/include -I/usr/local/include -I../../zjhsvr  -I../ 

LD = gcc
LFLAGS = #-static
LIBS = /usr/local/lib/libjsoncpp.a /usr/local/lib/libhiredis.a -L. -L./zjhsvr/libzjh -L/usr/local/lib  -lstdc++ -lev
#LIBS = -L. -L./zjhsvr/libzjh -L/usr/local/lib  -lstdc++ -lev -ljsoncpp -lhiredis 


ROLE_CLIENT=role_client

BET_CLIENT=bet_client



OBJECTS =  ../AllKillClient.o  ../../zjhsvr/jpacket.o ../../zjhsvr/log.o

all: $(ROLE_CLIENT) $(BET_CLIENT)

$(ROLE_CLIENT): $(OBJECTS) $(ROLE_CLIENT).o
	$(LD) $(LFLAGS) -o $(ROLE_CLIENT) $(OBJECTS) $(ROLE_CLIENT).o $(LIBS) 

$(BET_CLIENT): $(OBJECTS) $(BET_CLIENT).o
	$(LD) $(LFLAGS) -o $(BET_CLIENT) $(OBJECTS) $(BET_CLIENT).o $(LIBS) 




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

cp:
	cp $(BET_CLIENT) /data/game/bin
