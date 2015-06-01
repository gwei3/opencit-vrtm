#
# Root Makefile
# Sujan Negi
#
# 
#RPROOT=~/rp
RPROOT=..
OBJROOTR=$(RPROOT)/build
#OBJROOTD=$(RPROOT)/build/debug
BIN=$(RPROOT)/bin
LIB=$(RPROOT)/lib

RMOBJROOTR=../build/
#RMOBJROOTD=../build/debug
RMBIN=../bin
RMLIB=../lib
 

BUILD_TARGETS=  makedirstructure \
		rpchannel-g  \
		nontpmrpcorsvc-g 


TARGETS=$(BUILD_TARGETS) clean

.PHONY: $(TARGETS)

all: $(BUILD_TARGETS)
makedirstructure:
	mkdir -p $(BIN)
	mkdir -p $(LIB)
	mkdir -p $(OBJROOTR)
nontpmrpcorsvc-g:
	mkdir -p $(OBJROOTR)/nontpmrpcorsvcobjects
	$(MAKE) -C rpcore debug=$(debug) -f nontpmrpcorsvc-g.mak

rpchannel-g:
	mkdir -p $(OBJROOTR)/rpchannelobjects
	$(MAKE) -C rpchannel debug=$(debug) -f rpchannel-g.mak
clean:

	rm -f $(RMOBJROOTR)/*objects/*.o
	rm -f $(RMOBJROOTR)/*objects/*.s
	rm -f $(RMOBJROOTR)/*.o
	rm -f $(RMOBJROOTR)/*/*.o
	rm -f $(RMOBJROOTR)/*/*.s
	rm -rf $(RMOBJROOTR)/*
	rm -f $(RMBIN)/*.exe
	rm -f $(RMBIN)/*
	rm -f $(RMLIB)/*.a
	rm -f $(RMLIB)/*.so


