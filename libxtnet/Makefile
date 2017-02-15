CC = gcc
CXX = g++
CFLAGS = -g -O0 -Wall
CXXFLAGS = -g -O0 -frtti -fexceptions -Wall
INCPATH = -I/usr/local/include -I.
LD = gcc
LFLAGS = #-static
LIBS = -L. -lstdc++

STATIC_LIB = libxtnet.a
OBJECTS_LIB = XtLog.o XtRedisClient.o XtRedisSubscribe.o  XtBuffer.o XtEncrypt.o  XtJsonPacket.o XtStreamClient.o  protocol/XtProtocol.o protocol/XtMsgProtocol.o XtSqlClient.o daemonize.o
 


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
