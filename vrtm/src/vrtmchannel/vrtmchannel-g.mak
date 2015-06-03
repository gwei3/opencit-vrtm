RPROOT=../..

ifeq ($(debug),1)
	DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
else
	DEBUG_CFLAGS     := -Wall -Wno-unknown-pragmas -Wno-format -O3
endif

BIN=        $(RPROOT)/bin
OBJ=        $(RPROOT)/build/vrtmchannelobjects
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

S=          ../util
SC=         ../util/commonCode
SBM=        ../util/jlmbignum
#TS=         ../fileProxy/Code/TPMDirect
TM=	    	../vrtmchannel
#TPM=        ../fileProxy/Code/TPMDirect
PY=	    	/usr/include/python2.7
PYTHON=		python2.7
LXML=		/usr/include/libxml2/
OPENSSL=    /usr/include/openssl/
LOG4CPP=	/usr/local/include/log4cpp/

RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
LDFLAGS          := ${RELEASE_LDFLAGS}
CFLAGS=     -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D TEST1 -D TCSERVICE -D __FLUSHIO__ $(DEBUG_CFLAGS) -fPIC -DNEWANDREORGANIZED -D TPMTEST
O1CFLAGS=    -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

sobjs=     $(OBJ)/channelcoding.o $(OBJ)/pyifc.o $(OBJ)/tcpchan.o $(OBJ)/logging.o $(OBJ)/base64.o $(OBJ)/log_vrtmchannel.o


all: $(LIB)/libvrtmchannel-g.so 

$(OBJ)/logging.o: $(SC)/logging.cpp $(SC)/logging.h 
	$(CC) $(CFLAGS) -I$(SC) -I$(LOG4CPP) -c -o $(OBJ)/logging.o $(SC)/logging.cpp

$(OBJ)/log_vrtmchannel.o: $(TM)/log_vrtmchannel.cpp $(TM)/log_vrtmchannel.h 
	$(CC) $(CFLAGS) -I$(SC) -I$(LOG4CPP) -c -o $(OBJ)/log_vrtmchannel.o $(TM)/log_vrtmchannel.cpp
	
$(OBJ)/channelcoding.o: $(TM)/channelcoding.cpp $(TM)/channelcoding.h
	gcc $(CFLAGS) -I$(TM) -I$(SC) -I$(S) -I$(LOG4CPP) -c -o $(OBJ)/channelcoding.o $(TM)/channelcoding.cpp

$(OBJ)/base64.o: $(SC)/base64.cpp $(SC)/base64.h
	$(CC) $(CFLAGS) -I$(OPENSSL) -I$(LOG4CPP) -c -o $(OBJ)/base64.o $(SC)/base64.cpp

$(OBJ)/pyifc.o: $(TM)/pyifc.cpp
	$(CC) $(CFLAGS) -I$(PY) -I$(SC) -I$(LXML) -I$(LOG4CPP) -c -o $(OBJ)/pyifc.o $(TM)/pyifc.cpp -lxml2
	
$(OBJ)/tcpchan.o: $(TM)/tcpchan.cpp $(TM)/tcpchan.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(TM) -I$(LOG4CPP) -c -o $(OBJ)/tcpchan.o $(TM)/tcpchan.cpp

$(LIB)/libvrtmchannel-g.so: $(sobjs)
	@echo "Building libvrtmchannel-g.so ..."
	$(LINK) -shared  -o  $(LIB)/libvrtmchannel-g.so  $(sobjs)  -L/usr/lib -L/usr/local/lib/ -l$(PYTHON) -lpthread -lxml2 -lssl -lcrypto -llog4cpp
ifneq "$(debug)" "1"
	strip -s $(LIB)/libvrtmchannel-g.so
endif
