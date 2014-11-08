E=          ~/rp/bin/release
B=          ~/rp/build/vmtcserviceobjects
S=          ../tcService
TH=         ../tao
SC=         ../commonCode
SCC=        ../jlmcrypto
BSC=        ../jlmbignum
TS=         ../TPMDirect
CH=         ../channels
VLT=        ../vault
RMM=        ../resources
CLM=	    ../claims
TM=			../../../tao_mods


DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall -Werror -Wno-unknown-pragmas -Wno-format -O1
LDFLAGS          := ${RELEASE_LDFLAGS}

#-D TEST

CFLAGS=     -D TPMSUPPORT -D QUOTE2_DEFINED -DTCSERVICE  -D __FLUSHIO__ $(RELEASE_CFLAGS) -DNEWANDREORGANIZED
O1CFLAGS=    -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(O1RELEASE_CFLAGS)


CC=         g++
LINK=       g++

sobjs=      $(B)/tcIO.o $(B)/logging.o $(B)/jlmcrypto.o $(B)/jlmUtility.o \
	    $(B)/keys.o $(B)/aesni.o $(B)/sha256.o $(B)/mpBasicArith.o \
	    $(B)/mpModArith.o $(B)/mpNumTheory.o $(B)/fastArith.o \
	    $(B)/cryptoHelper.o $(B)/fileHash.o \
	    $(B)/hmacsha256.o $(B)/modesandpadding.o $(B)/buffercoding.o \
	    $(B)/taoSupport.o $(B)/taoEnvironment.o $(B)/taoHostServices.o \
	    $(B)/taoInit.o $(B)/linuxHostsupport.o $(B)/TPMHostsupport.o \
	    $(B)/sha1.o $(B)/tinystr.o $(B)/tinyxmlerror.o \
	    $(B)/tinyxml.o $(B)/tinyxmlparser.o $(B)/vTCIDirect.o  \
	    $(B)/tcService.o $(B)/hmacsha1.o $(B)/cert.o  $(B)/quote.o \
	    $(B)/channel.o  $(B)/trustedKeyNego.o $(B)/hashprep.o \
	    $(B)/encryptedblockIO.o  $(B)/dombuilder.o $(B)/tcconfig.o

#$(B)/resource.o $(B)/vault.o $(B)/secPrincipal.o $(B)/claims.o 

all: $(E)/rpcoreservice.exe

$(E)/rpcoreservice.exe: $(sobjs)
	@echo "tcService"
	$(LINK) -o $(E)/rpcoreservice.exe $(sobjs) $(LDFLAGS) -lpthread -lxenlight -lxlutil -lxenctrl -lxenguest -lblktapctl -lxenstore  -luuid -lutil 

# $(LINK) -o $(E)/tcService.exe $(sobjs) /lib/x86_64-linux-gnu/libprocps.so.0 -lpthread


$(B)/tcService.o: $(TM)/tcService.cpp
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(RMM) -I$(VLT) -I$(BSC) -I$(S) -I$(TH) -I$(CLM) -c -o $(B)/tcService.o $(TM)/tcService.cpp

$(B)/fileHash.o: $(SCC)/fileHash.cpp $(SCC)/fileHash.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -c -o $(B)/fileHash.o $(SCC)/fileHash.cpp

#$(B)/tcIO.o: $(S)/tcIO.cpp $(S)/tcIO.h
#	$(CC) $(CFLAGS) -I$(S) -I$(SC) -c -o $(B)/tcIO.o $(S)/tcIO.cpp
$(B)/tcIO.o: $(TM)/tcIO.cpp $(S)/tcIO.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -c -o $(B)/tcIO.o $(TM)/tcIO.cpp

$(B)/resource.o: $(RMM)/resource.cpp $(RMM)/resource.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -I$(CLM) -I$(RMM)  -c -o $(B)/resource.o $(RMM)/resource.cpp

$(B)/vault.o: $(VLT)/vault.cpp $(VLT)/vault.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -I$(CH) -I$(RMM) -I$(CLM) -I$(S) -I$(TH) -I$(VLT) -c -o $(B)/vault.o $(VLT)/vault.cpp


