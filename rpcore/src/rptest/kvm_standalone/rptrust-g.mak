E=          ../../bin/debug
B=          ../../build/librptrustobjects-g
S=          ../fileProxy/Code/keyNegoServer
SC=         ../fileProxy/Code/commonCode
#FPX=        ../fileProxy/Code/newfileProxy
SCC=        ../fileProxy/Code/jlmcrypto
ACC=        ../fileProxy/Code/accessControl
BSC=        ../fileProxy/Code/jlmbignum
CLM=        ../fileProxy/Code/claims
TH=         ../fileProxy/Code/tao
CH=         ../rpchannel
TPD=        ../fileProxy/Code/TPMDirect
TRS=        ../rpcore
LIB=        ../../lib
CR=	    ../rpcrypto

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall -Werror -Wno-unknown-pragmas -Wno-format -O1
LDFLAGSXML      := ${RELEASE_LDFLAGS}

#-D TEST -DTEST1 -DCRYPTOTEST
CFLAGS=     -D FILECLIENT -D NOKNS -D LINUX -D QUOTE2_DEFINED  -D __FLUSHIO__ $(DEBUG_CFLAGS)
CFLAGS1=    -D FILECLIENT -D LINUX -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(O1DEBUG_CFLAGS)

CC=         g++
LINK=       g++

lobjs=      $(B)/rptrust.o


all: $(LIB)/librptrust-g.a

$(B)/rptrust.o: ./rptrust.cpp ./rptrust.h
	$(CC) $(CFLAGS) -I$(CR)  -I$(CH) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(TRS)  -I$(VLT)  -I$(TPD) -I$(CLM) -I$(TH) -c -o $(B)/rptrust.o ./rptrust.cpp


$(LIB)/librptrust-g.a: $(lobjs)
	@echo "Building librpcrypto.a ..."
	ar cr $(LIB)/librptrust-g.a  $(lobjs) 

clean:
	rm $(B)/*.o
