RPROOT=../..
BIN=        $(RPROOT)/bin/debug
OBJ=        $(RPROOT)/build/debug/rptoolobjects
#BIN=        $(RPROOT)/bin/release
#OBJ=        $(RPROOT)/build/release/rptoolobjects
#LIBHOME=    /home/s1
#LIB=        $(LIBHOME)/rp/lib
LIB=        $(RPROOT)/lib
INC=        $(RPROOT)/inc

DEBUG_CFLAGS	:= -Wall -Wno-format -g -DDEBUG
RELEASE_CFLAGS	:= -Wall -Wno-unknown-pragmas -Wno-format -O3
CFLAGS=      -D NOAESNI $(DEBUG_CFLAGS)
#CFLAGS=      -D NOAESNI $(RELEASE_CFLAGS)
CFLAGS1=     -D NOAESNI -Wall -Wno-unknown-pragmas -Wno-format -O1

CC=         g++
LINK=       g++

rpapicryputilobjs=     	$(OBJ)/rpapicryputil.o
rpapitestobjs=     	$(OBJ)/rpapitest.o
tobjs=             	$(OBJ)/rputil.o


all: 	$(BIN)/rpapiTest  \
	$(BIN)/rpapicrypUtil \
	$(BIN)/rputil

$(BIN)/rpapiTest: $(rpapitestobjs)
	@echo "Linking rpapiTest ..."
	$(LINK) -o $(BIN)/rpapiTest $(rpapitestobjs) -L$(LIB)  -lrpchannel-g  -lrpcrypto-g

$(OBJ)/rpapitest.o: ./rpapitest.cpp ./rpapitest.h $(INC)/rpapi.h
	@echo "Compiling..."
	$(CC) $(CFLAGS) -I. -I$(INC) -c -o $(OBJ)/rpapitest.o ./rpapitest.cpp 


$(BIN)/rpapicrypUtil: $(rpapicryputilobjs)
	@echo "Linking rpapicryputil ..."
	$(LINK) -o $(BIN)/rpapicryputil $(rpapicryputilobjs) -lpthread  -L$(LIB) -lrpcrypto-g


$(OBJ)/rpapicryputil.o: ./rpapicryputil.cpp ./rpapicryputil.h $(INC)/rpapi.h
	@echo "Compiling..."
	$(CC) $(CFLAGS) -I. -I$(INC)  -c -o $(OBJ)/rpapicryputil.o ./rpapicryputil.cpp 

$(BIN)/rputil: $(tobjs)
	@echo "Linking rputil ..."
	$(LINK) -o $(BIN)/rputil $(tobjs) -L$(LIB)  -lrpchannel-g  -lrpcrypto-g

$(OBJ)/rputil.o: ./rputil.cpp ./rpapicryputil.h $(INC)/rpapi.h
	@echo "Compiling..."
	$(CC) $(CFLAGS) -I. -I$(INC)  -c -o $(OBJ)/rputil.o ./rputil.cpp

