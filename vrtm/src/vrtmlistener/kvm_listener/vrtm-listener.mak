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

listnerobj=$(OBJ)/vrtm_listener.o $(OBJ)/logging.o

all: $(BIN)/vrtm_listener 

$(BIN)/vrtm_listener: $(listnerobj)
	$(LINK) -o $(BIN)/vrtm_listener $(listnerobj) -L$(LIB) -L/usr/local/lib/ -lvrtmchannel-g -lpthread -lvirt -llog4cpp
ifneq "$(debug)" "1"
	strip -s $(BIN)/vrtm_listener
endif

$(OBJ)/logging.o: $(SC)/logging.cpp $(SC)/logging.h 
	$(CC) $(CFLAGS) -I$(SC) -I$(LOG4CPP) -c -o $(OBJ)/logging.o $(SC)/logging.cpp

$(OBJ)/vrtm_listener.o: vrtm_listener.cpp
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -I. -I$(TM) -I$(RPCORE) -I$(LOG4CPP) -I$(SC) -c -o $(OBJ)/vrtm_listener.o vrtm_listener.cpp


.PHONY: clean
clean:
	rm -f $(BIN)/vrtm_listener
	rm -f $(OBJ)/vrtm_listener.o



