
RPROOT= ../..
BIN=         $(RPROOT)/bin/release
OBJ=         $(RPROOT)/build/release/rpcryptoobjects
LIB=         $(RPROOT)/lib
INC=         $(RPROOT)/inc

#BIN=         ~/rp/bin/release
#OBJ=         ~/rp/build/release/rpcryptoobjects
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
TM=			../rpcore
CH=         ../rpchannel

DEBUG_CFLAGS     := -Wall -Wno-format -g -DDEBUG 
RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O3 
LDFLAGSXML      := ${RELEASE_LDFLAGS}
#CFLAGS=      -D NOAESNI -D FILECLIENT -DTEST -DTEST1 $(DEBUG_CFLAGS) -DRPNOMAIN -DRPLIB
CFLAGS=      -D NOAESNI $(RELEASE_LDFLAGS) -DRPNOMAIN  -D FILECLIENT -DRPLIB -fPIC
CFLAGS1=     -D NOAESNI -Wall -Wno-unknown-pragmas -Wno-format -O1 -DRPNOMAINa -fPIC

CC=         g++
LINK=       g++

lobjs=  $(OBJ)/logging.o $(OBJ)/jlmcrypto.o $(OBJ)/aes.o \
	    $(OBJ)/sha256.o $(OBJ)/modesandpadding.o $(OBJ)/hmacsha256.o \
	    $(OBJ)/encapsulate.o $(OBJ)/keys.o \
	    $(OBJ)/sha1.o $(OBJ)/hashprep.o \
        $(OBJ)/jlmUtility.o  $(OBJ)/fastArith.o \
	    $(OBJ)/mpBasicArith.o $(OBJ)/mpModArith.o $(OBJ)/mpNumTheory.o \
	    $(OBJ)/fileHash.o $(OBJ)/tinystr.o $(OBJ)/tinyxmlerror.o \
	    $(OBJ)/tinyxml.o $(OBJ)/tinyxmlparser.o \
	    $(OBJ)/rpapi.o $(OBJ)/rpapi.d.o \
	    $(OBJ)/cryptoHelper.o\
	    $(OBJ)/quote.o $(OBJ)/cert.o $(OBJ)/validateEvidence.o $(OBJ)/tcpchan.o

#$(OBJ)/rptrust.o  v: removed because of its dependancy on rpc
#	    $(OBJ)/channelcoding.o \

all: $(LIB)/librpcrypto.a

$(OBJ)/logging.o: $(SC)/logging.cpp $(SC)/logging.h 
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/logging.o $(SC)/logging.cpp

$(OBJ)/jlmUtility.o: $(SC)/jlmUtility.cpp $(SC)/jlmUtility.h 
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/jlmUtility.o $(SC)/jlmUtility.cpp

$(OBJ)/hashprep.o: $(TPM)/hashprep.cpp $(TPM)/hashprep.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(TPM) -c -o $(OBJ)/hashprep.o $(TPM)/hashprep.cpp

$(OBJ)/aes.o: $(SCC)/aes.cpp $(SCC)/aes.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SCC) -c -o $(OBJ)/aes.o $(SCC)/aes.cpp

$(OBJ)/aesni.o: $(SCC)/aesni.cpp $(SCC)/aesni.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -c -o $(OBJ)/aesni.o $(SCC)/aesni.cpp

$(OBJ)/sha1.o: $(SCC)/sha1.cpp $(SCC)/sha1.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/sha1.o $(SCC)/sha1.cpp

$(OBJ)/sha256.o: $(SCC)/sha256.cpp $(SCC)/sha256.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/sha256.o $(SCC)/sha256.cpp

$(OBJ)/hmacsha256.o: $(SCC)/hmacsha256.cpp $(SCC)/hmacsha256.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/hmacsha256.o $(SCC)/hmacsha256.cpp

$(OBJ)/encapsulate.o: $(SCC)/encapsulate.cpp $(SCC)/encapsulate.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/encapsulate.o $(SCC)/encapsulate.cpp

$(OBJ)/fastArith.o: $(SBM)/fastArith.cpp
	$(CC) $(CFLAGS1) -I$(SC) -I$(SBM) -c -o $(OBJ)/fastArith.o $(SBM)/fastArith.cpp

$(OBJ)/mpBasicArith.o: $(SBM)/mpBasicArith.cpp
	$(CC) $(CFLAGS1) -I$(SC) -I$(SBM) -c -o $(OBJ)/mpBasicArith.o $(SBM)/mpBasicArith.cpp

