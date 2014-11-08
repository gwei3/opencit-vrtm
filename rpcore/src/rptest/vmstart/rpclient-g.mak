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
SWIG=	swig

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
CFLAGS=     -D LINUX  -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(DEBUG_CFLAGS) -fPIC
LDFLAGS          := $(RELEASE_LDFLAGS)
O1CFLAGS=    -D LINUX -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

dobjs=      $(OBJ)/tcpchan.o $(OBJ)/rpclient.o $(OBJ)/rpclient_wrap.o


all: $(BIN)/rpclient

$(BIN)/rpclient: $(dobjs)
	@echo "rpclient"
	$(LINK) -shared -fPIC -o _rpclient.so $(dobjs) $(LDFLAGS) -lpthread -L$(LIB) -lrputil

$(OBJ)/rpclient.o: $(S)/rpclient.cpp
	$(CC) $(CFLAGS) -I$(TAO_MODS) -I$(TM) -c -o $(OBJ)/rpclient.o $(TRS)/rpclient.cpp

$(OBJ)/rpclient_wrap.o: $(S)/rpclient_wrap.cxx
	$(SWIG) -c++ -python rpclient.i
	$(CC) $(CFLAGS) -I$(TAO_MODS) -I$(TM) -I$(PYTHON) -c -o $(OBJ)/rpclient_wrap.o $(TRS)/rpclient_wrap.cxx

$(OBJ)/tcpchan.o: $(TM)/tcpchan.cpp $(TM)/tcpchan.h
	$(CC) $(CFLAGS) -I$(TM) -c -o $(OBJ)/tcpchan.o $(TM)/tcpchan.cpp
