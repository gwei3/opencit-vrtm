RPROOT=../..
BIN=        $(RPROOT)/bin/debug
OBJ=        $(RPROOT)/build/debug/rpchannelobjects
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

S=          ../util
SC=         ../util/commonCode
SBM=        ../util/jlmbignum
#TS=         ../fileProxy/Code/TPMDirect
TM=	    	../rpchannel
#TPM=        ../fileProxy/Code/TPMDirect
PY=	    	/usr/include/python2.7
PYTHON=		python2.7
LXML=		/usr/include/libxml2/

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
LDFLAGS          := ${RELEASE_LDFLAGS}
CFLAGS=     -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D TEST1 -D TCSERVICE -D __FLUSHIO__ $(DEBUG_CFLAGS) -fPIC -DNEWANDREORGANIZED -D TPMTEST
O1CFLAGS=    -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

sobjs=     $(OBJ)/channelcoding.o $(OBJ)/pyifc.o $(OBJ)/tcpchan.o $(OBJ)/logging.o


all: $(LIB)/librpchannel-g.so 

$(OBJ)/logging.o: $(SC)/logging.cpp $(SC)/logging.h 
	$(CC) $(CFLAGS) -I$(SC) -c -o $(OBJ)/logging.o $(SC)/logging.cpp
	
$(OBJ)/channelcoding.o: $(TM)/channelcoding.cpp $(TM)/channelcoding.h
	gcc $(CFLAGS) -I$(TM) -I$(SC) -I$(S) -c -o $(OBJ)/channelcoding.o $(TM)/channelcoding.cpp
		
$(OBJ)/pyifc.o: $(TM)/pyifc.cpp
	$(CC) $(CFLAGS) -I$(PY) -I$(LXML) -c -o $(OBJ)/pyifc.o $(TM)/pyifc.cpp -lxml2
	
$(OBJ)/tcpchan.o: $(TM)/tcpchan.cpp $(TM)/tcpchan.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(TM) -c -o $(OBJ)/tcpchan.o $(TM)/tcpchan.cpp

$(LIB)/librpchannel-g.so: $(sobjs)
	@echo "Building librpchannel-g.so ..."
	$(LINK) -shared  -o  $(LIB)/librpchannel-g.so  $(sobjs)  -L/usr/lib -l$(PYTHON) -lpthread -lxml2

