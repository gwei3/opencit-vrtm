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
		rpcrypto-g   \
		rpchannel-g  \
		imvm \
		rptools-g    \
		keynego-g    \
		nontpmkss-g  \
		tsigner-g    \
		vtci         \
		rpmeasurer-g \
		rptrust-g    \
		rpquote-g    \
		rpmmio-g     \
		driver       \
		rptpm-emu-g rptpm-direct-g rptpm-tcsd-g \
		tpmtest-g    \
		nontpmrpcorsvc-g tcsd-rpcoreservice-g


TARGETS=$(BUILD_TARGETS) clean

.PHONY: $(TARGETS)

all: $(BUILD_TARGETS)
makedirstructure:
	mkdir -p $(BIN)/debug
	mkdir -p $(BIN)/release
	mkdir -p $(LIB)
	mkdir -p $(OBJROOTR)
	mkdir -p $(OBJROOTD)
imvm:
	$(MAKE) -C imvm -f verifier-g.mak
rpproxy:
	$(MAKE) -C rpproxy/kvm_proxy -f rp-proxy.mak
nontpmrpcorsvc-g:
	mkdir -p $(OBJROOTD)/nontpmrpcorsvcobjects
	$(MAKE) -C rpcore -f nontpmrpcorsvc-g.mak
rpcoreservice-g:
	mkdir -p $(OBJROOTD)/rpcoreserviceobjects
	$(MAKE) -C rpcore -f rpcoreservice-g.mak
tcsd-rpcoreservice-g:
	mkdir -p $(OBJROOTD)/rpcoreserviceobjects
	$(MAKE) -C rpcore -f tcsd-rpcoresvc-g.mak
keynego:
	mkdir -p $(OBJROOTR)/keyNegoServerobjects
	$(MAKE) -C rpapps -f keyNegoServer.mak
rpmeasurer-g:
	mkdir -p $(OBJROOTD)/dombldrobjects
	$(MAKE) -C measurer -f rpmeasurer-g.mak

rpchannel:
	mkdir -p $(OBJROOTR)/rpchannelobjects
	$(MAKE) -C rpchannel -f rpchannel.mak

rpchannel-g:
	mkdir -p $(OBJROOTD)/rpchannelobjects
	$(MAKE) -C rpchannel -f rpchannel-g.mak

rptpm-emu-g:
	mkdir -p $(OBJROOTD)/rptpmobjects
	$(MAKE) -C rptpm -f rptpm-emu-g.mak
rptpm-direct-g:
	mkdir -p $(OBJROOTD)/rptpmobjects
	$(MAKE) -C rptpm -f rptpm-direct-g.mak
rptpm-tcsd-g:
	mkdir -p $(OBJROOTD)/rptpmobjects
	$(MAKE) -C rptpm -f rptpm-tcsd-g.mak

keynego-g:
	mkdir -p $(OBJROOTD)/keyNegoServerobjects
	$(MAKE) -C rpapps -f keyNegoServer-g.mak

#The nontpmkss is for Non TPM version of keyNegoServer
#The nontpmrpcs is for the Non-TPM version of rpcoreservice/tcService
nontpmkss:
	mkdir -p $(OBJROOTR)/nontpmkssobjects
	$(MAKE) -C rpapps -f nontpmkss.mak
nontpmkss-g:
	mkdir -p $(OBJROOTD)/nontpmkssobjects
	$(MAKE) -C rpapps -f nontpmkss-g.mak

tsigner:
	mkdir -p $(OBJROOTR)/signerutilobjects
	$(MAKE) -C signingsvr -f tsigner.mak
tsigner-g:
	mkdir -p $(OBJROOTD)/signerutilobjects
	$(MAKE) -C signingsvr -f tsigner-g.mak
vtci:
	mkdir -p $(OBJROOTR)/vTCIobjects
	$(MAKE) -C rpapps -f vTCI.mak
