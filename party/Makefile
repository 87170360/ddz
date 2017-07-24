#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -O0 -Wall
# CXXFLAGS = -O0 -frtti -fexceptions -Wall
CXXFLAGS = -g -O0 -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -Wall -DDEBUG -DAK_DEBUG
INCPATH = -I/download/jsoncpp-src-0.5.0/include -I../libzjh -I/usr/local/include -I../zjhsvr -I../libxtnet 

LD = gcc
LFLAGS = #-static
LIBS = /usr/local/lib/libjsoncpp.a /usr/local/lib/libhiredis.a -L. -L../libzjh -lzjh -L/usr/local/lib -lstdc++ -lev -L/usr/lib64/mysql -lmysqlclient -lm
#LIBS = -L. -L../libzjh -lzjh -L/usr/local/lib  -lstdc++ -lev -ljsoncpp -lhiredis  -L/usr/local/mysql/lib -lmysqlclient

DESTDIR_TARGET = party
OBJECTS = main.o PartyServer.o PartyClient.o PartyGame.o  PartyPlayer.o ../zjhsvr/log.o ../zjhsvr/jpacket.o  ../zjhsvr/daemonize.o ../zjhsvr/redis_client.o  ../libxtnet/XtSqlClient.o 

all: $(DESTDIR_TARGET)

$(DESTDIR_TARGET): $(OBJECTS)
	$(LD) $(LFLAGS) -o $(DESTDIR_TARGET) $(OBJECTS) $(LIBS)
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
	rm -rf $(DESTDIR_TARGET) *.o
