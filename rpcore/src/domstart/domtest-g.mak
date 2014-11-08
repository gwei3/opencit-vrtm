RPROOT=../..
BIN=        $(RPROOT)/bin/debug
OBJ=        $(RPROOT)/build/debug/rptoolobjects
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
TAO_MODS=   ../tao_mods

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
CFLAGS=     -D LINUX  -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(DEBUG_CFLAGS)
LDFLAGS          := $(RELEASE_LDFLAGS)
O1CFLAGS=    -D LINUX -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

dobjs=      $(OBJ)/domstart-g.o $(OBJ)/tcpchan.o 

all: $(BIN)/domtest

$(BIN)/domtest: $(dobjs)
	@echo "domtest"
	$(LINK) -o $(BIN)/domtest $(dobjs) $(LDFLAGS) -lpthread

$(OBJ)/domstart-g.o: $(S)/domstart.c
	$(CC) $(CFLAGS) -I$(TAO_MODS) -c -o $(OBJ)/domstart-g.o $(TRS)/domstart.c

$(OBJ)/tcpchan.o: $(TAO_MODS)/tcpchan.cpp $(TAO_MODS)/tcpchan.h
	$(CC) $(CFLAGS) -I$(TAO_MODS) -c -o $(OBJ)/tcpchan.o $(TAO_MODS)/tcpchan.cpp
