RPROOT=../..
BIN=        $(RPROOT)/bin/debug
OBJ=        $(RPROOT)/build/debug/nontpmrpcsobjects
#BIN=        $(RPROOT)/bin/release
#OBJ=        $(RPROOT)/build/release/vmtcServiceobjects
#LIBHOME=    /home/s1
#LIB=        $(LIBHOME)/rp/lib
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

#BIN=         ~/rp/bin/debug
#OBJ=         ~/rp/build/debug/vmtcServiceobjects
#LIB=         ~/rp/lib
#INC=         ../../inc

S=          ../fileProxy/Code/tcService
TH=         ../fileProxy/Code/tao
SC=         ../fileProxy/Code/commonCode
SCC=        ../fileProxy/Code/jlmcrypto
SCD=        ../fileProxy/Code/jlmcrypto
SBM=        ../fileProxy/Code/jlmbignum
TS=         ../fileProxy/Code/TPMDirect
CH=         ../fileProxy/Code/channels
CHL=        ../fileProxy/Code/channels
VLT=        ../fileProxy/Code/vault
CLM=	    ../fileProxy/Code/claims
TM=	    ../rpcore
FPX=	    ../fileProxy/Code/fileProxy
ACC=        ../fileProxy/Code/accessControl
PROTO=      ../fileProxy/Code/protocolChannel
TAO=        ../fileProxy/Code/tao
TPM=        ../fileProxy/Code/TPMDirect

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
LDFLAGS          := ${RELEASE_LDFLAGS}
CFLAGS=     -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D TEST1 -D TCSERVICE -D __FLUSHIO__ $(DEBUG_CFLAGS) -DNEWANDREORGANIZED  -D TPMTEST
#CFLAGS=     -D TPMSUPPORT -D TEST -D QUOTE2_DEFINED -D TCSERVICE -D __FLUSHIO__ $(RELEASE_CFLAGS) -DNEWANDREORGANIZED
O1CFLAGS=    -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

sobjs=      $(OBJ)/tcIO.o $(OBJ)/logging.o $(OBJ)/jlmcrypto.o $(OBJ)/jlmUtility.o \
	    $(OBJ)/keys.o $(OBJ)/aesni.o $(OBJ)/sha256.o $(OBJ)/mpBasicArith.o \
	    $(OBJ)/mpModArith.o $(OBJ)/mpNumTheory.o  $(OBJ)/fileHash.o \
	    $(OBJ)/hmacsha256.o $(OBJ)/modesandpadding.o $(OBJ)/buffercoding.o \
	    $(OBJ)/taoSupport.o $(OBJ)/taoEnvironment.o $(OBJ)/taoHostServices.o \
	    $(OBJ)/taoInit.o $(OBJ)/linuxHostsupport.o $(OBJ)/TPMHostsupport.o \
	    $(OBJ)/sha1.o $(OBJ)/tinystr.o $(OBJ)/tinyxmlerror.o \
	    $(OBJ)/tinyxml.o $(OBJ)/tinyxmlparser.o $(OBJ)/vTCIDirect.o \
	    $(OBJ)/tcService.o $(OBJ)/hmacsha1.o \
	    $(OBJ)/channel.o \
             $(OBJ)/trustedKeyNego.o $(OBJ)/hashprep.o \
	    $(OBJ)/encryptedblockIO.o \
	    $(OBJ)/tcconfig.o \
            $(OBJ)/cert.o $(OBJ)/quote.o  \
	    $(OBJ)/cryptoHelper.o \
            $(OBJ)/fastArith.o \
	    $(OBJ)/tcpchan.o
	    #$(OBJ)/dombuilder.o 


all: $(BIN)/nontpmrpcoreservice

$(BIN)/nontpmrpcoreservice: $(sobjs)
	@echo "nontpmrpcoreservice"
	$(LINK) -o $(BIN)/nontpmrpcoreservice $(sobjs) $(LDFLAGS) -L$(LIB) -lrpdombldr-g
#	$(LINK) -o $(BIN)/nontpmrpcoreservice $(sobjs) $(LDFLAGS) -lxenlight -lxlutil -lxenctrl -lxenguest -lblktapctl -lxenstore -luuid -lutil 

# $(LINK) -o $(BIN)/tcService $(sobjs) /lib/x86_64-linux-gnu/libprocps.so.0 -lpthread

$(OBJ)/fileHash.o: $(SCC)/fileHash.cpp $(SCC)/fileHash.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -c -o $(OBJ)/fileHash.o $(SCC)/fileHash.cpp