rpcrypto:
	mkdir -p $(OBJROOTR)/rpcryptoobjects
	$(MAKE) -C rpcrypto -f rpcrypto.mak

rpcrypto-g:
	mkdir -p $(OBJROOTD)/rpcryptoobjects
	$(MAKE) -C rpcrypto -f rpcrypto-g.mak
rptools:
	mkdir -p $(OBJROOTR)/rptoolobjects
	$(MAKE) -C rptools -f rpapitools.mak
	mkdir -p $(OBJROOTR)/cryptUtilityobjects
	$(MAKE) -C rptools -f cryptUtility.mak
rptools-g:
	mkdir -p $(OBJROOTD)/rptoolobjects
	$(MAKE) -C rptools -f rpapitools-g.mak
	mkdir -p $(OBJROOTR)/cryptUtilityobjects
	$(MAKE) -C rptools -f cryptUtility.mak
rptest:
	mkdir -p $(OBJROOTR)/rptrustobjects
	$(MAKE) -C rptest/rptrusttest -f rptest.mak
	$(MAKE) -C rptest/rptrusttest -f rpapiSUAtest.mak
	mkdir -p $(OBJROOTR)/quoteobjects
	$(MAKE) -C rptest/rptrusttest -f qtest.mak
rptest-g:
	mkdir -p $(OBJROOTD)/rptrustobjects
	$(MAKE) -C rptest/rptrusttest -f rpapiSUAtest-g.mak
	mkdir -p $(OBJROOTD)/quoteobjects
	$(MAKE) -C rptest/rptrusttest -f qtest-g.mak
rptrust:
	mkdir -p $(OBJROOTR)/rptrustobjects
	$(MAKE) -C rptrust -f rptrust.mak
rptrust-g:
	mkdir -p $(OBJROOTD)/rptrustobjects
	$(MAKE) -C rptrust -f rptrust-g.mak
rpquote-g:
	mkdir -p $(OBJROOTD)/librpquoteobjects
	$(MAKE) -C rpquote
tpmtest:
	mkdir -p $(OBJROOTR)/rptrustobjects
	$(MAKE) -C rptest/tpmtest -f tpmextend.mak
tpmtest-g:
	mkdir -p $(OBJROOTR)/rptrustobjects
	$(MAKE) -C rptest/tpmtest -f tpmextend-g.mak
rpmmio:
	$(MAKE) -C rptpm/rpmmio
rpmmio-g:
	$(MAKE) -C rptpm/rpmmio
driver:
	$(MAKE) -C driver
	
