RPROOT=../..
BIN=        $(RPROOT)/bin/debug
OBJ=        $(RPROOT)/build/debug/nontpmrpcorsvcobjects
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

UTIL=       ../util
SC=         ../util/commonCode
SCC=        ../util/jlmcrypto
SBM=        ../util/jlmbignum
CH=         ../rpchannel
TM=	    	../rpcore
DM=			../measurer

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
LDFLAGS          := ${RELEASE_LDFLAGS}
CFLAGS=     -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D TEST1 -D TCSERVICE -D __FLUSHIO__ $(DEBUG_CFLAGS) -DNEWANDREORGANIZED -D TPMTEST -DTEST_SEG -DCSR_REQ
#CFLAGS=     -D TPMSUPPORT -D TEST -D QUOTE2_DEFINED -D TCSERVICE -D __FLUSHIO__ $(RELEASE_CFLAGS) -DNEWANDREORGANIZED
O1CFLAGS=    -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

sobjs=    	$(OBJ)/rpcoremain.o $(OBJ)/rpinterface.o \
            $(OBJ)/modtcService.o #\
			$(OBJ)/taoSupport.o  $(OBJ)/taoHostServices.o $(OBJ)/taoEnvironment.o\
			$(OBJ)/RP_RSAKey-helper.o $(OBJ)/taoInit.o  $(OBJ)/trustedKeyNego.o \
			$(OBJ)/linuxHostsupport.o $(OBJ)/emuTPMHostsupport.o $(OBJ)/hashprep.o
			
#	    $(OBJ)/dombuilder.o $(OBJ)/tcpchan.o


all: $(BIN)/nontpmrpcore

$(BIN)/nontpmrpcore: $(sobjs)
	@echo "rpcoreservice"
	$(LINK) -o $(BIN)/nontpmrpcore $(sobjs) $(LDFLAGS) -lpthread -L$(LIB) -lrpchannel-g -lxml2

#$(LINK) -o $(BIN)/rpcoreservice $(sobjs) $(LDFLAGS) -lxenlight -lxlutil -lxenctrl -lxenguest -lblktapctl -lxenstore -luuid -lutil -lpthread -L$(LIB) -lrpdombldr-g


$(OBJ)/rpcoremain.o: $(TM)/rpcoremain.cpp
	$(CC) $(CFLAGS) -I$(UTIL) -I$(SC) -c -o $(OBJ)/rpcoremain.o $(TM)/rpcoremain.cpp

$(OBJ)/rpconfig.o: $(TM)/rpconfig.cpp $(TM)/tcconfig.h
	gcc $(CFLAGS) -I$(TM) -I$(UTIL) -I$(SC) -I$(CH) -c -o $(OBJ)/rpconfig.o $(TM)/rpconfig.cpp

$(OBJ)/rpinterface.o: $(TM)/rpinterface.cpp $(UTIL)/tcIO.h
	$(CC) $(CFLAGS)  -I$(CH) -I$(UTIL)   -I$(SC) -c -o $(OBJ)/rpinterface.o $(TM)/rpinterface.cpp


#original tcService
$(OBJ)/modtcService.o: $(TM)/modtcService.cpp
	$(CC) $(CFLAGS) -I$(UTIL)  -I/usr/include/libxml2 -I$(CH)  -I$(SC) -c -o $(OBJ)/modtcService.o $(TM)/modtcService.cpp

