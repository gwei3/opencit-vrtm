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
CFLAGS	= -fPIE -fPIC -fstack-protector -O2 -D FORTIFY_SOURCE=2 $(DEBUG_CFLAGS)

CC	= g++
LINK	= g++

proxyobj=$(OBJ)/vrtm_proxy.o $(OBJ)/logging.o

all: $(BIN)/vrtm_proxy

$(BIN)/vrtm_proxy: $(proxyobj)
	$(LINK) -o $(BIN)/vrtm_proxy $(proxyobj) $(LDFLAGS) -L$(LIB) -L$(SAFESTRING) -L/usr/local/lib/ -lSafeStringRelease -lvrtmchannel -llog4cpp

ifneq "$(debug)" "1"
	strip -s $(BIN)/vrtm_proxy
endif

$(OBJ)/logging.o: $(SC)/logging.cpp $(SC)/logging.h 
	$(CC) $(CFLAGS) -I$(SC) -I$(LOG4CPP) -c -o $(OBJ)/logging.o $(SC)/logging.cpp

$(OBJ)/vrtm_proxy.o: vrtm_proxy.cpp
	mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -I. -I$(TM) -I$(RPCORE) -I$(LOG4CPP) -I$(SC) -I$(SAFESTRING_INCLUDE) -c -o $(OBJ)/vrtm_proxy.o vrtm_proxy.cpp

.PHONY: clean
clean:
	rm -f $(BIN)/vrtm_proxy
	rm -f $(OBJ)/vrtm_proxy.o



