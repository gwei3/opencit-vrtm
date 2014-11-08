RPROOT=../..
E=          $(RPROOT)/bin/debug
B=          $(RPROOT)/build/debug/signerutilobjects
S=          ../fileProxy/Code/claims
T=	    	../fileProxy/Code/resources
SC=         ../fileProxy/Code/commonCode
SCC=        ../fileProxy/Code/jlmcrypto
BSC=        ../fileProxy/Code/jlmbignum
TH=	    	../fileProxy/Code/tao
TCS =	    ../fileProxy/Code/tcService
TRS=	    .
TS=	    	../fileProxy/Code/TPMDirect
VLT=	    ../fileProxy/Code/vault
LIB=		$(RPROOT)/lib
INC=	        $(RPROOT)/inc

DEBUG_CFLAGS     := -Wall  -Wno-format -fPIC -g -DDEBUG 
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3 
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1

CFLAGS=     -D LINUX  -DRPNOMAIN -DTESTQUOTE  -D TEST   -D __FLUSHIO__ $(DEBUG_CFLAGS)  

LDFLAGS          := $(RELEASE_LDFLAGS)
O1CFLAGS=    -D LINUX -D TEST -D TIXML_USE_STL -D __FLUSHIO__ $(O1RELEASE_CFLAGS) -DNORPMAIN

CC=         g++
LINK=       g++

dobjs=      $(B)/signerutil.o $(B)/logging.o
	    


all: $(E)/signerutil-g

$(E)/signerutil-g: $(dobjs)
	@echo "qtest"
	$(LINK) -o $(E)/signerutil-g $(dobjs) $(LDFLAGS) -L$(LIB) -lpthread -lrpcrypto-g

$(B)/signerutil.o: $(TRS)/signerutil.cpp 
	$(CC) $(CFLAGS) -I$(TRS) -I$(SCC) -I$(BSC) -I$(SC) -I$(TCS) -I$(INC) -c -o $(B)/signerutil.o $(TRS)/signerutil.cpp

$(B)/logging.o: $(SC)/logging.cpp $(SC)/logging.h
	$(CC) $(CFLAGS) -I$(SC) -c -o $(B)/logging.o $(SC)/logging.cpp



clean:
	rm $(B)/*.o $(E)/tpmsigner
