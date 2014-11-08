RPROOT=../../..
BIN=        $(RPROOT)/bin/debug
OBJ=        $(RPROOT)/build/debug/rpcryptoobjects
#BIN=        $(RPROOT)/bin/release
#OBJ=        $(RPROOT)/build/release/rptoolobjects
#LIBHOME=    /home/s1
#LIB=        $(LIBHOME)/rp/lib
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

S=          .
T=          .
SC=         ../fileProxy/Code/commonCode
SCC=        ../fileProxy/Code/jlmcrypto
BSC=        ../fileProxy/Code/oldjlmbignum
TH=         ../fileProxy/Code/tao
TRS=	    .
TS=	    ../TPMDirect
CH=	    ../channels
TCS =	    ../fileProxy/Code/tcService
TAO_MODS=   ../../rpcore
TM= ../../rpcore
PYTHON=		/usr/include/python2.7/

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
CFLAGS=     -D LINUX  -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(DEBUG_CFLAGS) -fPIC
LDFLAGS          := $(RELEASE_LDFLAGS)
O1CFLAGS=    -D LINUX -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

dobjs=      $(OBJ)/tcpchan.o $(OBJ)/vmstart.o $(OBJ)/vmstart_wrap.o


all: $(BIN)/vmstart

$(BIN)/vmstart: $(dobjs)
	@echo "vmmtest"
	#$(LINK) -o $(BIN)/vmstart $(dobjs) $(LDFLAGS) -lpthread -L$(LIB) -lrputil
	$(LINK) -shared -fPIC -o $(BIN)/_vmstart.so $(dobjs) $(LDFLAGS) -lpthread -L$(LIB) -lrputil

$(OBJ)/vmstart.o: $(S)/vmstart.cpp
	$(CC) $(CFLAGS) -I$(TAO_MODS) -I$(TM) -c -o $(OBJ)/vmstart.o $(TRS)/vmstart.cpp

$(OBJ)/vmstart_wrap.o: $(S)/vmstart.cpp
	$(CC) $(CFLAGS) -I$(TAO_MODS) -I$(TM) -I$(PYTHON) -c -o $(OBJ)/vmstart_wrap.o $(TRS)/vmstart_wrap.cxx

$(OBJ)/tcpchan.o: $(TM)/tcpchan.cpp $(TM)/tcpchan.h
	$(CC) $(CFLAGS) -I$(TM) -c -o $(OBJ)/tcpchan.o $(TM)/tcpchan.cpp
