RPROOT=../..
BIN=        $(RPROOT)/bin/debug
OBJ=        $(RPROOT)/build/debug/rptrustobjects
#BIN=        $(RPROOT)/bin/release
#OBJ=        $(RPROOT)/build/release/rptrustobjects
#LIBHOME=    /home/s1
#LIB=        $(LIBHOME)/rp/lib
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc
C=          $(RPROOT)/src/rpcrypto/
D=          $(RPROOT)/src/fileProxy/Code/tcService
S=          .
T=	    .
SC=         ../fileProxy/Code/commonCode
SCC=        ../fileProxy/Code/jlmcrypto
BSC=        ../fileProxy/Code/jlmbignum
#TH=         ../../fileProxy/Code/tao
TRS=        ./tao
#TS=         ../../rptpm/TPMDirect
#TS=         ../../rptpm/TPMtcsd
CYP=        ../../rpcrypto
DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
CFLAGS=     -D LINUX  -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(DEBUG_CFLAGS)
LDFLAGS          := $(RELEASE_LDFLAGS)
O1CFLAGS=    -D LINUX -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

TBOOTINCLUDE= ../../fileProxy/tboot-1.7.3/tboot/include/ 
TBOOTINCLUDE2=../../fileProxy/tboot-1.7.3/include
SHA1SRC=../../fileProxy/tboot-1.7.3/tboot/common

CC=         g++
LINK=       g++

dobjs=      $(OBJ)/RP_RSAKey-helper.o 
dobjs1=     $(OBJ)/rsakey-helper-test.o 

all: mainfun

mainfun: $(dobjs) $(dobjs1) 
	@echo "rsakey-helper-test.cpp"
	$(LINK) -o $(BIN)/rsakey-helper-test $(dobjs) $(dobjs1) $(LDFLAGS) -lpthread -L$(LIB) -lrpcrypto-g -lcrypto 

$(dobjs): $(TRS)/RP_RSAKey-helper.cpp
	$(CC) $(CFLAGS) -I$(TAO_MODS) -I$(BSC) -I$(SCC) -I$(CYP) -I$(TRS) -I$(SC) -I$(BSC) -I$(SCC) -I$(C) -I$(D)  -c -o $(OBJ)/RP_RSAKey-helper.o $(TRS)/RP_RSAKey-helper.cpp

$(dobjs1): $(TRS)/rsakey-helper-test.cpp
	$(CC) $(CFLAGS) -I$(TAO_MODS) -I$(BSC) -I$(SCC) -I$(CYP) -I$(TRS) -I$(SC) -I$(BSC) -I$(SCC) -I$(C) -I$(D)  -c -o $(OBJ)/rsakey-helper-test.o $(TRS)/rsakey-helper-test.cpp 



