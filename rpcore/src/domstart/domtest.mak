RPROOT=../..
#BIN=        $(RPROOT)/bin/debug
#OBJ=        $(RPROOT)/build/debug/rptoolobjects
BIN=        $(RPROOT)/bin/release
OBJ=        $(RPROOT)/build/release/rptoolobjects
#LIBHOME=    /home/s1
#LIB=        $(LIBHOME)/rp/lib
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

S=	    .
SC=         ../fileProxy/Code/commonCode
SCC=        ../fileProxy/Code/jlmcrypto
BSC=        ../fileProxy/Code/oldjlmbignum
TH=         ../fileProxy/Code/tao
TRS=	    .
TS=	    ../TPMDirect
CH=	    ../channels
TCS =	    ../fileProxy/Code/tcService
TAO_MODS=   ../tao_mods
TM= ../tao_mods
DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
CFLAGS=     -D LINUX  -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(DEBUG_CFLAGS)
LDFLAGS          := $(RELEASE_LDFLAGS)
O1CFLAGS=    -D LINUX -D TIXML_USE_STL -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

dobjs=      $(OBJ)/tcpchan.o $(OBJ)/domstart.o 


all: $(BIN)/domtest

$(BIN)/domtest: $(dobjs)
	@echo "domtest"
	$(LINK) -o $(BIN)/domtest $(dobjs) $(LDFLAGS) -lpthread

$(OBJ)/domstart.o: $(S)/domstart.c
	$(CC) $(CFLAGS) -I$(TAO_MODS) -I$(TM) -c -o $(OBJ)/domstart.o $(TRS)/domstart.c

$(OBJ)/tcpchan.o: $(TM)/tcpchan.cpp $(TM)/tcpchan.h
	$(CC) $(CFLAGS) -I$(TM) -c -o $(OBJ)/tcpchan.o $(TM)/tcpchan.cpp
