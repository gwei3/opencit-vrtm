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
#TPM=	    ../fileProxy/Code/TPMtcsd
TS=         ../rptpm/TPMtcsd
TRT=        ../rptrust
TAO=        ../rpcore/tao
CLM=        ../fileProxy/Code/claims
RPCYP=      .
TM=			../rptpm

DEBUG_CFLAGS     := -Wall -Wno-format -g -DDEBUG  -fPIC
RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O3 
LDFLAGSXML      := ${RELEASE_LDFLAGS}
CFLAGS=      -D QUOTE2_DEFINED $(DEBUG_CFLAGS) -DRPNOMAIN  -D FILECLIENT -D RPLIB -D TPMTEST -D TEST  -DTEST_D -DTEST_DD -D TEST1 
#CFLAGS=      -D NOAESNI $(RELEASE_LDFLAGS) -DRPNOMAIN  -D FILECLIENT -D RPLIB
CFLAGS1=     -D NOAESNI -Wall -Wno-unknown-pragmas -Wno-format -O1 -DRPNOMAIN -fPIC

CC=         g++
LINK=       g++

lobjs=      $(OBJ)/vTCI.o $(OBJ)/hashprep.o 

#$(OBJ)/hmacsha1.o


all: $(LIB)/librptpm-tcsd-g.a

	
$(OBJ)/vTCI.o: $(TS)/vTCI.cpp $(TS)/vTCI.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(TS) -I$(SBM) -c -o $(OBJ)/vTCI.o $(TS)/vTCI.cpp
	
$(OBJ)/hashprep.o: $(TS)/hashprep.cpp $(TS)/hashprep.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(CLM) -I$(TH) -I$(TS) -c -o $(OBJ)/hashprep.o $(TS)/hashprep.cpp

#$(OBJ)/hmacsha1.o: $(TS)/hmacsha1.cpp $(TS)/hmacsha1.h
#	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(TS) -I$(SBM) -c -o $(OBJ)/hmacsha1.o $(TS)/hmacsha1.cpp
	
$(LIB)/librptpm-tcsd-g.a: $(lobjs)
	@echo "Building librptpm-tcsd-g.a ..."
	ar cr $(LIB)/librptpm-tcsd-g.a  $(lobjs)

clean:
	rm $(LIB)/librptpm-tcsd-g.a
	rm $(OBJ)/vTCI.o
	rm $(OBJ)/hashprep.o
