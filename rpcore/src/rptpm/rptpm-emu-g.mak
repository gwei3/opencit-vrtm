RPROOT= ../..
BIN=         $(RPROOT)/bin/debug
OBJ=         $(RPROOT)/build/debug/rptpmobjects
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
TPM=	    ../fileProxy/Code/TPMDirect
TS=         ../fileProxy/Code/tcService
TRT=        ../rptrust
TAO=        ../fileProxy/Code/tao
CLM=        ../fileProxy/Code/claims
RPCYP=      .
TM=			../rpcore/tao

DEBUG_CFLAGS     := -Wall -Wno-format -g -DDEBUG  -fPIC
RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O3 
LDFLAGSXML      := ${RELEASE_LDFLAGS}
CFLAGS=      -D QUOTE2_DEFINED $(DEBUG_CFLAGS) -DRPNOMAIN  -D FILECLIENT -D RPLIB -D TEST -D TEST -D CRYPTOTEST4 -D CRYPTOTEST -D CRYPTOTEST1 -DTEST_D -DTEST_DD -D TEST1
#CFLAGS=      -D NOAESNI $(RELEASE_LDFLAGS) -DRPNOMAIN  -D FILECLIENT -D RPLIB
CFLAGS1=     -D NOAESNI -Wall -Wno-unknown-pragmas -Wno-format -O1 -DRPNOMAIN -fPIC

CC=         g++
LINK=       g++

lobjs=      $(OBJ)/emuTPMHostsupport.o 

#$(OBJ)/channelcoding.o \

all: $(LIB)/librptpm-emu-g.a

$(OBJ)/emuTPMHostsupport.o: $(TM)/emuTPMHostsupport.cpp $(TM)/TPMHostsupport.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/emuTPMHostsupport.o $(TM)/emuTPMHostsupport.cpp
	
$(LIB)/librptpm-emu-g.a: $(lobjs)
	@echo "Building librptpm-emu-g.a ..."
	ar cr $(LIB)/librptpm-emu-g.a  $(lobjs) 


