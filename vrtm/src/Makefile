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
		vrtmchannel-g  \
		vrtmcore-g 


TARGETS=$(BUILD_TARGETS) clean

.PHONY: $(TARGETS)

all: $(BUILD_TARGETS)
makedirstructure:
	mkdir -p $(BIN)
	mkdir -p $(LIB)
	mkdir -p $(OBJROOTR)
vrtmcore-g:
	mkdir -p $(OBJROOTR)/vrtmcoreobjects
	$(MAKE) -C vrtmcore debug=$(debug) -f vrtmcore-g.mak

vrtmchannel-g:
	mkdir -p $(OBJROOTR)/vrtmchannelobjects
	$(MAKE) -C vrtmchannel debug=$(debug) -f vrtmchannel-g.mak
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


