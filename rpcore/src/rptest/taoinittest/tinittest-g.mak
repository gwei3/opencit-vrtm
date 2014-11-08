RPROOT=../../..
BIN=        $(RPROOT)/bin/debug
OBJ=        $(RPROOT)/build/debug/testobjects
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
#TS=	    ../TPMDirect
CH=         ../../rpchannel
#TCS =	    ../fileProxy/Code/tcService
TAO_MODS=   ../../rpcore
TM= ../../rpcore

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
CFLAGS=     -D LINUX  -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(DEBUG_CFLAGS)
LDFLAGS          := $(RELEASE_LDFLAGS)
O1CFLAGS=    -D LINUX -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

dobjs=      $(OBJ)/taoinittest.o 


all: $(BIN)/titest

$(BIN)/titest: $(dobjs)
	@echo "cfgtest"
	$(LINK) -o $(BIN)/titest $(dobjs) $(LDFLAGS) -lpthread -L$(LIB) -lrpchannel-g

$(OBJ)/taoinittest.o: $(S)/taoinittest.cpp
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -I$(TAO_MODS) -I$(TM) -I$(SC)  -I$(CH) -c -o $(OBJ)/taoinittest.o $(TRS)/taoinittest.cpp




