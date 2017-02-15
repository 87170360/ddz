#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -O0 -Wall
# CXXFLAGS = -O0 -frtti -fexceptions -Wall
CXXFLAGS = -g -O0 -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -Wall -DDEBUG -DBG_DEBUG
INCPATH = -I/usr/local/include -I../zjhsvr -I../libxtgame -I../libxtnet -I../libbull

LD = gcc
LFLAGS = #-static
LIBS = -L. -L../libbull -L../libxtgame -L../libxtnet  -L/usr/local/lib  -lstdc++ -lev  -lhiredis  -lxtgame -lxtnet -lzjh -ljsoncpp  -L/usr/local/mysql/lib -lmysqlclient

 
DESTDIR_TARGET = beauty
OBJECTS = main.o XtBeautyGame.o XtBeautyPlayer.o XtBeautyServer.o 

all: $(DESTDIR_TARGET)

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
