E=          .
B=          ./obj/vTCIobjects
TPU=	    ../TPMUser
S=          .
# CFLAGS=     -D UNIXRANDBITS -D TPMTEST

DEBUG_CFLAGS     := -Wall -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O3
CFLAGS=     -D UNIXRANDBITS -D TPMTEST -D QUOTE2_DEFINED -g
LDFLAGSXML      := ${RELEASE_LDFLAGS}

CC=         g++
LINK=       g++

dobjs=      $(B)/vTCI.o 

all: $(E)/vTCI.exe

$(E)/vTCI.exe: $(dobjs)
	@echo "vTCI"
	$(LINK) -o $(E)/vTCI.exe $(dobjs) -ltspi


$(B)/vTCI.o: $(S)/vTCI.cpp $(S)/vTCI.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(CR) -I$(TPU) -I$(BN) -I/usr/include/tss -c -o $(B)/vTCI.o $(S)/vTCI.cpp


