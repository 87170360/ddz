#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -O0 -Wall
# CXXFLAGS = -O0 -frtti -fexceptions -Wall
CXXFLAGS = -g -O0 -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -Wall -DDEBUG
INCPATH = -I/download/jsoncpp-src-0.5.0/include -I../libddz -I/usr/local/include

LD = gcc
LFLAGS = #-static
#LIBS = -L. -L../libddz -L/usr/local/lib -lstdc++ -lev -ljsoncpp -lhiredis
LIBS =/usr/local/lib/libjsoncpp.a /usr/local/lib/libhiredis.a -L. -L../libddz -lddz -L/usr/local/lib -lstdc++ -lev 

DESTDIR_TARGET = hlddz_robot
OBJECTS = main.o XtRobotClient.o jpacket.o Robots.o

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
