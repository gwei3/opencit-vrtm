RPROOT	= ../../..
ifeq ($(debug),1)
	DEBUG_CFLAGS    := -Wall  -Wno-format -g -DDEBUG -fpermissive
else
	DEBUG_CFLAGS    := -Wall -Wno-unknown-pragmas -Wno-format -fpermissive -O3
endif

BIN     	= $(RPROOT)/bin
OBJ             = $(RPROOT)/build/vrtmproxyobjects
LIB             = $(RPROOT)/lib

INC		= $(RPROOT)/inc
TM      = ../../vrtmchannel
RPCORE  = ../../vrtmcore/
LOG4CPP=	/usr/include/log4cpp/
SC=         ../../util


#DEBUG_CFLAGS	:= -Wall  -Wno-format -g -DDEBUG -fpermissive
RELEASE_CFLAGS	:= -Wall  -Wno-unknown-pragmas -Wno-format -O3
CFLAGS			= $(DEBUG_CFLAGS)

CC		= g++
LINK	= g++

proxyobj=$(OBJ)/vrtm_proxy.o $(OBJ)/logging.o

all: $(BIN)/vrtm_proxy

$(BIN)/vrtm_proxy: $(proxyobj)
	$(LINK) -o $(BIN)/vrtm_proxy $(proxyobj) -L$(LIB) -L/usr/local/lib/ -lvrtmchannel-g -llog4cpp
ifneq "$(debug)" "1"
	strip -s $(BIN)/vrtm_proxy
endif

$(OBJ)/logging.o: $(SC)/logging.cpp $(SC)/logging.h 
	$(CC) $(CFLAGS) -I$(SC) -I$(LOG4CPP) -c -o $(OBJ)/logging.o $(SC)/logging.cpp

$(OBJ)/vrtm_proxy.o: vrtm_proxy.cpp
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -I. -I$(TM) -I$(RPCORE) -I$(LOG4CPP) -I$(SC) -c -o $(OBJ)/vrtm_proxy.o vrtm_proxy.cpp

.PHONY: clean
clean:
	rm -f $(BIN)/vrtm_proxy
	rm -f $(OBJ)/vrtm_proxy.o



