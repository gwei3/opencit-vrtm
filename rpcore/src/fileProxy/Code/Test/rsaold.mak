E=          ~/jlmcrypt
B=          $(E)/rsaspeedtestobjects
SC=         ../commonCode
SCC=	    ../jlmcrypto
SBM=	    ../oldjlmbignum

DEBUG_CFLAGS     := -Wall -Wno-format -g -DDEBUG
CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O3 -D NOAESNI -D NOINLINEARITH -D OLD
CFLAGS1   := -Wall -Wno-unknown-pragmas -Wno-format  -D NOAESNI -D NOINLINEARITH -D OLD
LDFLAGSXML      := ${RELEASE_LDFLAGS}

CC=         g++
LINK=       g++

dobjs=  $(B)/rsaspeedtest.o $(B)/keys.o $(B)/jlmcrypto.o \
	$(B)/aes.o $(B)/sha256.o $(B)/mpBasicArith.o  \
	$(B)/modesandpadding.o $(B)/mpModArith.o $(B)/mpNumTheory.o \
	$(B)/tinystr.o $(B)/tinyxmlerror.o $(B)/tinyxml.o \
	$(B)/tinyxmlparser.o $(B)/logging.o $(B)/hmacsha256.o \
 	$(B)/fastArith.o 

tobjs=  $(B)/mpTest.o $(B)/keys.o $(B)/jlmcrypto.o $(B)/logging.o \
	$(B)/aes.o $(B)/sha256.o $(B)/mpBasicArith.o  \
	$(B)/modesandpadding.o $(B)/mpModArith.o $(B)/mpNumTheory.o \
	$(B)/tinystr.o $(B)/tinyxmlerror.o $(B)/tinyxml.o \
	$(B)/tinyxmlparser.o $(B)/hmacsha256.o $(B)/fastArith.o

all: $(E)/rsaoldtest.exe $(E)/mpoldTest.exe

$(E)/mpoldTest.exe: $(tobjs)
	@echo "mpTest"
	$(LINK) -o $(E)/mpoldTest.exe $(tobjs)

$(E)/rsaoldtest.exe: $(dobjs)
	@echo "rsaspeedtest"
	$(LINK) -o $(E)/rsaoldtest.exe $(dobjs)

$(B)/mpTest.o: mpTest.cpp 
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(B)/mpTest.o mpTest.cpp

$(B)/rsaspeedtest.o: rsaspeedtest.cpp $(SCC)/jlmcrypto.h $(SCC)/keys.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(B)/rsaspeedtest.o rsaspeedtest.cpp

$(B)/keys.o: $(SCC)/keys.cpp $(SCC)/keys.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(B)/keys.o $(SCC)/keys.cpp

$(B)/jlmErrors.o: $(SC)/jlmErrors.cpp $(SC)/jlmErrors.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -c -o $(B)/jlmErrors.o $(SC)/jlmErrors.cpp

$(B)/fastArith.o: $(SBM)/fastArith.cpp
	$(CC) $(CFLAGS1) -I$(SC) -I$(SBM) -c -o $(B)/fastArith.o $(SBM)/fastArith.cpp
	$(CC) $(CFLAGS1) -I$(SC) -I$(SBM) -S -o $(B)/fastArith.s $(SBM)/fastArith.cpp

$(B)/mpBasicArith.o: $(SBM)/mpBasicArith.cpp
	$(CC) $(CFLAGS1) -I$(SC) -I$(SBM) -c -o $(B)/mpBasicArith.o $(SBM)/mpBasicArith.cpp

$(B)/mpModArith.o: $(SBM)/mpModArith.cpp
	$(CC) $(CFLAGS) -I$(SC) -I$(SBM) -c -o $(B)/mpModArith.o $(SBM)/mpModArith.cpp

$(B)/mpNumTheory.o: $(SBM)/mpNumTheory.cpp
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(B)/mpNumTheory.o $(SBM)/mpNumTheory.cpp

$(B)/rsaHelper.o: $(SCC)/rsaHelper.cpp $(SCC)/rsaHelper.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(B)/rsaHelper.o $(SCC)/rsaHelper.cpp

$(B)/tinyxml.o : $(SC)/tinyxml.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(B)/tinyxml.o $(SC)/tinyxml.cpp

$(B)/tinyxmlparser.o : $(SC)/tinyxmlparser.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(B)/tinyxmlparser.o $(SC)/tinyxmlparser.cpp

$(B)/tinyxmlerror.o : $(SC)/tinyxmlerror.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(B)/tinyxmlerror.o $(SC)/tinyxmlerror.cpp

$(B)/tinystr.o : $(SC)/tinystr.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(B)/tinystr.o $(SC)/tinystr.cpp

$(B)/aes.o: $(SCC)/aes.cpp $(SCC)/aes.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SCC) -c -o $(B)/aes.o $(SCC)/aes.cpp

$(B)/sha256.o: $(SCC)/sha256.cpp $(SCC)/sha256.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(B)/sha256.o $(SCC)/sha256.cpp

$(B)/jlmcrypto.o: $(SCC)/jlmcrypto.cpp $(SCC)/jlmcrypto.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(B)/jlmcrypto.o $(SCC)/jlmcrypto.cpp

$(B)/modesandpadding.o: $(SCC)/modesandpadding.cpp $(SCC)/modesandpadding.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(B)/modesandpadding.o $(SCC)/modesandpadding.cpp

$(B)/logging.o: $(SC)/logging.cpp 
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(B)/logging.o $(SC)/logging.cpp

$(B)/hmacsha256.o: $(SCC)/hmacsha256.cpp 
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(B)/hmacsha256.o $(SCC)/hmacsha256.cpp
