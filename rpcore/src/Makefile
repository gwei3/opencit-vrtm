#
# Root Makefile
# Sujan Negi
#
# 
#RPROOT=~/rp
RPROOT=..
OBJROOTR=$(RPROOT)/build/release
OBJROOTD=$(RPROOT)/build/debug
BIN=$(RPROOT)/bin
LIB=$(RPROOT)/lib

RMOBJROOTR=../build/release
RMOBJROOTD=../build/debug
RMBIN=../bin
RMLIB=../lib

BUILD_TARGETS=  makedirstructure \
		rpchannel-g  \
		nontpmrpcorsvc-g 


TARGETS=$(BUILD_TARGETS) clean

.PHONY: $(TARGETS)

all: $(BUILD_TARGETS)
makedirstructure:
	mkdir -p $(BIN)/debug
	mkdir -p $(BIN)/release
	mkdir -p $(LIB)
	mkdir -p $(OBJROOTR)
	mkdir -p $(OBJROOTD)
nontpmrpcorsvc-g:
	mkdir -p $(OBJROOTD)/nontpmrpcorsvcobjects
	$(MAKE) -C rpcore -f nontpmrpcorsvc-g.mak

rpchannel-g:
	mkdir -p $(OBJROOTD)/rpchannelobjects
	$(MAKE) -C rpchannel -f rpchannel-g.mak
clean:

	rm -f $(RMOBJROOTR)/*objects/*.o
	rm -f $(RMOBJROOTR)/*objects/*.s
	rm -f $(RMOBJROOTR)/*.o
	rm -f $(RMOBJROOTR)/*/*.o
	rm -f $(RMOBJROOTR)/*/*.s
	rm -rf $(RMOBJROOTR)/*
	rm -f $(RMBIN)/release/*.exe
	rm -f $(RMBIN)/release/*
	rm -f $(RMLIB)/*.a
	rm -f $(RMOBJROOTD)/*/*.o
	rm -f $(RMOBJROOTD)/*/*.s
	rm -rf $(RMOBJROOTD)/*
	rm -f $(RMBIN)/debug/*.exe
	rm -f $(RMBIN)/debug/*
	rm -f $(RMLIB)/*.a
	rm -f $(RMLIB)/*.so


