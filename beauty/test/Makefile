#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -O0 -Wall
# CXXFLAGS = -O0 -frtti -fexceptions -Wall
CXXFLAGS = -g -O0 -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -Wall -DDEBUG
INCPATH = -I/usr/local/include -I../../zjhsvr  -I../  -I../../libxtgame -I../../libxtnet  -I../ 

LD = gcc
LFLAGS = #-static
LIBS = -L. -L../../zjhsvr/libbull -L/usr/local/lib  -L../../libxtgame -L../../libxtnet -L../ -lxtgame  -lzjh -lxtnet -lstdc++ -lev -ljsoncpp -lhiredis 




DESK_TEST_RAND= BeautyDeck_Test_Rand

DESK_TEST_TYPE= BeautyDeck_Test_Type

DESK_TEST_FAPAI=BeautyDeck_Test_FaPai

DESK_TEST=BeautyDeck_Test


DESK_TEST_FAPAI_OBJ= BeautyDeck_Test_FaPai.o   ../XtBeautyDeck.o 

DESK_TEST_OBJ=BeautyDeck_Test.o ../XtBeautyDeck.o

DESK_TEST_TYPE_OBJ=  BeautyDeck_Test_Type.o ../XtBeautyDeck.o

DESK_TEST_RAND_OBJ=  BeautyDeck_Test_Rand.o ../XtBeautyDeck.o


all: $(DESK_TEST_FAPAI)  $(DESK_TEST) $(DESK_TEST_TYPE) $(DESK_TEST_RAND)


$(DESK_TEST_TYPE) : $(DESK_TEST_TYPE_OBJ)
	$(LD) $(LFLAGS) -o $(DESK_TEST_TYPE) $(DESK_TEST_TYPE_OBJ) $(LIBS) 

$(DESK_TEST_FAPAI) : $(DESK_TEST_FAPAI_OBJ)
	$(LD) $(LFLAGS) -o $(DESK_TEST_FAPAI) $(DESK_TEST_FAPAI_OBJ) $(LIBS) 

$(DESK_TEST):$(DESK_TEST_OBJ)
	$(LD) $(LFLAGS) -o $(DESK_TEST) $(DESK_TEST_OBJ) $(LIBS) 

$(DESK_TEST_RAND): $(DESK_TEST_RAND_OBJ)
	$(LD) $(LFLAGS) -o $(DESK_TEST_RAND) $(DESK_TEST_RAND_OBJ) $(LIBS) 





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