$(OBJ)/tcIO.o: $(TM)/tcIO.cpp $(S)/tcIO.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -c -o $(OBJ)/tcIO.o $(TM)/tcIO.cpp

$(OBJ)/resource.o: $(FPX)/resource.cpp $(FPX)/resource.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -I$(CLM) -I$(FPX) -c -o $(OBJ)/resource.o $(FPX)/resource.cpp

$(OBJ)/tcService.o: $(TM)/tcService.cpp
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(FPX) -I$(VLT) -I$(SBM) -I$(S) -I$(TH) -I$(CLM) -c -o $(OBJ)/tcService.o $(TM)/tcService.cpp

$(OBJ)/buffercoding.o: $(S)/buffercoding.cpp $(S)/buffercoding.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(TH) -I$(SBM) -I$(S) -c -o $(OBJ)/buffercoding.o $(S)/buffercoding.cpp

$(OBJ)/keys.o: $(SCC)/keys.cpp $(SCC)/keys.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/keys.o $(SCC)/keys.cpp

$(OBJ)/modesandpadding.o: $(SCC)/modesandpadding.cpp $(SCC)/modesandpadding.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/modesandpadding.o $(SCC)/modesandpadding.cpp

$(OBJ)/logging.o: $(SC)/logging.cpp $(SC)/logging.h
	$(CC) $(CFLAGS) -I$(SC) -c -o $(OBJ)/logging.o $(SC)/logging.cpp

$(OBJ)/hmacsha256.o: $(SCC)/hmacsha256.cpp $(SCC)/hmacsha256.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/hmacsha256.o $(SCC)/hmacsha256.cpp

$(OBJ)/taoSupport.o: $(TH)/taoSupport.cpp $(TH)/tao.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TS) -I$(CLM) -c -o $(OBJ)/taoSupport.o $(TH)/taoSupport.cpp

$(OBJ)/taoInit.o: $(TH)/taoInit.cpp $(TH)/tao.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TM)  -I$(TS) -I$(CLM) -c -o $(OBJ)/taoInit.o $(TH)/taoInit.cpp

$(OBJ)/taoEnvironment.o: $(TH)/taoEnvironment.cpp $(TH)/tao.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TS) -I$(CLM) -c -o $(OBJ)/taoEnvironment.o $(TH)/taoEnvironment.cpp

$(OBJ)/taoHostServices.o: $(TH)/taoHostServices.cpp $(TH)/tao.h 
	$(CC) $(CFLAGS) -I$(TM) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TS) -I$(CLM) -c -o $(OBJ)/taoHostServices.o $(TH)/taoHostServices.cpp

$(OBJ)/linuxHostsupport.o: $(TH)/linuxHostsupport.cpp $(TH)/linuxHostsupport.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TM)  -I$(TS) -I$(CLM) -c -o $(OBJ)/linuxHostsupport.o $(TH)/linuxHostsupport.cpp

$(OBJ)/TPMHostsupport.o: $(TM)/TPMHostsupport.cpp $(TH)/TPMHostsupport.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TS) -I$(CLM) -c -o $(OBJ)/TPMHostsupport.o $(TM)/TPMHostsupport.cpp

#$(OBJ)/TPMHostsupport.o: $(TH)/TPMHostsupport.cpp $(TH)/TPMHostsupport.h
	#$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TS) -I$(CLM) -c -o $(OBJ)/TPMHostsupport.o $(TH)/TPMHostsupport.cpp

$(OBJ)/jlmUtility.o: $(SC)/jlmUtility.cpp $(SC)/jlmUtility.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/jlmUtility.o $(SC)/jlmUtility.cpp

$(OBJ)/jlmcrypto.o: $(SCC)/jlmcrypto.cpp $(SCC)/jlmcrypto.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/jlmcrypto.o $(SCC)/jlmcrypto.cpp

$(OBJ)/aesni.o: $(SCC)/aesni.cpp $(SCC)/aesni.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SCC) -c -o $(OBJ)/aesni.o $(SCC)/aesni.cpp

$(OBJ)/sha256.o: $(SCC)/sha256.cpp $(SCC)/sha256.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/sha256.o $(SCC)/sha256.cpp

$(OBJ)/fastArith.o: $(SBM)/fastArith.cpp
	$(CC) $(O1CFLAGS) -I$(SC) -I$(SBM) -c -o $(OBJ)/fastArith.o $(SBM)/fastArith.cpp

$(OBJ)/mpBasicArith.o: $(SBM)/mpBasicArith.cpp
	$(CC) $(O1CFLAGS) -I$(SC) -I$(SBM) -c -o $(OBJ)/mpBasicArith.o $(SBM)/mpBasicArith.cpp

