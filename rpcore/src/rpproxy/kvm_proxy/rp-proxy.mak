RPROOT	= ../../..
BIN     = $(RPROOT)/bin/debug
OBJ		= $(RPROOT)/build/debug/rpproxyobjects
LIB		= $(RPROOT)/lib
INC		= $(RPROOT)/inc
TM      = ../../rpchannel
RPCORE  = ../../rpcore/


DEBUG_CFLAGS	:= -Wall  -Wno-format -g -DDEBUG -fpermissive
RELEASE_CFLAGS	:= -Wall  -Wno-unknown-pragmas -Wno-format -O3
CFLAGS			= $(DEBUG_CFLAGS) 

CC		= g++
LINK	= g++

proxyobj=$(OBJ)/rp_proxy.o

listnerobj=$(OBJ)/rp_listner.o

all: $(BIN)/rp_proxy $(BIN)/rp_listner 

$(BIN)/rp_proxy: $(proxyobj)
	$(LINK) -o $(BIN)/rp_proxy $(proxyobj) -L$(LIB) -lrpchannel-g

$(BIN)/rp_listner: $(listnerobj)
	$(LINK) -o $(BIN)/rp_listner $(listnerobj) -L$(LIB) -lrpchannel-g -lpthread -lvirt

$(OBJ)/rp_proxy.o: rp_proxy.cpp
	$(CC) $(CFLAGS) -I. -I$(TM) -I$(RPCORE) -c -o $(OBJ)/rp_proxy.o rp_proxy.cpp

$(OBJ)/rp_listner.o: rp_listner.cpp
	$(CC) $(CFLAGS) -I. -I$(TM) -I$(RPCORE) -c -o $(OBJ)/rp_listner.o rp_listner.cpp

.PHONY: clean
clean:
	rm -f $(BIN)/rp_proxy
	rm -f $(OBJ)/rp_proxy.o
	rm -f $(BIN)/rp_listner
	rm -f $(OBJ)/rp_listner.o