#$(B)/tcService.o: $(S)/tcService.cpp
#	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(RMM) -I$(VLT) -I$(BSC) -I$(S) -I$(TH) -I$(CLM) -c -o $(B)/tcService.o $(S)/tcService.cpp

#$(B)/secPrincipal.o: $(CLM)/secPrincipal.cpp $(CLM)/secPrincipal.h
#	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(VLT) -I$(TH) -I$(CLM) -I$(RMM)  -c -o $(B)/secPrincipal.o $(CLM)/secPrincipal.cpp

$(B)/cert.o: $(CLM)/cert.cpp $(CLM)/cert.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(VLT) -I$(TH) -I$(CLM) -I$(RMM)  -c -o $(B)/cert.o $(CLM)/cert.cpp

$(B)/quote.o: $(CLM)/quote.cpp $(CLM)/quote.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(CLM) -I$(TH) -I$(TS) -c -o $(B)/quote.o $(CLM)/quote.cpp

$(B)/buffercoding.o: $(S)/buffercoding.cpp $(S)/buffercoding.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(TH) -I$(BSC) -I$(S) -c -o $(B)/buffercoding.o $(S)/buffercoding.cpp
	

$(B)/keys.o: $(SCC)/keys.cpp $(SCC)/keys.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(B)/keys.o $(SCC)/keys.cpp

$(B)/modesandpadding.o: $(SCC)/modesandpadding.cpp $(SCC)/modesandpadding.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(B)/modesandpadding.o $(SCC)/modesandpadding.cpp

$(B)/logging.o: $(SC)/logging.cpp $(SC)/logging.h
	$(CC) $(CFLAGS) -I$(SC) -c -o $(B)/logging.o $(SC)/logging.cpp

$(B)/hmacsha256.o: $(SCC)/hmacsha256.cpp $(SCC)/hmacsha256.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(B)/hmacsha256.o $(SCC)/hmacsha256.cpp

$(B)/taoSupport.o: $(TH)/taoSupport.cpp $(TH)/tao.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(TH) -I$(TS) -I$(CLM) -c -o $(B)/taoSupport.o $(TH)/taoSupport.cpp

$(B)/taoInit.o: $(TH)/taoInit.cpp $(TH)/tao.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(TH) -I$(TS) -I$(CLM) -c -o $(B)/taoInit.o $(TH)/taoInit.cpp

$(B)/taoEnvironment.o: $(TH)/taoEnvironment.cpp $(TH)/tao.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(TH) -I$(TS) -I$(CLM) -c -o $(B)/taoEnvironment.o $(TH)/taoEnvironment.cpp

$(B)/taoHostServices.o: $(TH)/taoHostServices.cpp $(TH)/tao.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(TH) -I$(TS) -I$(CLM) -c -o $(B)/taoHostServices.o $(TH)/taoHostServices.cpp

$(B)/linuxHostsupport.o: $(TH)/linuxHostsupport.cpp $(TH)/linuxHostsupport.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(TH) -I$(TS) -I$(CLM) -c -o $(B)/linuxHostsupport.o $(TH)/linuxHostsupport.cpp

#$(B)/TPMHostsupport.o: $(TH)/TPMHostsupport.cpp $(TH)/TPMHostsupport.h
#	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(TH) -I$(TS) -I$(CLM) -c -o $(B)/TPMHostsupport.o $(TH)/TPMHostsupport.cpp

$(B)/TPMHostsupport.o: $(TM)/TPMHostsupport.cpp $(TH)/TPMHostsupport.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(TH) -I$(TS) -I$(CLM) -c -o $(B)/TPMHostsupport.o $(TM)/TPMHostsupport.cpp

$(B)/jlmUtility.o: $(SC)/jlmUtility.cpp $(SC)/jlmUtility.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(B)/jlmUtility.o $(SC)/jlmUtility.cpp

$(B)/jlmcrypto.o: $(SCC)/jlmcrypto.cpp $(SCC)/jlmcrypto.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(B)/jlmcrypto.o $(SCC)/jlmcrypto.cpp

$(B)/cryptoHelper.o: $(SCC)/cryptoHelper.cpp $(SCC)/cryptoHelper.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(B)/cryptoHelper.o $(SCC)/cryptoHelper.cpp

