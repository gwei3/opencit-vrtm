E=          .
B=          ./obj
TPU=	    ./
S=          .
# CFLAGS=     -D UNIXRANDBITS -D TPMTEST

DEBUG_CFLAGS     := -Wall -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O3
CFLAGS=     -D UNIXRANDBITS -D TPMTEST -D QUOTE2_DEFINED -D TPMTEST -g
LDFLAGSXML      := ${RELEASE_LDFLAGS}

CC=         g++
LINK=       g++

dobjs=      $(B)/sealtest.o 

all: $(E)/sealtest.exe

$(E)/sealtest.exe: $(dobjs)
	@echo "sealtest"
	$(LINK) -o $(E)/sealtest.exe $(dobjs) -ltspi


$(B)/sealtest.o: $(S)/sealtest.cpp $(S)/vTCI.h
	$(CC) $(CFLAGS) -I$(S) -I$(SC) -I$(CR) -I$(TPU) -I$(BN) -I/usr/include/tss -c -o $(B)/sealtest.o $(S)/sealtest.cpp