$(OBJ)/mpModArith.o: $(SBM)/mpModArith.cpp
	$(CC) $(CFLAGS) -I$(SC) -I$(SBM) -c -o $(OBJ)/mpModArith.o $(SBM)/mpModArith.cpp

$(OBJ)/mpNumTheory.o: $(SBM)/mpNumTheory.cpp
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/mpNumTheory.o $(SBM)/mpNumTheory.cpp

$(OBJ)/keys.o: $(SCC)/keys.cpp $(SCC)/keys.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/keys.o $(SCC)/keys.cpp

$(OBJ)/cryptoHelper.o: $(SCC)/cryptoHelper.cpp $(SCC)/cryptoHelper.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/cryptoHelper.o $(SCC)/cryptoHelper.cpp

$(OBJ)/jlmcrypto.o: $(SCC)/jlmcrypto.cpp $(SCC)/jlmcrypto.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/jlmcrypto.o $(SCC)/jlmcrypto.cpp

$(OBJ)/modesandpadding.o: $(SCC)/modesandpadding.cpp $(SCC)/modesandpadding.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/modesandpadding.o $(SCC)/modesandpadding.cpp

$(OBJ)/fileHash.o: $(SCC)/fileHash.cpp $(SCC)/fileHash.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -c -o $(OBJ)/fileHash.o $(SCC)/fileHash.cpp

$(OBJ)/tinyxml.o : $(SC)/tinyxml.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(OBJ)/tinyxml.o $(SC)/tinyxml.cpp

$(OBJ)/tinyxmlparser.o : $(SC)/tinyxmlparser.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(OBJ)/tinyxmlparser.o $(SC)/tinyxmlparser.cpp

$(OBJ)/tinyxmlerror.o : $(SC)/tinyxmlerror.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(OBJ)/tinyxmlerror.o $(SC)/tinyxmlerror.cpp

$(OBJ)/tinystr.o : $(SC)/tinystr.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(OBJ)/tinystr.o $(SC)/tinystr.cpp

$(OBJ)/rpapi.o : ./rpapi.cpp ./rpapilocal.h ./rpapilocalext.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(S) -I$(SC)  -I$(SCC) -I$(SBM) -I$(INC) -I$(TPM) -I$(TS) -I$(TM) -I$(CH) -c -o $(OBJ)/rpapi.o ./rpapi.cpp

$(OBJ)/rpapi.d.o : ./rpapi.d.cpp 
	$(CC) $(CFLAGS) $(RELEASECFLAGS)  -c -o $(OBJ)/rpapi.d.o ./rpapi.d.cpp

#referred via librputil
#$(OBJ)/channelcoding.o : $(TM)/channelcoding.cpp $(TM)/channelcoding.h
#	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(S) -I$(SC) -I$(TAO) -I$(SCC) -I$(SBM)  -I$(TPM)   -c -o $(OBJ)/channelcoding.o $(TM)/channelcoding.cpp

#$(OBJ)/rptrust.o : $(TRT)/rptrust.cpp $(TRT)/rptrust.h
#	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I. -I$(CLM) -I$(TS) -I$(SC) -I$(SBM) -I$(SCC)  -I$(TM)  -c -o $(OBJ)/rptrust.o $(TRT)/rptrust.cpp

$(OBJ)/validateEvidence.o: $(CLM)/validateEvidence.cpp $(CLM)/validateEvidence.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TPM) -I$(ACC) -I$(CLM) -I$(TAO) -c -o $(OBJ)/validateEvidence.o $(CLM)/validateEvidence.cpp

$(OBJ)/cert.o: $(CLM)/cert.cpp $(CLM)/cert.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TPM) -I$(CLM) -I$(TAO) -c -o $(OBJ)/cert.o $(CLM)/cert.cpp

$(OBJ)/quote.o: $(CLM)/quote.cpp $(CLM)/quote.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM)   -I$(TPM) -I$(CLM) -I$(TAO) -c -o $(OBJ)/quote.o $(CLM)/quote.cpp

$(OBJ)/tcpchan.o: $(CH)/tcpchan.cpp $(CH)/tcpchan.h
	$(CC) $(CFLAGS) -I$(CH) -c -o $(OBJ)/tcpchan.o $(CH)/tcpchan.cpp
	
$(LIB)/librpcrypto.a: $(lobjs)
	@echo "Building librpcrypto.a ..."
	ar cr $(LIB)/librpcrypto.a  $(lobjs) 