clean:

	rm -f $(RMOBJROOTR)/*objects/*.o
	rm -f $(RMOBJROOTR)/*objects/*.s
	rm -f $(RMOBJROOTR)/*.o
	rm -f $(RMOBJROOTR)/*/*.o
	rm -f $(RMOBJROOTR)/*/*.s
	rm -f $(RMBIN)/release/*.exe
	rm -f $(RMBIN)/release/*
	rm -f $(RMLIB)/*.a
	rm -f $(RMOBJROOTD)/*/*.o
	rm -f $(RMOBJROOTD)/*/*.s
	rm -f $(RMBIN)/debug/*.exe
	rm -f $(RMBIN)/debug/*
	rm -f $(RMLIB)/*.a
	rm -f $(RMLIB)/*.so

	rm -f rptrust/*.o
	rm -f domstart/*.o
	rm -f rptrust/*.exe
	rm -f domstart/*.exe

	rm -f ../install/*.*

	rm -f ../install/rplib/*.*
	rm -f ../install/rplib/rp/*.*
	rm -f ../install/rplib/rp/examples/*
	rm  -f ../install/rplib/rp/lib/*
	rm  -f ../install/rplib/rp/inc/*

	rm -f ../install/rpplat/*.*
	rm -f ../install/rpplat/rp/*.*
	rm -f ../install/rpplat/rp/bin/*
	rm  -f ../install/rpplat/rp/xen/*
	rm  -rf ../install/rpplat/rp/src/*
	rm  -rf ../install/rpplat/rp/config/*
	rm  -rf ../install/rpplat/rp/domimages/3.7.1/*
	rm  -rf ../install/rpplat/rp/domimages/*

	rm -f ../install/rpkss/rp/bin/*
	rm -f ../install/rpkss/rp/bin/kss
	rm -f ../install/rpkss/rp/*.txt
	rm -f ../install/rpkss/rp/config/*.xml
	rm -rf ../install/rpkss/rp/config/policy

	rm -f ../reldir/*

INSTALL_DIR= ../install/rplib
INSTALL_DIR-p= ../install/rpplat
INSTALL_DIR-k= ../install/rpkss
RELEASE_DIR = ../reldir

PACKAGELIB   = RPCryptoLib
VERSION   = ` date +"%Y.%m%d%H%M" `
RELEASE_FILE1   = $(PACKAGELIB)-$(VERSION)
 
PACKAGEPLAT   = RPplat
VERSION   = ` date +"%Y.%m%d%H%M" `
RELEASE_FILE2   = $(PACKAGEPLAT)-$(VERSION)

PACKAGEKSS   = RPkss
VERSION   = ` date +"%Y.%m%d%H%M" `
RELEASE_FILE3   = $(PACKAGEKSS)-$(VERSION)
#Make a release
#cp ../config/p2048tpm.xml	../install/rpplat/rp/config/
#Driver code no longer needed from Jul 1 as this has been replaced with
#tcp/ip sokcs.

#cp -r driver                    ../install/rpplat/rp/src/
#cp /etc/xen/dusample            ../install/rpplat/rp/xen/


install:
	unix2dos -k -n ./rptools/releasenotes/ReleaseNotes.UX ./rptools/releasenotes/ReleaseNotes.txt
	unix2dos -k -n ./rptools/releasenotes/RPapidoc.UX ./rptools/releasenotes/RPapidoc.txt
	unix2dos -k -n ./rptools/releasenotes/RPplatReleaseNotes.UX ./rptools/releasenotes/RPplatReleaseNotes.txt
	unix2dos -k -n ./rptools/releasenotes/RPkssReleaseNotes.UX ./rptools/releasenotes/RPkssReleaseNotes.txt
	cp ../lib/librpcrypto.a     ../install/rplib/rp/lib/
	cp ../inc/rpapi.h           ../install/rplib/rp/inc/
	cp ../examples/rpapisample1.cpp  ../install/rplib/rp/examples/
	cp ../examples/rpapisample1.h    ../install/rplib/rp/examples/
	cp ../examples/PlainText1        ../install/rplib/rp/examples/
	cp ../examples/rpapisample3.cpp  ../install/rplib/rp/examples/
	cp ../examples/rpapisample3.h    ../install/rplib/rp/examples/
	cp  ./rptools/releasenotes/ReleaseNotes.txt  ../install/rplib/rp/
	cp  ./rptools/releasenotes/RPapidoc.txt      ../install/rplib/rp/

	cp ../config/tcconfig.xml	../install/rpplat/rp/config/
	cp -r ../config/HW	        ../install/rpplat/rp/config/
	cp -r ../config/HWRoot	        ../install/rpplat/rp/config/
	cp -r ../config/policy	        ../install/rpplat/rp/config/
	cp -r ../config/TrustedOS	../install/rpplat/rp/config/
	cp ../config/RSAkey.xml         ../install/rpplat/rp/config/
	cp ../config/PubKey.xml         ../install/rpplat/rp/config/
	cp ../config/AIK.xml            ../install/rpplat/rp/config/
	cp ../config/TrOS.xml           ../install/rpplat/rp/config/

	cp  ../domimages/*.igz	        ../install/rpplat/rp/domimages/
	cp  ../domimages/mkirfs_r.sh    ../install/rpplat/rp/domimages/mkirfs.sh
	cp  ../domimages/cinit	        ../install/rpplat/rp/domimages/
	cp  ../domimages/kinit	        ../install/rpplat/rp/domimages/
	cp  ../domimages/sinit	        ../install/rpplat/rp/domimages/
	cp  ../domimages/linit	        ../install/rpplat/rp/domimages/
	cp  ../domimages/lvm_many.sh    ../install/rpplat/rp/domimages/
	cp  ../domimages/vmc.sh    ../install/rpplat/rp/domimages/
	cp  ../domimages/kvm.sh    ../install/rpplat/rp/domimages/
	cp  ../domimages/rpcrdom.sh    ../install/rpplat/rp/domimages/
	cp -r ../domimages/3.7.1	../install/rpplat/rp/domimages/

	cp /etc/xen/kns                 ../install/rpplat/rp/xen/
	cp ../bin/release/rpcoreservice    ../install/rpplat/rp/bin/rpcoreservice
	cp ../bin/release/rptest         ../install/rpplat/rp/bin/
	cp ../bin/release/rpapiSUAtest   ../install/rpplat/rp/bin/
	cp ../bin/release/rpapiQVtest   ../install/rpplat/rp/bin/
	cp ../bin/release/domtest   	 ../install/rpplat/rp/bin/
	cp  ./rptools/releasenotes/RPplatReleaseNotes.txt      ../install/rpplat/rp/
	
	cp ../bin/release/kss            ../install/rpkss/rp/bin/kss
	cp ../bin/release/vTCI   	 ../install/rpkss/rp/bin/
	cp ../bin/release/signerutil   	 ../install/rpkss/rp/bin/
	cp  ./rptools/releasenotes/RPkssReleaseNotes.txt      ../install/rpkss/rp/
	cp ../config/tcconfig.xml	../install/rpkss/rp/config/
	cp -r ../config/policy	        ../install/rpkss/rp/config/
#

#tar -cvf  $(RELEASE_DIR)/$(RELEASE_FILE).tar -C  $(INSTALL_DIR) rp
#tar -cvf  $(RELEASE_DIR)/$(RELEASE_FILE)-g.tar -C  $(INSTALL_DIR-g) rp
dist:
	tar -czvf  $(RELEASE_DIR)/$(RELEASE_FILE1).tgz -C $(INSTALL_DIR) rp
	md5sum  $(RELEASE_DIR)/$(RELEASE_FILE1).tgz > $(RELEASE_DIR)/$(RELEASE_FILE1).MD5SUM
	tar -czvf  $(RELEASE_DIR)/$(RELEASE_FILE2).tgz -C $(INSTALL_DIR-p) rp
	md5sum  $(RELEASE_DIR)/$(RELEASE_FILE2).tgz > $(RELEASE_DIR)/$(RELEASE_FILE2).MD5SUM
	tar -czvf  $(RELEASE_DIR)/$(RELEASE_FILE3).tgz -C $(INSTALL_DIR-k) rp
	md5sum  $(RELEASE_DIR)/$(RELEASE_FILE3).tgz > $(RELEASE_DIR)/$(RELEASE_FILE3).MD5SUM


cleaninstall:

	rm -f ../install/rplib/rp/examples/*
	rm -f ../install/rplib/rp/inc/*
	rm  -f ../install/rplib/rp/lib/*
	rm  -f ../install/rplib/rp/*.*
	rm  -f ../install/rplib/*.*
	rm  -f ../install/rpkss/rp/bin/vTCI
	rm  -f ../install/rpkss/rp/bin/signerutil


	rm  -rf ../install/rpplat/rp/src/*
	rm  -rf ../install/rpplat/rp/config/*

	rm  -f ../install/rpplat/rp/bin/*
	rm  -f ../install/rpplat/rp/xen/*
	rm  -rf ../install/rpplat/rp/domimages/3.7.1/*
	rm  -rf ../install/rpplat/rp/domimages/*
	rm  -f ../install/rpplat/rp/*.*
	rm  -f ../install/rpplat/*.*

	rm -f ../install/rpkss/rp/bin/kss
	rm -f ../install/rpkss/rp/*.txt
	rm -f ../install/rpkss/rp/config/*.xml
	rm -rf ../install/rpkss/rp/config/policy

cleandist:
	rm -f ../reldir/*.tgz
	rm -f ../reldir/*.MD5SUM

cleanlib:
	rm -f $(LIB)/*.a
	rm -f $(LIB)/*.so
	rm -f $(OBJROOTR)/rpcryptoobjects/*.o
	rm -f $(OBJROOTR)/rpcryptoobjects/*.s
	rm -f $(OBJROOTD)/rpcryptoobjects/*.o
	rm -f $(OBJROOTD)/rpcryptoobjects/*.s

cleanbin:
	rm -f $(BIN)/release/*
	rm -f $(BIN)/debug/*
	rm -f $(BIN)/*
cleantc:
	rm -f $(OBJROOTR)/tcServiceobjects/*.o
	rm -f $(OBJROOTR)/tcServiceobjects/*.s
	rm -f $(OBJROOTD)/tcServiceobjects/*.o
	rm -f $(OBJROOTD)/tcServiceobjects/*.s
	rm -f $(BIN)/release/tc*
	rm -f $(BIN)/debug/tc*
cleanvmtc:
	rm -f $(OBJROOTR)/vmtcServiceobjects/*.o
	rm -f $(OBJROOTR)/vmtcServiceobjects/*.s
	rm -f $(OBJROOTD)/vmtcServiceobjects/*.o
	rm -f $(OBJROOTD)/vmtcServiceobjects/*.s
	rm -f $(BIN)/release/rpcore*
	rm -f $(BIN)/debug/rpcore*

cleanfs:
	rm -f $(OBJROOTR)/fileServerobjects/*.o
	rm -f $(OBJROOTR)/fileServerobjects/*.s
	rm -f $(OBJROOTD)/fileServerobjects/*.o
	rm -f $(OBJROOTD)/fileServerobjects/*.s
	rm -f $(BIN)/release/fileS*
	rm -f $(BIN)/debug/fileS*
cleanfc:
	rm -f $(OBJROOTR)/fileClientobjects/*.o
	rm -f $(OBJROOTR)/fileClientobjects/*.s
	rm -f $(OBJROOTD)/fileClientobjects/*.o
	rm -f $(OBJROOTD)/fileClientobjects/*.s
	rm -f $(BIN)/release/fileC*
	rm -f $(BIN)/debug/fileC*

cleankeynego:
	rm -f $(OBJROOTR)/keyNegoServerobjects/*.o
	rm -f $(OBJROOTR)/keyNegoServerobjects/*.s
	rm -f $(OBJROOTD)/keyNegoServerobjects/*.o
	rm -f $(OBJROOTD)/keyNegoServerobjects/*.s
	rm -f $(BIN)/release/kss
	rm -f $(BIN)/debug/kss
cleanexamples:
	rm -f $(OBJROOTR)/examplesobjects/*.o
	rm -f $(OBJROOTR)/examplesobjects/*.s
	rm -f $(OBJROOTD)/examplesobjects/*.o
	rm -f $(OBJROOTD)/examplesobjects/*.s
	rm -f $(BIN)/release/rpapiS*
	rm -f $(BIN)/debug/rpapiS*
cleanrptools:
	rm -f $(OBJROOTR)/rptoolobjects/*.o
	rm -f $(OBJROOTR)/rptoolobjects/*.s
	rm -f $(OBJROOTD)/rptoolobjects/*.o
	rm -f $(OBJROOTD)/rptoolobjects/*.s
	rm -f $(BIN)/release/rpapicryputil
	rm -f $(BIN)/debug/rpapicryputil
	rm -f $(BIN)/release/rpapiTest
	rm -f $(BIN)/debug/rpapiTest
	rm -f $(BIN)/debug/rputil*
	rm -f $(BIN)/release/rputil*
cleanrptrust:
	rm -f $(OBJROOTR)/rptrustobjects/*.o
	rm -f $(OBJROOTR)/rptrustobjects/*.s
	rm -f $(OBJROOTD)/rptrustobjects/*.o
	rm -f $(OBJROOTD)/rptrustobjects/*.s
	rm -f $(BIN)/release/rptest
	rm -f $(BIN)/debug/rptest
	rm -f $(BIN)/debug/rpapiSUAtest
	rm -f $(BIN)/release/rpapiSUAtest
	rm -f $(BIN)/debug/rpapiQVtest
	rm -f $(BIN)/release/rpapiQVtest
	rm -f $(BIN)/debug/rpapiVtest
	rm -f $(BIN)/release/rpapiVtest
	rm -f $(BIN)/debug/qtest
	rm -f $(BIN)/release/qtest

cleancrypt:
	rm -f $(OBJROOTR)/cryptUtilityobjects/*.s
	rm -f $(OBJROOTR)/cryptUtilityobjects/*.o
	rm -f $(OBJROOTD)/cryptUtilityobjects/*.o
	rm -f $(OBJROOTD)/cryptUtilityobjects/*.s
	rm -f $(BIN)/release/cryptUtility
	rm -f $(BIN)/debug/cryptUtility

cleansigner:
	rm -f $(OBJROOTR)/signerutilobjects/*.s
	rm -f $(OBJROOTR)/signerutilobjects/*.o
	rm -f $(BIN)/release/signerutil
	#rm -f $(BIN)/debug/signerutil
	#rm -f $(OBJROOTD)/signerutilobjects/*.o
	#rm -f $(OBJROOTD)/signerutilobjects/*.s

cleanvtci:
	rm -f $(OBJROOTR)/vTCIobjects/*.s
	rm -f $(OBJROOTR)/vTCIobjects/*.o
	rm -f $(BIN)/release/vTCI
	#rm -f $(OBJROOTD)/vTCIobjects/*.o
	#rm -f $(OBJROOTD)/vTCIobjects/*.s
	#rm -f $(BIN)/debug/vTCI

cleannewrpcrypto:
	rm -f $(OBJROOTR)/newrpcryptoobjects/*.s
	rm -f $(OBJROOTD)/newrpcryptoobjects/*.o
	rm -f $(OBJROOTR)/newrpcryptoobjects/*.s
	rm -f $(OBJROOTD)/newrpcryptoobjects/*.o
	rm -f $(BIN)/release/
	rm -f $(BIN)/debug/

cleannontpmkss:
	rm -f $(OBJROOTR)/nontpmkssobjects/*.s
	rm -f $(OBJROOTR)/nontpmkssobjects/*.o
	rm -f $(OBJROOTD)/nontpmkssobjects/*.s
	rm -f $(OBJROOTD)/nontpmkssobjects/*.o
	rm -f $(BIN)/release/nontpmkss
	rm -f $(BIN)/debug/nontpmkss

cleannontpmsrpcs:
	rm -f $(OBJROOTR)/nontpmrpcsobjects/*.s
	rm -f $(OBJROOTR)/nontpmrpcsobjects/*.o
	rm -f $(OBJROOTD)/nontpmrpcsobjects/*.s
	rm -f $(OBJROOTD)/nontpmrpcsobjects/*.o
	rm -f $(BIN)/release/nontpmrpcoreservice
	rm -f $(BIN)/debug/nontpmrpcoreservice

cleanquotes:
	rm -f $(OBJROOTR)/quoteobjects/*.s
	rm -f $(OBJROOTR)/quoteobjects/*.o
	rm -f $(OBJROOTD)/quoteobjects/*.s
	rm -f $(OBJROOTD)/quoteobjects/*.o
	rm -f $(BIN)/release/qtest
	rm -f $(BIN)/debug/qtest
