#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -O0 -Wall
# CXXFLAGS = -O0 -frtti -fexceptions -Wall
CXXFLAGS = -g -O0 -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -Wall -DDEBUG
INCPATH = -I/data/download/jsoncpp-src-0.5.0/include -I../libddz -I/usr/local/include

LD = gcc
LFLAGS = #-static
LIBS = /usr/local/lib/libjsoncpp.a /usr/local/lib/libhiredis.a -L. -L../libddz -L/usr/local/lib -lddz -lstdc++ -lev -lm
#LIBS = -L. -L../libddz -L/usr/local/lib -lzjh -lstdc++ -lev -ljsoncpp -lhiredis

DESTDIR_TARGET = lzddz
OBJECTS = lzddz.o daemonize.o log.o client.o game.o jpacket.o table.o player.o redis_client.o eventlog.o

all: $(DESTDIR_TARGET)
	ctags -R

cp: all
	cp -f $(DESTDIR_TARGET) /data/game/bin

$(DESTDIR_TARGET): $(OBJECTS)
	$(LD) $(LFLAGS) -o $(DESTDIR_TARGET) $(OBJECTS) $(LIBS)

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
