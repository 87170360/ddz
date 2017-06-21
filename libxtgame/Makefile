CC = gcc
CXX = g++
CFLAGS = -g -O0 -Wall
CXXFLAGS = -g -O0 -frtti -fexceptions -Wall -DXT_DEBUG
INCPATH = -I/usr/local/include -I../libxtnet
LD = gcc
LFLAGS = #-static 
LIBS = -L. -lstdc++

STATIC_LIB = libxtgame.a
OBJECTS_LIB = XtGameClient.o XtGameServer.o  XtGameLogic.o 



lib: $(STATIC_LIB)

$(STATIC_LIB): $(OBJECTS_LIB)
	ar -r $(STATIC_LIB) $(OBJECTS_LIB)

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
	rm -rf $(DESTDIR_TARGET) *.a
