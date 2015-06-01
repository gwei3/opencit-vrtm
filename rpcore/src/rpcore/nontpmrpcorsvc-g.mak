RPROOT=../..

ifeq ($(debug),1)
	DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
else
        DEBUG_CFLAGS     := -Wall -Wno-unknown-pragmas -Wno-format -O3
endif

BIN=        $(RPROOT)/bin
OBJ=        $(RPROOT)/build/nontpmrpcorsvcobjects
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

UTIL=       ../util
SC=         ../util/commonCode
SCC=        ../util/jlmcrypto
SBM=        ../util/jlmbignum
CH=         ../rpchannel
TM=	    	../rpcore
DM=			../measurer
LOG4CPP=	/usr/local/include/log4cpp/

#DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
LDFLAGS          := ${RELEASE_LDFLAGS}
CFLAGS=     -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D TEST1 -D TCSERVICE -D __FLUSHIO__ $(DEBUG_CFLAGS) -DNEWANDREORGANIZED -D TPMTEST -DTEST_SEG -DCSR_REQ
#CFLAGS=     -D TPMSUPPORT -D TEST -D QUOTE2_DEFINED -D TCSERVICE -D __FLUSHIO__ $(RELEASE_CFLAGS) -DNEWANDREORGANIZED
O1CFLAGS=    -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

sobjs=    	$(OBJ)/rpcoremain.o $(OBJ)/rpinterface.o \
            $(OBJ)/modtcService.o $(OBJ)/logging.o
			
#	    $(OBJ)/dombuilder.o $(OBJ)/tcpchan.o


all: $(BIN)/nontpmrpcore

$(BIN)/nontpmrpcore: $(sobjs)
	@echo "rpcoreservice"
	$(LINK) -o $(BIN)/nontpmrpcore $(sobjs) $(LDFLAGS) -L/usr/local/lib/ -lpthread -L$(LIB) -lrpchannel-g -lxml2 -llog4cpp
ifneq "$(debug)" "1"
	strip -s $(BIN)/nontpmrpcore
endif

#$(LINK) -o $(BIN)/rpcoreservice $(sobjs) $(LDFLAGS) -lxenlight -lxlutil -lxenctrl -lxenguest -lblktapctl -lxenstore -luuid -lutil -lpthread -L$(LIB) -lrpdombldr-g

$(OBJ)/logging.o: $(SC)/logging.cpp $(SC)/logging.h 
	$(CC) $(CFLAGS) -I$(SC) -I$(LOG4CPP) -c -o $(OBJ)/logging.o $(SC)/logging.cpp

$(OBJ)/rpcoremain.o: $(TM)/rpcoremain.cpp
	@echo $(debug)
	$(CC) $(CFLAGS) -I$(UTIL) -I$(SC) -I$(LOG4CPP) -I$(CH) -c -o $(OBJ)/rpcoremain.o $(TM)/rpcoremain.cpp

$(OBJ)/rpinterface.o: $(TM)/rpinterface.cpp $(UTIL)/tcIO.h
	$(CC) $(CFLAGS)  -I$(CH) -I$(UTIL)   -I$(SC) -I$(LOG4CPP) -c -o $(OBJ)/rpinterface.o $(TM)/rpinterface.cpp


#original tcService
$(OBJ)/modtcService.o: $(TM)/modtcService.cpp
	$(CC) $(CFLAGS) -I$(UTIL)  -I/usr/include/libxml2 -I$(CH)  -I$(SC) -I$(LOG4CPP) -c -o $(OBJ)/modtcService.o $(TM)/modtcService.cpp

