RPROOT=../../..
BIN=        $(RPROOT)/bin/debug
LIB=        $(RPROOT)/lib
OBJ=        $(RPROOT)/build/debug/testobjects
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
TM= ../../rpchannel

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
CFLAGS=     -D LINUX  -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(DEBUG_CFLAGS)
LDFLAGS          := $(RELEASE_LDFLAGS)
O1CFLAGS=    -D LINUX -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

dobjs=       $(OBJ)/configtest.o 


all: $(BIN)/cfgtest

$(BIN)/cfgtest: $(dobjs)
	@echo "cfgtest"
	$(LINK) -o $(BIN)/cfgtest $(dobjs) $(LDFLAGS) -lpthread -L$(LIB) -lrpchannel-g

$(OBJ)/configtest.o: $(S)/configtest.cpp
	$(CC) $(CFLAGS) -I$(TAO_MODS) -I$(TM) -c -o $(OBJ)/configtest.o $(TRS)/configtest.cpp


