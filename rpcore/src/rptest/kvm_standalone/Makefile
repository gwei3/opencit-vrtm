all: testapp kvm_client kvm_standalone
testapp: testapp.cpp ../../rpchannel/tcpchan.cpp ../../rpchannel/tcpchan.h
		g++ -g testapp.cpp  -I ../../rpcore -I ../../rpchannel/ -I ../../rptrust -L ../../../lib -lrptrust-g -lrpchannel-g -o testapp
kvm_client: kvm_client.cpp kvm_standalone.h ../../rpchannel/tcpchan.cpp ../../rpchannel/tcpchan.h ../../rptrust/rptrust.h
		g++ -g -o kvm_client kvm_client.cpp -I ../../rpcore -I ../../rpchannel/ -I ../../rptrust -I ../../rpcore -L ../../../lib -lrptrust-g  -lrpcrypto-g -lrpchannel-g 
kvm_standalone: kvm_standalone.cpp kvm_standalone.h ../../rpchannel/tcpchan.cpp ../../rpchannel/tcpchan.h ../../rptrust/rptrust.h
		g++ -g -o kvm_standalone kvm_standalone.cpp -I ../../rpcore -I ../../rpchannel/ -I ../../rptrust -I ../../rpcore -L ../../../lib -lrptrust-g  -lrpcrypto-g -lrpchannel-g 
clean:
		rm testapp kvm_client kvm_standalone
