E=          ../../bin/debug
B=          ../../build/debug/librpquoteobjects
S=          ../fileProxy/Code/keyNegoServer
SC=         ../fileProxy/Code/commonCode
#FPX=        ../fileProxy/Code/newfileProxy
SCC=        ../fileProxy/Code/jlmcrypto
BSC=        ../fileProxy/Code/jlmbignum
CLM=        ../fileProxy/Code/claims
TH=         ../fileProxy/Code/tao
CH=         ../rpchannel
TPD=        ../fileProxy/Code/TPMDirect
TRS=        ../rpcore
LIB=        ../../lib
CR=	    ../rpcrypto
TRT=  		../rptrust

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall -Werror -Wno-unknown-pragmas -Wno-format -O1
LDFLAGSXML      := ${RELEASE_LDFLAGS}

#-D TEST -DTEST1 -DCRYPTOTEST
CFLAGS=     -D FILECLIENT -D NOKNS -D LINUX -D QUOTE2_DEFINED  -D __FLUSHIO__ $(DEBUG_CFLAGS)
CFLAGS1=    -D FILECLIENT -D LINUX -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(O1DEBUG_CFLAGS)

CC=         g++
LINK=       g++

lobjs=      $(B)/rpquote.o


all: $(LIB)/librpquote-g.a

$(B)/rpquote.o: ./rpquote.cpp ./rpquote.h
	$(CC) $(CFLAGS) -I$(CR)  -I$(CH) -I$(S) -I$(SC) -I$(SCC) -I$(BSC) -I$(TRS)  -I$(TRT)  -I$(TPD) -I$(CLM) -I$(TH) -c -o $(B)/rpquote.o ./rpquote.cpp


$(LIB)/librpquote-g.a: $(lobjs)
	@echo "Building librpquote-g.a ..."
	ar cr $(LIB)/librpquote-g.a  $(lobjs) 

clean:
	rm $(B)/*.o
