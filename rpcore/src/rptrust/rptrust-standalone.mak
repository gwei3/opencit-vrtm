E=          ../../bin/release
B=          ../../build/librptrustobjects
S=          ../fileProxy/Code/keyNegoServer
SC=         ../fileProxy/Code/commonCode
#FPX=        ../fileProxy/Code/newfileProxy
SCC=        ../fileProxy/Code/jlmcrypto
ACC=        ../fileProxy/Code/accessControl
BSC=        ../fileProxy/Code/jlmbignum
CLM=        ../fileProxy/Code/claims
TH=         ../fileProxy/Code/tao
CH=         ../fileProxy/Code/channels
TPD=        ../fileProxy/Code/TPMDirect
VLT=        ../fileProxy/Code/vault
TRS=        ../fileProxy/Code/tcService
LIB=        ../../lib


DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG -DTEST
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall -Werror -Wno-unknown-pragmas -Wno-format -O1
LDFLAGSXML      := ${RELEASE_LDFLAGS}

#-D TEST -DTEST1 -DCRYPTOTEST
CFLAGS=     -D FILECLIENT -D NOKNS -D LINUX -D QUOTE2_DEFINED  -D __FLUSHIO__ $(RELEASE_CFLAGS) 
CFLAGS1=    -D FILECLIENT -D LINUX -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++ -g
LINK=       g++ -g

objs=../../build/librptrustobjects/*
robjs=../../build/debug/rpcoreserviceobjects/*

all: rptrust

rptrust: rptrust.o
	$(LINK) $(DEBUG_CFLAGS) -o rptrust rptrust.o $(LDFLAGS) -lpthread -L../../lib/ -lrptrust -lrputil-g -lrpcrypto

rptrust.o: rptrust.cpp rptrust.h
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -I. -I../../src/rptrust/head/ -I$(S) -I../../src/rpcrypto/ -I../../src/rpcore/ -I$(TRS)  -I$(TPD) -I$(VLT) -I$(SC) -I$(TH) -I$(SCC) -I$(BSC) -I$(CH) -I$(CLM) -c rptrust.cpp -o rptrust.o



