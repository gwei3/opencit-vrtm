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

listnerobj=$(OBJ)/rp_listener.o

all: $(BIN)/rp_proxy $(BIN)/rp_listener 

$(BIN)/rp_proxy: $(proxyobj)
	$(LINK) -o $(BIN)/rp_proxy $(proxyobj) -L$(LIB) -lrpchannel-g

$(BIN)/rp_listener: $(listnerobj)
	$(LINK) -o $(BIN)/rp_listener $(listnerobj) -L$(LIB) -lrpchannel-g -lpthread -lvirt

$(OBJ)/rp_proxy.o: rp_proxy.cpp
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -I. -I$(TM) -I$(RPCORE) -c -o $(OBJ)/rp_proxy.o rp_proxy.cpp

$(OBJ)/rp_listener.o: rp_listener.cpp
	$(CC) $(CFLAGS) -I. -I$(TM) -I$(RPCORE) -c -o $(OBJ)/rp_listener.o rp_listener.cpp

.PHONY: clean
clean:
	rm -f $(BIN)/rp_proxy
	rm -f $(OBJ)/rp_proxy.o
	rm -f $(BIN)/rp_listener
	rm -f $(OBJ)/rp_listener.o



