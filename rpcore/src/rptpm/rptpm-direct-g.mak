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
TS=         ../rptpm/TPMDirect
TRT=        ../rptrust
TAO=        ../rpcore/tao
CLM=        ../fileProxy/Code/claims
RPCYP=      .
TM=			../rptpm

DEBUG_CFLAGS     := -Wall -Wno-format -g -DDEBUG  -fPIC
RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O3 
LDFLAGSXML      := ${RELEASE_LDFLAGS}
CFLAGS=      -D QUOTE2_DEFINED $(DEBUG_CFLAGS) -DRPNOMAIN  -D FILECLIENT -D RPLIB -D TEST -D TEST -D CRYPTOTEST4 -D CRYPTOTEST -D CRYPTOTEST1 -DTEST_D -DTEST_DD -D TEST1
#CFLAGS=      -D NOAESNI $(RELEASE_LDFLAGS) -DRPNOMAIN  -D FILECLIENT -D RPLIB
CFLAGS1=     -D NOAESNI -Wall -Wno-unknown-pragmas -Wno-format -O1 -DRPNOMAIN -fPIC

CC=         g++
LINK=       g++

lobjs=      $(OBJ)/vTCIDirect.o $(OBJ)/hashprep.o $(OBJ)/hmacsha1.o

#$(OBJ)/channelcoding.o \

all: $(LIB)/librptpm-dir-g.a

	
$(OBJ)/vTCIDirect.o: $(TS)/vTCIDirect.cpp $(TS)/vTCIDirect.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(TS) -I$(SBM) -c -o $(OBJ)/vTCIDirect.o $(TS)/vTCIDirect.cpp
	
$(OBJ)/hashprep.o: $(TS)/hashprep.cpp $(TS)/hashprep.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(CLM) -I$(TH) -I$(TS) -c -o $(OBJ)/hashprep.o $(TS)/hashprep.cpp

$(OBJ)/hmacsha1.o: $(TS)/hmacsha1.cpp $(TS)/hmacsha1.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(TS) -I$(SBM) -c -o $(OBJ)/hmacsha1.o $(TS)/hmacsha1.cpp
	
$(LIB)/librptpm-dir-g.a: $(lobjs)
	@echo "Building librptpm-dir-g.a ..."
	ar cr $(LIB)/librptpm-dir-g.a  $(lobjs) 


