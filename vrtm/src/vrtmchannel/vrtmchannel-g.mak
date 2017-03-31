RPROOT=../..

ifeq ($(debug),1)
	DEBUG_CFLAGS     := -Wall  -Wno-format -g -DDEBUG
else
	DEBUG_CFLAGS     := -Wall -Wno-unknown-pragmas -Wno-format -O3 -Wformat -Wformat-security
endif

BIN=        $(RPROOT)/bin
OBJ=        $(RPROOT)/build/vrtmchannelobjects
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

S=          ../util
TM=	    	../vrtmchannel
LXML=		/usr/include/libxml2/
OPENSSL=    /usr/include/openssl/
LOG4CPP=	/usr/include/log4cpp/
SAFESTRING=	../util/SafeStringLibrary/
SAFESTRING_INCLUDE=    $(SAFESTRING)/include/

#RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O3
O1RELEASE_CFLAGS   := -Wall  -Wno-unknown-pragmas -Wno-format -O1
RELEASE_LDFLAGS  :=  -z noexecstack -z relro -z now
LDFLAGS          := ${RELEASE_LDFLAGS}
CFLAGS=      -fPIC -fstack-protector-strong -O2 -D FORTIFY_SOURCE=2 -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D TEST1 -D TCSERVICE -D __FLUSHIO__ $(DEBUG_CFLAGS) -DNEWANDREORGANIZED -D TPMTEST
O1CFLAGS=    -D TPMSUPPORT -D QUOTE2_DEFINED -D TEST -D __FLUSHIO__ $(O1RELEASE_CFLAGS)

CC=         g++
LINK=       g++

sobjs=     $(OBJ)/channelcoding.o $(OBJ)/parser.o $(OBJ)/tcpchan.o $(OBJ)/logging.o $(OBJ)/base64.o $(OBJ)/log_vrtmchannel.o $(OBJ)/xpathparser.o $(OBJ)/vrtmsockets.o $(OBJ)/loadconfig.o $(OBJ)/vrtmCommon.o

all: $(LIB)/libvrtmchannel.so 

$(OBJ)/vrtmCommon.o: $(S)/vrtmCommon.cpp $(S)/vrtmCommon.h
	$(CC) $(CFLAGS) -I$(S) -I$(LOG4CPP) -c -o $(OBJ)/vrtmCommon.o $(S)/vrtmCommon.cpp

$(OBJ)/loadconfig.o: $(S)/loadconfig.cpp $(S)/loadconfig.h
	$(CC) $(CFLAGS) -I$(S) -I$(LOG4CPP) -I$(SAFESTRING_INCLUDE) -c -o $(OBJ)/loadconfig.o $(S)/loadconfig.cpp

$(OBJ)/vrtmsockets.o: $(S)/vrtmsockets.cpp $(S)/vrtmsockets.h
	$(CC) $(CFLAGS) -I$(S) -c -o $(OBJ)/vrtmsockets.o $(S)/vrtmsockets.cpp

$(OBJ)/logging.o: $(S)/logging.cpp $(S)/logging.h 
	$(CC) $(CFLAGS) -I$(S) -I$(LOG4CPP) -c -o $(OBJ)/logging.o $(S)/logging.cpp

$(OBJ)/log_vrtmchannel.o: $(TM)/log_vrtmchannel.cpp $(TM)/log_vrtmchannel.h 
	$(CC) $(CFLAGS) -I$(LOG4CPP) -I$(S) -I$(SAFESTRING_INCLUDE) -c -o $(OBJ)/log_vrtmchannel.o $(TM)/log_vrtmchannel.cpp
	
$(OBJ)/channelcoding.o: $(TM)/channelcoding.cpp $(TM)/channelcoding.h
	$(CC) $(CFLAGS) -I$(TM) -I$(S) -I$(LOG4CPP) -I$(SAFESTRING_INCLUDE) -c -o $(OBJ)/channelcoding.o $(TM)/channelcoding.cpp

$(OBJ)/base64.o: $(S)/base64.cpp $(S)/base64.h
	$(CC) $(CFLAGS) -I$(OPENSSL) -I$(LOG4CPP) -I$(SAFESTRING_INCLUDE) -c -o $(OBJ)/base64.o $(S)/base64.cpp

$(OBJ)/parser.o: $(TM)/parser.cpp
	$(CC) $(CFLAGS) -I$(S) -I$(LXML) -I$(LOG4CPP) -I$(SAFESTRING_INCLUDE) -c -o $(OBJ)/parser.o $(TM)/parser.cpp
	
$(OBJ)/tcpchan.o: $(TM)/tcpchan.cpp $(TM)/tcpchan.h
	$(CC) $(CFLAGS) -I$(S) -I$(TM) -I$(LOG4CPP) -I$(SAFESTRING_INCLUDE) -c -o $(OBJ)/tcpchan.o $(TM)/tcpchan.cpp

$(OBJ)/xpathparser.o: $(TM)/xpathparser.cpp $(TM)/xpathparser.h
	$(CC) $(CFLAGS) -I$(S) -I$(LXML) -I$(LOG4CPP) -I$(SAFESTRING_INCLUDE) -c -o $(OBJ)/xpathparser.o $(TM)/xpathparser.cpp

$(OBJ)/loadconfig.o: $(S)/loadconfig.cpp $(S)/loadconfig.h
	$(CC) $(CFLAGS) -I$(S) -I$(LOG4CPP) -I$(SAFESTRING_INCLUDE) -c -o $(OBJ)/loadconfig.o $(S)/loadconfig.cpp
	
	
$(LIB)/libvrtmchannel.so: $(sobjs)
	@echo "Building libvrtmchannel.so ..."
	$(LINK) -shared  -o  $(LIB)/libvrtmchannel.so  $(sobjs) $(LDFLAGS) -L$(SAFESTRING) -L/usr/lib -L/usr/local/lib/ -lpthread -lxml2 -llog4cpp -lssl -lcrypto -lSafeStringRelease
ifneq "$(debug)" "1"
	strip -s $(LIB)/libvrtmchannel.so
endif
