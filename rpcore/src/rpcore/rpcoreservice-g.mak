RPROOT=../..
BIN=        $(RPROOT)/bin/debug
OBJ=        $(RPROOT)/build/debug/rpcoreserviceobjects
#BIN=        $(RPROOT)/bin/release
#OBJ=        $(RPROOT)/build/release/vmtcServiceobjects
#LIBHOME=    /home/s1
#LIB=        $(LIBHOME)/rp/lib
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc


S=          ../fileProxy/Code/tcService
TH=         ../rpcore/tao
SC=         ../fileProxy/Code/commonCode
SCC=        ../fileProxy/Code/jlmcrypto
SCD=        ../fileProxy/Code/jlmcrypto
SBM=        ../fileProxy/Code/jlmbignum
TS=         ../rptpm/TPMDirect
CH=         ../rpchannel
CHL=        ../fileProxy/Code/channels
VLT=        ../fileProxy/Code/vault
CLM=	    ../fileProxy/Code/claims
TM=	    	../rpcore
FPX=	    ../fileProxy/Code/fileProxy
ACC=        ../fileProxy/Code/accessControl
PROTO=      ../fileProxy/Code/protocolChannel
TAO=        ../rpcore/tao
TPM=        ../rptpm
DM=			../measurer

DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
LDFLAGS          := ${RELEASE_LDFLAGS}
CFLAGS=     -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D TEST1 -D TCSERVICE -D __FLUSHIO__ $(DEBUG_CFLAGS) -DNEWANDREORGANIZED -D TPMTEST -DTEST_SEG	
#CFLAGS=     -D TPMSUPPORT -D TEST -D QUOTE2_DEFINED -D TCSERVICE -D __FLUSHIO__ $(RELEASE_CFLAGS) -DNEWANDREORGANIZED
O1CFLAGS=    -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

sobjs=    	$(OBJ)/rpcoremain.o $(OBJ)/rpconfig.o  $(OBJ)/rpinterface.o \
            $(OBJ)/modtcService.o $(OBJ)/TPMHostsupport.o\
			$(OBJ)/taoSupport.o  $(OBJ)/taoHostServices.o $(OBJ)/taoEnvironment.o\
			$(OBJ)/taoInit.o  $(OBJ)/trustedKeyNego.o \
			$(OBJ)/linuxHostsupport.o 
			
#	    $(OBJ)/dombuilder.o $(OBJ)/tcpchan.o


all: $(BIN)/rpcoresvc

$(BIN)/rpcoresvc: $(sobjs)
	@echo "rpcoreservice"
	$(LINK) -o $(BIN)/rpcoresvc $(sobjs) $(LDFLAGS) -lpthread -L$(LIB) -lrpcrypto-g -lrpmeasurer-g -lrptpm-dir-g -lrpchannel-g 

#$(LINK) -o $(BIN)/rpcoreservice $(sobjs) $(LDFLAGS) -lxenlight -lxlutil -lxenctrl -lxenguest -lblktapctl -lxenstore -luuid -lutil -lpthread -L$(LIB) -lrpdombldr-g


$(OBJ)/rpcoremain.o: $(TM)/rpcoremain.cpp
	$(CC) $(CFLAGS) -I$(S) -I$(DM) -I$(SC) -I$(SCC) -I$(FPX) -I$(VLT) -I$(SBM) -I$(S) -I$(TH) -I$(CLM) -c -o $(OBJ)/rpcoremain.o $(TM)/rpcoremain.cpp

$(OBJ)/rpconfig.o: $(TM)/rpconfig.cpp $(TM)/tcconfig.h
	gcc $(CFLAGS) -I$(SC)  -I$(DM) -I$(SCC) -I$(S) -I$(SBM) -I$(TH) -I$(CH) -c -o $(OBJ)/rpconfig.o $(TM)/rpconfig.cpp

$(OBJ)/rpinterface.o: $(TM)/rpinterface.cpp $(S)/tcIO.h
	$(CC) $(CFLAGS)  -I$(CH)  -I$(S) -I$(SC) -c -o $(OBJ)/rpinterface.o $(TM)/rpinterface.cpp


#original tcService
$(OBJ)/modtcService.o: $(TM)/modtcService.cpp
	$(CC) $(CFLAGS) -I$(S) -I$(DM) -I$(CH)  -I$(SC) -I$(SCC) -I$(FPX) -I$(VLT) -I$(SBM) -I$(S) -I$(TH) -I$(CLM) -c -o $(OBJ)/modtcService.o $(TM)/modtcService.cpp

#implements channel used by taoenv
$(OBJ)/linuxHostsupport.o: $(TH)/linuxHostsupport.cpp $(TH)/linuxHostsupport.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TM) -I$(TS) -I$(CLM)  -I$(CH) -c -o $(OBJ)/linuxHostsupport.o $(TH)/linuxHostsupport.cpp


#CP utility functions
##tao

$(OBJ)/taoHostServices.o: $(TH)/taoHostServices.cpp $(TH)/tao.h 
	$(CC) $(CFLAGS) -I$(TM) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TS) -I$(CLM) -c -o $(OBJ)/taoHostServices.o $(TH)/taoHostServices.cpp


$(OBJ)/taoSupport.o: $(TH)/taoSupport.cpp $(TH)/tao.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TS) -I$(CLM) -c -o $(OBJ)/taoSupport.o $(TH)/taoSupport.cpp


$(OBJ)/taoEnvironment.o: $(TH)/taoEnvironment.cpp $(TH)/tao.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TS) -I$(CLM) -c -o $(OBJ)/taoEnvironment.o $(TH)/taoEnvironment.cpp

#initialization code
$(OBJ)/trustedKeyNego.o: $(TH)/trustedKeyNego.cpp $(TH)/trustedKeyNego.h
	$(CC) $(CFLAGS) -I$(TM) -I$(SC) -I$(SCC) -I$(SBM) -I$(CLM) -I$(CH)  -I$(CHL) -I$(TH) -c -o $(OBJ)/trustedKeyNego.o $(TH)/trustedKeyNego.cpp


$(OBJ)/taoInit.o: $(TH)/taoInit.cpp $(TH)/tao.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TM) -I$(TS) -I$(CLM) -I$(CLM) -I$(CH) -c -o $(OBJ)/taoInit.o $(TH)/taoInit.cpp

$(OBJ)/TPMHostsupport.o: $(TH)/TPMHostsupport.cpp $(TH)/TPMHostsupport.h
	$(CC) $(CFLAGS) -I$(SC) -I$(SCC) -I$(SBM) -I$(TH) -I$(TS) -c -o $(OBJ)/TPMHostsupport.o $(TH)/TPMHostsupport.cpp