$(OBJ)/mpModArith.o: $(SBM)/mpModArith.cpp
	$(CC) $(O1CFLAGS) -I$(SC) -I$(SBM) -c -o $(OBJ)/mpModArith.o $(SBM)/mpModArith.cpp

$(OBJ)/mpNumTheory.o: $(SBM)/mpNumTheory.cpp
	$(CC) $(O1CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/mpNumTheory.o $(SBM)/mpNumTheory.cpp

$(OBJ)/sha1.o: $(SCC)/sha1.cpp $(SCC)/sha1.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/sha1.o $(SCC)/sha1.cpp

$(OBJ)/vTCIDirect.o: $(TS)/vTCIDirect.cpp $(TS)/vTCIDirect.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(TS) -I$(SBM) -c -o $(OBJ)/vTCIDirect.o $(TS)/vTCIDirect.cpp

$(OBJ)/hmacsha1.o: $(TS)/hmacsha1.cpp $(TS)/hmacsha1.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(TS) -I$(SBM) -c -o $(OBJ)/hmacsha1.o $(TS)/hmacsha1.cpp

$(OBJ)/tinyxml.o : $(SC)/tinyxml.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(OBJ)/tinyxml.o $(SC)/tinyxml.cpp

$(OBJ)/tinyxmlparser.o : $(SC)/tinyxmlparser.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(OBJ)/tinyxmlparser.o $(SC)/tinyxmlparser.cpp

$(OBJ)/tinyxmlerror.o : $(SC)/tinyxmlerror.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(OBJ)/tinyxmlerror.o $(SC)/tinyxmlerror.cpp

$(OBJ)/tinystr.o : $(SC)/tinystr.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(OBJ)/tinystr.o $(SC)/tinystr.cpp

$(OBJ)/hashprep.o: $(TS)/hashprep.cpp $(TS)/hashprep.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(CLM) -I$(TH) -I$(TS) -c -o $(OBJ)/hashprep.o $(TS)/hashprep.cpp

$(OBJ)/trustedKeyNego.o: $(TH)/trustedKeyNego.cpp $(TH)/trustedKeyNego.h
	$(CC) $(CFLAGS) -I$(TM) -I$(SC) -I$(SCC) -I$(SBM) -I$(CLM) -I$(CH) -I$(TH) -c -o $(OBJ)/trustedKeyNego.o $(TH)/trustedKeyNego.cpp

$(OBJ)/channel.o: $(CH)/channel.cpp $(CH)/channel.h
	$(CC) $(CFLAGS) -I$(SC) -I$(CH) -c -o $(OBJ)/channel.o $(CH)/channel.cpp

$(OBJ)/encryptedblockIO.o: $(SCC)/encryptedblockIO.cpp $(SCC)/encryptedblockIO.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -c -o $(OBJ)/encryptedblockIO.o $(SCC)/encryptedblockIO.cpp

$(OBJ)/dombuilder.o: $(TM)/dombuilder.c $(TM)/dombuilder.h
	gcc $(CFLAGS) -I$(TM) -I$(SC) -I$(S) -I$(SBM) -c -o $(OBJ)/dombuilder.o $(TM)/dombuilder.c

$(OBJ)/cryptoHelper.o: $(SCD)/cryptoHelper.cpp $(SCD)/cryptoHelper.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCD) -I$(SBM) -c -o $(OBJ)/cryptoHelper.o $(SCD)/cryptoHelper.cpp

$(OBJ)/tcconfig.o: $(TM)/tcconfig.cpp $(TM)/tcconfig.h
	gcc $(CFLAGS) -I$(SC) -I$(S) -I$(SBM) -c -o $(OBJ)/tcconfig.o $(TM)/tcconfig.cpp

$(OBJ)/cert.o: $(CLM)/cert.cpp $(CLM)/cert.h
	$(CC) $(CFLAGS) -I$(SC) -I$(CLM) -I$(SCD) -I$(ACC) -I$(SBM) -c -o $(OBJ)/cert.o $(CLM)/cert.cpp

$(OBJ)/quote.o: $(CLM)/quote.cpp $(CLM)/quote.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCD) -I$(CLM) -I$(TAO) -I$(TPM) -I$(SBM) -c -o $(OBJ)/quote.o $(CLM)/quote.cpp

$(OBJ)/tcpchan.o: $(TM)/tcpchan.cpp $(TM)/tcpchan.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(TM) -c -o $(OBJ)/tcpchan.o $(TM)/tcpchan.cpp
