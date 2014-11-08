RPROOT=../..
BIN=        $(RPROOT)/bin/debug
OBJ=        $(RPROOT)/build/debug/keyNegoServerobjects
#BIN=        $(RPROOT)/bin/release
#OBJ=        $(RPROOT)/build/release/keyNegoServerobjects
#LIBHOME=    /home/s1
#LIB=        $(LIBHOME)/rp/lib
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

#BIN=         ~/rp/bin/debug
#OBJ=         ~/rp/build/debug/keyNegoServerobjects
#LIB=         ~/rp/lib
#INC=         ../../inc

S=          ../fileProxy/Code/keyNegoServer
SC=         ../fileProxy/Code/commonCode
FPX=        ../fileProxy/Code/fileProxy
SCC=        ../fileProxy/Code/jlmcrypto
ACC=        ../fileProxy/Code/accessControl
BSC=        ../fileProxy/Code/jlmbignum
CLM=        ../fileProxy/Code/claims
TH=         ../rpcore/tao
CH=         ../rpchannel
TPD=        ../rptpm/TPMDirect
VLT=        ../fileProxy/Code/vault
TRS=        ../fileProxy/Code/tcService
        

#-Werror removed
DEBUG_CFLAGS     := -Wall   -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall   -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O1
LDFLAGSXML      := ${RELEASE_LDFLAGS}
CFLAGS=     -D FILECLIENT -D LINUX -D QUOTE2_DEFINED -D TEST -D TEST1 -D TPMKSS -D __FLUSHIO__ $(DEBUG_CFLAGS)
#CFLAGS=     -D FILECLIENT -D LINUX -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(RELEASE_CFLAGS)
CFLAGS1=    -D FILECLIENT -D LINUX -D QUOTE2_DEFINED -D TEST -D TPMKSS -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

dobjs=      $(OBJ)/keyNegoServer.o $(OBJ)/logging.o $(OBJ)/jlmcrypto.o $(OBJ)/jlmUtility.o \
	    $(OBJ)/keys.o $(OBJ)/aesni.o $(OBJ)/sha256.o  $(OBJ)/sha1.o $(OBJ)/channel.o \
            $(OBJ)/hmacsha256.o $(OBJ)/mpBasicArith.o $(OBJ)/mpModArith.o $(OBJ)/hashprep.o \
	    $(OBJ)/mpNumTheory.o  $(OBJ)/cryptoHelper.o $(OBJ)/quote.o $(OBJ)/cert.o  \
	    $(OBJ)/modesandpadding.o $(OBJ)/validateEvidence.o \
	    $(OBJ)/tinystr.o $(OBJ)/tinyxmlerror.o $(OBJ)/tinyxml.o $(OBJ)/tinyxmlparser.o \
	    $(OBJ)/fastArith.o $(OBJ)/encryptedblockIO.o 

all: $(BIN)/kss

$(BIN)/kss: $(dobjs)
	@echo "kss"
	$(LINK) -o $(BIN)/kss $(dobjs) -lpthread

$(OBJ)/keyNegoServer.o: $(S)/keyNegoServer.cpp $(S)/keyNegoServer.h
	$(CC) $(CFLAGS) -I$(S) -I$(TRS) -I$(FPX) -I$(TPD) -I$(VLT) -I$(SC) -I$(TH) -I$(SCC) -I$(BSC) -I$(CH) -I$(CLM) -c -o $(OBJ)/keyNegoServer.o $(S)/keyNegoServer.cpp

$(OBJ)/keys.o: $(SCC)/keys.cpp $(SCC)/keys.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(OBJ)/keys.o $(SCC)/keys.cpp

$(OBJ)/hmacsha256.o: $(SCC)/hmacsha256.cpp $(SCC)/hmacsha256.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(OBJ)/hmacsha256.o $(SCC)/hmacsha256.cpp

$(OBJ)/modesandpadding.o: $(SCC)/modesandpadding.cpp $(SCC)/modesandpadding.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(OBJ)/modesandpadding.o $(SCC)/modesandpadding.cpp

$(OBJ)/claims.o: $(CLM)/claims.cpp $(CLM)/claims.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(TPD) -I$(CLM) -I$(TH) -c -o $(OBJ)/claims.o $(CLM)/claims.cpp

