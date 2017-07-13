#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -g -O0 -Wall
CXXFLAGS = -g -O0 -frtti -fexceptions -Wall
INCPATH = #-I/usr/include/mysql
LD = gcc
LFLAGS = #-static
LIBS = -L. -lstdc++

STATIC_LIB = libzjh.a

OBJECTS_LIB = XtCard.o XtDeck.o  XtTypeDeck.o XtHoleCards.o XtShuffleDeck.o

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
