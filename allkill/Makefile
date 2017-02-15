#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -O0 -Wall
# CXXFLAGS = -O0 -frtti -fexceptions -Wall
CXXFLAGS = -g -O0 -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -Wall -DDEBUG -DAK_DEBUG
INCPATH = -I../libbull -I/usr/local/include -I../normal_bull -I../libxtnet 

LD = gcc
LFLAGS = #-static
LIBS = -L. -L../libbull -lbull -L/usr/local/lib  -lstdc++ -lev -ljsoncpp -lhiredis  -L/usr/local/mysql/lib -lmysqlclient

DESTDIR_TARGET = ak_bull
OBJECTS = main.o AllKillServer.o AllKillClient.o AllKillGame.o  AllKillPlayer.o ../normal_bull/log.o  ../normal_bull/daemonize.o ../normal_bull/redis_client.o  ../libxtnet/XtSqlClient.o 

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