$(B)/aesni.o: $(SCC)/aesni.cpp $(SCC)/aesni.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SCC) -c -o $(B)/aesni.o $(SCC)/aesni.cpp

$(B)/sha256.o: $(SCC)/sha256.cpp $(SCC)/sha256.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(B)/sha256.o $(SCC)/sha256.cpp

$(B)/fastArith.o: $(BSC)/fastArith.cpp
	$(CC) $(O1CFLAGS) -I$(SC) -I$(BSC) -c -o $(B)/fastArith.o $(BSC)/fastArith.cpp

$(B)/mpBasicArith.o: $(BSC)/mpBasicArith.cpp
	$(CC) $(O1CFLAGS) -I$(SC) -I$(BSC) -c -o $(B)/mpBasicArith.o $(BSC)/mpBasicArith.cpp

$(B)/mpModArith.o: $(BSC)/mpModArith.cpp
	$(CC) $(O1CFLAGS) -I$(SC) -I$(BSC) -c -o $(B)/mpModArith.o $(BSC)/mpModArith.cpp

$(B)/mpNumTheory.o: $(BSC)/mpNumTheory.cpp
	$(CC) $(O1CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(B)/mpNumTheory.o $(BSC)/mpNumTheory.cpp

$(B)/sha1.o: $(SCC)/sha1.cpp $(SCC)/sha1.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(B)/sha1.o $(SCC)/sha1.cpp

$(B)/vTCIDirect.o: $(TS)/vTCIDirect.cpp $(TS)/vTCIDirect.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(TS) -I$(BSC) -c -o $(B)/vTCIDirect.o $(TS)/vTCIDirect.cpp

$(B)/hmacsha1.o: $(TS)/hmacsha1.cpp $(TS)/hmacsha1.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(TS) -I$(BSC) -c -o $(B)/hmacsha1.o $(TS)/hmacsha1.cpp

$(B)/tinyxml.o : $(SC)/tinyxml.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(B)/tinyxml.o $(SC)/tinyxml.cpp

$(B)/tinyxmlparser.o : $(SC)/tinyxmlparser.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(B)/tinyxmlparser.o $(SC)/tinyxmlparser.cpp

$(B)/tinyxmlerror.o : $(SC)/tinyxmlerror.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(B)/tinyxmlerror.o $(SC)/tinyxmlerror.cpp

$(B)/tinystr.o : $(SC)/tinystr.cpp $(SC)/tinyxml.h $(SC)/tinystr.h
	$(CC) $(CFLAGS) $(RELEASECFLAGS) -I$(SC) -c -o $(B)/tinystr.o $(SC)/tinystr.cpp

$(B)/hashprep.o: $(TS)/hashprep.cpp $(TS)/hashprep.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(CLM) -I$(TH) -I$(TS) -c -o $(B)/hashprep.o $(TS)/hashprep.cpp

$(B)/trustedKeyNego.o: $(TH)/trustedKeyNego.cpp $(TH)/trustedKeyNego.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -I$(CLM) -I$(CH) -I$(TH) -c -o $(B)/trustedKeyNego.o $(TH)/trustedKeyNego.cpp

$(B)/channel.o: $(CH)/channel.cpp $(CH)/channel.h
	$(CC) $(CFLAGS) -I$(SC) -I$(CH) -c -o $(B)/channel.o $(CH)/channel.cpp

$(B)/encryptedblockIO.o: $(SCC)/encryptedblockIO.cpp $(SCC)/encryptedblockIO.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(BSC) -c -o $(B)/encryptedblockIO.o $(SCC)/encryptedblockIO.cpp

$(B)/tcconfig.o: $(TM)/tcconfig.cpp $(TM)/tcconfig.h
	$(CC) $(CFLAGS) -I$(TM) -I$(SC) -I$(S) -c -o $(B)/tcconfig.o $(TM)/tcconfig.cpp
	
$(B)/dombuilder.o: $(TM)/dombuilder.c $(TM)/dombuilder.h
	gcc $(CFLAGS) -I$(TM) -I$(SC) -I$(S)  -c -o $(B)/dombuilder.o $(TM)/dombuilder.c

	
clean:
	rm $(B)/*.o
