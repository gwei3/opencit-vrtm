RPROOT=../..
BIN=        $(RPROOT)/bin/release
OBJ=        $(RPROOT)/build/release/dombldrobjects
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

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
TM=	    	../rpchannel
FPX=	    ../fileProxy/Code/fileProxy
ACC=        ../fileProxy/Code/accessControl
PROTO=      ../fileProxy/Code/protocolChannel
TAO=        ../fileProxy/Code/tao
TPM=        ../fileProxy/Code/TPMDirect
PY=	    	/usr/include/python2.7
LXML=		/usr/include/libxml2/

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
LDFLAGS          := ${RELEASE_LDFLAGS}
#CFLAGS=     -D TPMSUPPORT -D QUOTE2_DEFINED -D CRYPTOTEST4 -D TEST -D TCSERVICE -D __FLUSHIO__ $(DEBUG_CFLAGS)
CFLAGS=     -D TPMSUPPORT -D QUOTE2_DEFINED -D TCSERVICE -D __FLUSHIO__ $(RELEASE_CFLAGS) -DNEWANDREORGANIZED -fPIC
O1CFLAGS=    -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

sobjs=   $(OBJ)/channelcoding.o $(OBJ)/pyifc.o


all: $(LIB)/librpchannel.so 


$(OBJ)/channelcoding.o: $(TM)/channelcoding.cpp $(TM)/channelcoding.h
	gcc $(CFLAGS) -I$(TM) -I$(SC) -I$(S) -I$(SBM) -c -o $(OBJ)/channelcoding.o $(TM)/channelcoding.cpp
	
$(OBJ)/pyifc.o: $(TM)/pyifc.cpp
	$(CC) $(CFLAGS) -I$(PY) -I$(LXML) -c -o $(OBJ)/pyifc.o $(TM)/pyifc.cpp -lxml2

$(LIB)/librpchannel.so: $(sobjs)
	@echo "Building librpchannel.so ..."
	$(LINK) -shared  -o  $(LIB)/librpchannel.so  $(sobjs)  -L/usr/lib -lpython2.7 -lpthread -lxml2
