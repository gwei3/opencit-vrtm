RPROOT=../..
BIN=        $(RPROOT)/bin/release
OBJ=        $(RPROOT)/build/release/vTCIobjects
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

SC=         ../fileProxy/Code/commonCode
BN=         ../fileProxy/Code/jlmbignum
CR=         ../fileProxy/Code/jlmcrypto
TPU=	    ../fileProxy/Code/TPMUser
#S=          .
S=	    ../fileProxy/Code/TPMUser
# CFLAGS=     -D UNIXRANDBITS -D TPMTEST

DEBUG_CFLAGS     := -Wall -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O3
CFLAGS=     -D UNIXRANDBITS -D TPMTEST -D QUOTE2_DEFINED -D PCR18
LDFLAGSXML      := ${RELEASE_LDFLAGS}

CC=         g++
LINK=       g++

dobjs=      $(OBJ)/vTCI.o 

#$(OBJ)/sha1.o $(OBJ)/mpBasicArith.o $(OBJ)/mpModArith.o \
	    $(OBJ)/mpNumTheory.o $(OBJ)/mpRand.o $(OBJ)/jlmUtility.o $(OBJ)/tinyxml.o \
	    $(OBJ)/tinyxmlparser.o $(OBJ)/tinyxmlerror.o $(OBJ)/tinystr.o \
	    $(OBJ)/hashprep.o $(OBJ)/sha256.o $(OBJ)/logging.o \
	    $(OBJ)/fastArith.o 

all: $(BIN)/vTCI

$(BIN)/vTCI: $(dobjs)
	@echo "vTCI"
	$(LINK) -o $(BIN)/vTCI $(dobjs) -ltspi

$(OBJ)/sha1.o: $(CR)/sha1.cpp $(CR)/sha1.h
	$(CC) $(CFLAGS) -I$(CR) -I$(SC) -I/usr/include/tss -c -o $(OBJ)/sha1.o $(CR)/sha1.cpp

$(OBJ)/vTCI.o: $(S)/vTCI.cpp $(S)/vTCI.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(CR) -I$(TPU) -I$(BN) -I/usr/include/tss -c -o $(OBJ)/vTCI.o $(S)/vTCI.cpp

$(OBJ)/mpBasicArith.o: $(BN)/mpBasicArith.cpp
	$(CC) $(CFLAGS) -I$(SC) -I$(BN) -c -o $(OBJ)/mpBasicArith.o $(BN)/mpBasicArith.cpp

$(OBJ)/mpModArith.o: $(BN)/mpModArith.cpp
	$(CC) $(CFLAGS) -I$(SC) -I$(BN) -c -o $(OBJ)/mpModArith.o $(BN)/mpModArith.cpp

$(OBJ)/mpNumTheory.o: $(BN)/mpNumTheory.cpp
	$(CC) $(CFLAGS) -I$(SC) -I$(CR) -I$(BN) -c -o $(OBJ)/mpNumTheory.o $(BN)/mpNumTheory.cpp

$(OBJ)/mpRand.o: $(BN)/mpRand.cpp
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BN) -c -o $(OBJ)/mpRand.o $(BN)/mpRand.cpp

$(OBJ)/jlmUtility.o: $(SC)/jlmUtility.cpp
	$(CC) $(CFLAGS) -I$(SC) -I$(CR) -I$(BN) -c -o $(OBJ)/jlmUtility.o $(SC)/jlmUtility.cpp

$(OBJ)/tinyxml.o : $(SC)/tinyxml.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) -I$(SC) -c -o $(OBJ)/tinyxml.o $(SC)/tinyxml.cpp

$(OBJ)/tinyxmlparser.o : $(SC)/tinyxmlparser.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) -I$(SC) -c -o $(OBJ)/tinyxmlparser.o $(SC)/tinyxmlparser.cpp

$(OBJ)/tinyxmlerror.o : $(SC)/tinyxmlerror.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) -I$(SC) -c -o $(OBJ)/tinyxmlerror.o $(SC)/tinyxmlerror.cpp

$(OBJ)/tinystr.o : $(SC)/tinystr.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) -I$(SC) -c -o $(OBJ)/tinystr.o $(SC)/tinystr.cpp

$(OBJ)/sha256.o: $(CR)/sha256.cpp $(CR)/sha256.h
	$(CC) $(CFLAGS) -I$(SC) -I$(CR) -c -o $(OBJ)/sha256.o $(CR)/sha256.cpp

$(OBJ)/hashprep.o: $(TPU)/hashprep.cpp $(TPU)/hashprep.h
	$(CC) $(CFLAGS) -D TEST -I$(SC) -I$(TPU) -I$(CR) -c -o $(OBJ)/hashprep.o $(TPU)/hashprep.cpp

$(OBJ)/logging.o: $(SC)/logging.cpp $(SC)/logging.h
	$(CC) $(CFLAGS) -I$(SC) -c -o $(OBJ)/logging.o $(SC)/logging.cpp

$(OBJ)/fastArith.o: $(BN)/fastArith.cpp
	$(CC) $(CFLAGS) -I$(SC) -I$(BN) -c -o $(OBJ)/fastArith.o $(BN)/fastArith.cpp

