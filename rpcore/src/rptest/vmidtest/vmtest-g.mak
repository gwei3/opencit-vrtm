RPROOT=../../..
BIN=        $(RPROOT)/bin/debug
OBJ=        $(RPROOT)/build/debug/vmidtestobjects
#BIN=        $(RPROOT)/bin/release
#OBJ=        $(RPROOT)/build/release/rptoolobjects
#LIBHOME=    /home/s1
#LIB=        $(LIBHOME)/rp/lib
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

S=          .
T=          .
SC=         ../../fileProxy/Code/commonCode
SCC=        ../fileProxy/Code/jlmcrypto
BSC=        ../fileProxy/Code/oldjlmbignum
TH=         ../fileProxy/Code/tao
TRS=	    .
TS=	    ../TPMDirect
CH=         ../../fileProxy/Code/channels
TCS =	    ../fileProxy/Code/tcService
TAO_MODS=   ../../rpcore
TM= ../../rpchannel

PYTHON=		/usr/include/python2.7/

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
CFLAGS=     -D LINUX  -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(DEBUG_CFLAGS) -fPIC
LDFLAGS          := $(RELEASE_LDFLAGS)
O1CFLAGS=    -D LINUX -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

dobjs=      $(OBJ)/vmidtest.o  


all: $(BIN)/vmidtest

$(BIN)/vmidtest: $(dobjs)
	@echo "vmmtest"
	$(LINK) -o $(BIN)/vmidtest $(dobjs) $(LDFLAGS) -lpthread -L$(LIB) -lrpchannel-g
	

$(OBJ)/vmidtest.o: $(S)/vmidtest.cpp
	$(CC) $(CFLAGS) -I$(TAO_MODS) -I$(TM) -c -o $(OBJ)/vmidtest.o $(TRS)/vmidtest.cpp