$(OBJ)/validateEvidence.o: $(CLM)/validateEvidence.cpp $(CLM)/validateEvidence.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(TPD) -I$(ACC) -I$(CLM) -I$(TH) -c -o $(OBJ)/validateEvidence.o $(CLM)/validateEvidence.cpp

$(OBJ)/cert.o: $(CLM)/cert.cpp $(CLM)/cert.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(TPD) -I$(CLM) -I$(TH) -c -o $(OBJ)/cert.o $(CLM)/cert.cpp

$(OBJ)/quote.o: $(CLM)/quote.cpp $(CLM)/quote.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(VLT) -I$(FPX) -I$(TPD) -I$(CLM) -I$(TH) -c -o $(OBJ)/quote.o $(CLM)/quote.cpp

$(OBJ)/logging.o: $(SC)/logging.cpp $(SC)/logging.h
	$(CC) $(CFLAGS) -I$(SC) -c -o $(OBJ)/logging.o $(SC)/logging.cpp

$(OBJ)/jlmUtility.o: $(SC)/jlmUtility.cpp $(SC)/jlmUtility.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(OBJ)/jlmUtility.o $(SC)/jlmUtility.cpp

$(OBJ)/jlmcrypto.o: $(SCC)/jlmcrypto.cpp $(SCC)/jlmcrypto.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(OBJ)/jlmcrypto.o $(SCC)/jlmcrypto.cpp

$(OBJ)/cryptoHelper.o: $(SCC)/cryptoHelper.cpp $(SCC)/cryptoHelper.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(OBJ)/cryptoHelper.o $(SCC)/cryptoHelper.cpp

$(OBJ)/tinyxml.o : $(SC)/tinyxml.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(OBJ)/tinyxml.o $(SC)/tinyxml.cpp

$(OBJ)/tinyxmlparser.o : $(SC)/tinyxmlparser.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(OBJ)/tinyxmlparser.o $(SC)/tinyxmlparser.cpp

$(OBJ)/tinyxmlerror.o : $(SC)/tinyxmlerror.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(OBJ)/tinyxmlerror.o $(SC)/tinyxmlerror.cpp

$(OBJ)/tinystr.o : $(SC)/tinystr.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(OBJ)/tinystr.o $(SC)/tinystr.cpp

$(OBJ)/aesni.o: $(SCC)/aesni.cpp $(SCC)/aesni.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SCC) -c -o $(OBJ)/aesni.o $(SCC)/aesni.cpp

$(OBJ)/sha1.o: $(SCC)/sha1.cpp $(SCC)/sha1.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(OBJ)/sha1.o $(SCC)/sha1.cpp

$(OBJ)/sha256.o: $(SCC)/sha256.cpp $(SCC)/sha256.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(OBJ)/sha256.o $(SCC)/sha256.cpp

$(OBJ)/hashprep.o: $(TPD)/hashprep.cpp $(TPD)/hashprep.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(TPD) -c -o $(OBJ)/hashprep.o $(TPD)/hashprep.cpp

$(OBJ)/fastArith.o: $(BSC)/fastArith.cpp
	$(CC) $(O1CFLAGS) -I$(SC) -I$(BSC) -c -o $(OBJ)/fastArith.o $(BSC)/fastArith.cpp

$(OBJ)/mpBasicArith.o: $(BSC)/mpBasicArith.cpp
	$(CC) $(O1CFLAGS) -I$(SC) -I$(BSC) -c -o $(OBJ)/mpBasicArith.o $(BSC)/mpBasicArith.cpp

$(OBJ)/mpModArith.o: $(BSC)/mpModArith.cpp
	$(CC) $(O1CFLAGS) -I$(SC) -I$(BSC) -c -o $(OBJ)/mpModArith.o $(BSC)/mpModArith.cpp

$(OBJ)/mpNumTheory.o: $(BSC)/mpNumTheory.cpp
	$(CC) $(O1CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(OBJ)/mpNumTheory.o $(BSC)/mpNumTheory.cpp

$(OBJ)/channel.o: $(CH)/channel.cpp $(CH)/channel.h
	$(CC) $(CFLAGS) -I$(SC) -I$(CH) -c -o $(OBJ)/channel.o $(CH)/channel.cpp

$(OBJ)/encryptedblockIO.o: $(SCC)/encryptedblockIO.cpp $(SCC)/encryptedblockIO.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(OBJ)/encryptedblockIO.o $(SCC)/encryptedblockIO.cpp


