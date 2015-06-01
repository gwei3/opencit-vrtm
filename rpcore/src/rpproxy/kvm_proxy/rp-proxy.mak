RPROOT	= ../../..
ifeq ($(debug),1)
	DEBUG_CFLAGS    := -Wall  -Wno-format -g -DDEBUG -fpermissive
else
	DEBUG_CFLAGS    := -Wall -Wno-unknown-pragmas -Wno-format -fpermissive -O3
endif

BIN     	= $(RPROOT)/bin
OBJ             = $(RPROOT)/build/rpproxyobjects
LIB             = $(RPROOT)/lib

INC		= $(RPROOT)/inc
TM      = ../../rpchannel
RPCORE  = ../../rpcore/
LOG4CPP=	/usr/local/include/log4cpp/
SC=         ../../util/commonCode


#DEBUG_CFLAGS	:= -Wall  -Wno-format -g -DDEBUG -fpermissive
RELEASE_CFLAGS	:= -Wall  -Wno-unknown-pragmas -Wno-format -O3
CFLAGS			= $(DEBUG_CFLAGS)

CC		= g++
LINK	= g++

proxyobj=$(OBJ)/rp_proxy.o $(OBJ)/logging.o

listnerobj=$(OBJ)/rp_listener.o $(OBJ)/logging.o

all: $(BIN)/rp_proxy $(BIN)/rp_listener 

$(BIN)/rp_proxy: $(proxyobj)
	$(LINK) -o $(BIN)/rp_proxy $(proxyobj) -L$(LIB) -L/usr/local/lib/ -lrpchannel-g -llog4cpp
ifneq "$(debug)" "1"
	strip -s $(BIN)/rp_proxy
endif

$(BIN)/rp_listener: $(listnerobj)
	$(LINK) -o $(BIN)/rp_listener $(listnerobj) -L$(LIB) -L/usr/local/lib/ -lrpchannel-g -lpthread -lvirt -llog4cpp
ifneq "$(debug)" "1"
	strip -s $(BIN)/rp_listener
endif

$(OBJ)/logging.o: $(SC)/logging.cpp $(SC)/logging.h 
	$(CC) $(CFLAGS) -I$(SC) -I$(LOG4CPP) -c -o $(OBJ)/logging.o $(SC)/logging.cpp

$(OBJ)/rp_proxy.o: rp_proxy.cpp
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -I. -I$(TM) -I$(RPCORE) -I$(LOG4CPP) -I$(SC) -c -o $(OBJ)/rp_proxy.o rp_proxy.cpp

$(OBJ)/rp_listener.o: rp_listener.cpp
	$(CC) $(CFLAGS) -I. -I$(TM) -I$(RPCORE) -I$(LOG4CPP) -I$(SC) -c -o $(OBJ)/rp_listener.o rp_listener.cpp

.PHONY: clean
clean:
	rm -f $(BIN)/rp_proxy
	rm -f $(OBJ)/rp_proxy.o
	rm -f $(BIN)/rp_listener
	rm -f $(OBJ)/rp_listener.o



