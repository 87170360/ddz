#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -O0 -Wall
# CXXFLAGS = -O0 -frtti -fexceptions -Wall
CXXFLAGS = -g -O0 -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -Wall -DDEBUG
INCPATH = -I../libbull -I/usr/local/include

LD = gcc
LFLAGS = #-static
#LIBS = -L. -L../libbull -L/usr/local/lib -lstdc++ -lev -ljsoncpp -lhiredis
LIBS = -L. -L../libbull -L/usr/local/lib -lbull -lstdc++ -lev -ljsoncpp -lhiredis
DESTDIR_TARGET = grab_robot
OBJECTS = main.o XtRobotClient.o Robots.o

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
