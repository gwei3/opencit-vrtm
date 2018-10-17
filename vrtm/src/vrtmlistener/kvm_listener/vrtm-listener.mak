RPROOT	= ../../..
ifeq ($(debug),1)
	DEBUG_CFLAGS    := -Wall  -Wno-format -g -DDEBUG -fpermissive
else
	DEBUG_CFLAGS    := -Wall -Wno-unknown-pragmas -Wno-format -fpermissive -O3 -Wformat -Wformat-security
endif

BIN     	= $(RPROOT)/bin
OBJ             = $(RPROOT)/build/vrtmproxyobjects
LIB             = $(RPROOT)/lib

INC		= $(RPROOT)/inc
TM      = ../../vrtmchannel
RPCORE  = ../../vrtmcore/
LOG4CPP=	/usr/include/log4cpp/
SC=         ../../util
SAFESTRING=     $(SC)/SafeStringLibrary/
SAFESTRING_INCLUDE=    $(SAFESTRING)/include/

#DEBUG_CFLAGS	:= -Wall  -Wno-format -g -DDEBUG -fpermissive
#RELEASE_CFLAGS	:= -Wall  -Wno-unknown-pragmas -Wno-format -O3
LDFLAGS  := -pie -z noexecstack -z relro -z now
CFLAGS	= -fPIE -fPIC -fstack-protector-strong -O2 -D FORTIFY_SOURCE=2 $(DEBUG_CFLAGS)

CC	= g++
LINK	= g++

listnerobj=$(OBJ)/vrtm_listener.o $(OBJ)/logging.o

all: $(BIN)/vrtm_listener 

$(BIN)/vrtm_listener: $(listnerobj)
	$(LINK) -o $(BIN)/vrtm_listener $(listnerobj) -L$(LIB) -L$(SAFESTRING) -L/usr/local/lib/ -lvrtmchannel -lpthread -lvirt -llog4cpp -lSafeStringRelease $(LDFLAGS)
ifneq "$(debug)" "1"
	strip -s $(BIN)/vrtm_listener
endif

$(OBJ)/logging.o: $(SC)/logging.cpp $(SC)/logging.h 
	$(CC) $(CFLAGS) -I$(SC) -I$(LOG4CPP) -c -o $(OBJ)/logging.o $(SC)/logging.cpp

$(OBJ)/vrtm_listener.o: vrtm_listener.cpp
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -I. -I$(TM) -I$(RPCORE) -I$(LOG4CPP) -I$(SC) -I$(SAFESTRING_INCLUDE) -c -o $(OBJ)/vrtm_listener.o vrtm_listener.cpp


.PHONY: clean
clean:
	rm -f $(BIN)/vrtm_listener
	rm -f $(OBJ)/vrtm_listener.o



