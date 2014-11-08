RPROOT= ../..
BIN=         $(RPROOT)/bin/debug
OBJ=         $(RPROOT)/build/debug/rpcryptoobjects
LIB=         $(RPROOT)/lib
INC=         $(RPROOT)/inc

#BIN=         ~/rp/bin/debug
#OBJ=         ~/rp/build/debug/rpcryptoobjects
#LIB=         ~/rp/lib
#INC=         ../../inc

S=          ../fileProxy/Code/cryptUtility
SC=         ../fileProxy/Code/commonCode
SCC=	    ../fileProxy/Code/jlmcrypto
SBM=	    ../fileProxy/Code/jlmbignum
#TPM=	    ../fileProxy/Code/TPMDirect
TS=         ../fileProxy/Code/tcService
TRT=        ../rpcore
TAO=        ../fileProxy/Code/tao
CLM=        ../fileProxy/Code/claims
RPCYP=      .
TM=			../measurer

DEBUG_CFLAGS     := -Wall -Wno-format -g -DDEBUG  -fPIC
RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O3 
LDFLAGSXML      := ${RELEASE_LDFLAGS}
CFLAGS=      -D NOAESNI $(DEBUG_CFLAGS) -DRPNOMAIN  -D FILECLIENT -D RPLIB -D TEST -D TEST -D CRYPTOTEST4 -D CRYPTOTEST -D CRYPTOTEST1 -DTEST_D -DTEST_DD -D TEST1
#CFLAGS=      -D NOAESNI $(RELEASE_LDFLAGS) -DRPNOMAIN  -D FILECLIENT -D RPLIB
CFLAGS1=     -D NOAESNI -Wall -Wno-unknown-pragmas -Wno-format -O1 -DRPNOMAIN -fPIC

CC=         g++
LINK=       g++

lobjs=      $(OBJ)/dombuilder.o 

#$(OBJ)/channelcoding.o \

all: $(LIB)/librpmeasurer-g.a

$(OBJ)/dombuilder.o: $(TM)/dombuilder.c $(TM)/dombuilder.h
	gcc $(CFLAGS) -I$(INC) -I$(TM) -I$(SC) -I$(S) -I$(TRT) -c -o $(OBJ)/dombuilder.o $(TM)/dombuilder.c
	
$(LIB)/librpmeasurer-g.a: $(lobjs)
	@echo "Building librpmeasurer-g.a ..."
	ar cr $(LIB)/librpmeasurer-g.a  $(lobjs) 


