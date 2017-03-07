#rocenting@gmail.com
CC = gcc
CXX = g++
CFLAGS = -O0 -Wall
# CXXFLAGS = -O0 -frtti -fexceptions -Wall
CXXFLAGS = -g -O0 -fno-strict-aliasing -Wno-write-strings -frtti -fexceptions -Wall -DDEBUG
INCPATH = -I/usr/local/include -I../../zjhsvr  -I../  -I../../libxtgame -I../../libxtnet  -I../ 

LD = gcc
LFLAGS = #-static
LIBS = -L. -L../../zjhsvr/libddz -L/usr/local/lib  -L../../libxtgame -L../../libxtnet -L../ -lxtgame  -lddz -lxtnet -lstdc++ -lev -ljsoncpp -lhiredis 

DESK_TEST_RAND= Deck_Test_Rand
DESK_TEST_TYPE= Deck_Test_Type
DESK_TEST_FAPAI=Deck_Test_FaPai
DESK_TEST=Deck_Test
shuffleDeckTest=shuffleDeckTest

DESK_TEST_FAPAI_OBJ= Deck_Test_FaPai.o  
DESK_TEST_OBJ=Deck_Test.o 
DESK_TEST_TYPE_OBJ=  Deck_Test_Type.o 
DESK_TEST_RAND_OBJ=  Deck_Test_Rand.o 
shuffleDeckTestObj=shuffleDeckTest.o

all: $(shuffleDeckTest)

$(shuffleDeckTest) : $(shuffleDeckTestObj)
	$(LD) $(LFLAGS) -o $(shuffleDeckTest) $(shuffleDeckTestObj) $(LIBS) 

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
	rm -f $(shuffleDeckTest) $(DESK_TEST_FAPAI) $(DESK_TEST) $(DESK_TEST_TYPE) $(DESK_TEST_RAND) *.o
