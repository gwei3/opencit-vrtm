#!/bin/bash

#This script installs RPCore, RPProxy, RPListner, and openstack patches


RES_DIR=$PWD
DEFAULT_INSTALL_DIR=/opt

OPENSTACK_DIR="Openstack/patch"
DIST_LOCATION=`/usr/bin/python -c "import site; print ( site.getsitepackages()[1] )" `

LINUX_FLAVOUR="ubuntu"
NON_TPM="false"
BUILD_LIBVIRT="FALSE"
RC_LOCAL_FILE=""
KVM_BINARY=""

function valid_ip()
{
    local  ip=$1
    local  stat=1

    if [[ $ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
        OIFS=$IFS
        IFS='.'
        ip=($ip)
        IFS=$OIFS
        [[ ${ip[0]} -le 255 && ${ip[1]} -le 255 \
            && ${ip[2]} -le 255 && ${ip[3]} -le 255 ]]
        stat=$?
    fi
    return $stat
}

# This function returns either rhel, fedora or ubuntu
# TODO : This function can be moved out to some common file
function getFlavour()
{
        flavour=""
        grep -c -i ubuntu /etc/*-release > /dev/null
        if [ $? -eq 0 ] ; then
                flavour="ubuntu"
        fi
        grep -c -i "red hat" /etc/*-release > /dev/null
        if [ $? -eq 0 ] ; then
                flavour="rhel"
        fi
        grep -c -i fedora /etc/*-release > /dev/null
        if [ $? -eq 0 ] ; then
                flavour="fedora"
        fi
        if [ "$flavour" == "" ] ; then
                echo "Unsupported linux flavor, Supported versions are ubuntu, rhel, fedora"
                exit
        else
                echo $flavour
        fi
}

function updateFlavourVariables()
{
        linuxFlavour=`getFlavour`
        if [ $linuxFlavour == "fedora" -o $linuxFlavour == "rhel" ]
        then
             export RC_LOCAL_FILE="/etc/rc.d/rc.local"
             export KVM_BINARY="/usr/bin/qemu-kvm"
	elif [ $linuxFlavour == "ubuntu" ]
	then
              export RC_LOCAL_FILE="/etc/rc.local"
              export KVM_BINARY="/usr/bin/kvm"
        fi
}


function untarResources()
{
	cd "$RES_DIR"
	cp KVM_*.tar.gz "$INSTALL_DIR"
	cd "$INSTALL_DIR"
	tar xvzf *.tar.gz
        if [ $? -ne 0 ]; then
                echo "ERROR : Untarring of $RES_DIR/*.tar.gz unsuccessful"
                exit
        fi
}

function installKVMPackages_rhel()
{
        echo "Installing Required Packages ....."
        yum -y update
        yum install -y "kernel-devel-uname-r == $(uname -r)"
        # Install the openstack repo
        yum install -y yum-plugin-priorities
        yum install -y https://repos.fedorapeople.org/repos/openstack/openstack-icehouse/rdo-release-icehouse-3.noarch.rpm

        yum install -y libvirt-devel libvirt libvirt-python
        #Libs required for compiling libvirt
        yum install -y gcc-c++ gcc make yajl yajl-devel device-mapper device-mapper-devel libpciaccess-devel libnl-devel libxml2
        yum install -y python-devel
        yum install -y openssh-server
	yum install -y trousers tpm-tools cryptsetup
	yum install -y tar procps

}

function installKVMPackages_ubuntu()
{
	echo "Installing Required Packages ....."
	apt-get -y install bridge-utils dnsmasq pm-utils ebtables ntp chkconfig guestfish
	apt-get -y install openssh-server
	apt-get -y install python-dev
	apt-get -y install tboot trousers tpm-tools libtspi-dev cryptsetup-bin
	apt-get -y install qemu-utils kpartx
	apt-get -y install iptables libblkid1 libc6 libcap-ng0 libdevmapper1.02.1 libgnutls26 libnl-3-200 libnuma1  libparted0debian1  libpcap0.8 libpciaccess0 libreadline6  libudev0  libxml2  libyajl1 procps
	
	echo "Starting ntp service ....."
	service ntp start
	chkconfig ntp on
}

function installKVMPackages()
{
        if [ $FLAVOUR == "ubuntu" ] ; then
		installKVMPackages_ubuntu
        elif [  $FLAVOUR == "rhel" -o $FLAVOUR == "fedora" ] ; then
		installKVMPackages_rhel
        fi

}

installLibvirtPackages_ubuntu()
{
        apt-get -y install python-software-properties
        add-apt-repository -y cloud-archive:icehouse

        echo "Updating repositories .. this may take a while "
        apt-get update > /dev/null
        if [ $? -ne 0 ] ; then
               echo "apt-get update failed, kindly resume after manually executing apt-get update"
        fi
        apt-get -y install libvirt-bin libvirt-dev libvirt0 python-libvirt

}

installLibvirtPackages_rhel()
{
	yum -y install libvirt-devel libvirt libvirt-python
        # This is required because if one installs only libvirt then the eco-system for libvirt is not ready
        # For e.g the virtualisation group also creates libvirtd group over the system.
        yum groupinstall -y Virtualization	
	grep -c libvirt /etc/groups
	if [ $? -eq 1 ] ; then
		groupadd libvirt
	fi
}

installLibvirtPackages()
{
        if [ $FLAVOUR == "ubuntu" ] ; then
		installLibvirtPackages_ubuntu
        elif [  $FLAVOUR == "rhel" -o $FLAVOUR == "fedora" ] ; then
		installLibvirtPackages_rhel
        fi
	
}


function installLibvirt()
{
	# Openstack icehouse repo contains libvirt 1.2.2
	# Adding the repository
	
	if [ $BUILD_LIBVIRT == "TRUE" ] ; then
		if [ -e libvirt-1.2.2.tar.gz ] ; then
			echo "Using the packaged libvirt found in dist"
		else
			echo "This dist package does not contain custom libvirt"
			echo "Please create a dist package using --with-libvirt option"
			echo "Aborting install process.."
			exit
		fi

	    tar xvzf libvirt-1.2.2.tar.gz
	    cd libvirt-1.2.2
	    ./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc --with-xen=no --with-esx=no
	    make -j 4
	        if [ $? -ne 0 ]; then
	                echo "ERROR : make failed for libvirtd "
	                exit
	        else
	                echo "INFO : Libvirtd make PASSED"
	        fi
	    make install
	        if [ $? -ne 0 ]; then
	                echo "ERROR : make install failed for libvirtd "
	                exit
	        else
	                echo "INFO : make install PASSED"
	        fi
	    echo "libvirt version is ....."
	    libvirtd --version
	    sleep 2
	else	
		installLibvirtPackages
	fi
	# Touch them only if they are commented	
	sed -i 's/^#.*unix_sock_group.*/unix_sock_group="libvirtd"/g' /etc/libvirt/libvirtd.conf
	sed -i 's/^#.*unix_sock_rw_perms.*/unix_sock_rw_perms="0770"/g' /etc/libvirt/libvirtd.conf
	sed -i 's/^#.*unix_sock_ro_perms.*/unix_sock_ro_perms="0777"/g' /etc/libvirt/libvirtd.conf
	sed -i 's/^#.*auth_unix_ro.*/auth_unix_ro="none"/g' /etc/libvirt/libvirtd.conf
	sed -i 's/^#.*auth_unix_rw.*/auth_unix_rw="none"/g' /etc/libvirt/libvirtd.conf

	# Disable the apparmor profile for libvirt
	ln -s /etc/apparmor.d/usr.sbin.libvirtd /etc/apparmor.d/disable/
	apparmor_parser -R /etc/apparmor.d/usr.sbin.libvirtd 

}

function installRPProxyAndListner()
{
	echo "Installing RPProxy and Starting RPListner...."
	pkill -9 libvirtd
	
	echo "#! /bin/sh" > $KVM_BINARY
	echo "exec qemu-system-x86_64 -enable-kvm \"\$@\""  >> $KVM_BINARY
	chmod +x $KVM_BINARY
	# is_already_replaced=`strings /usr/bin/qemu-system-x86_64 | grep -c -t "rpcore"`
	if [ -e /usr/bin/qemu-system-x86_64_orig ]
	then	
		echo "RP-Proxy binary is already updated, might be old" 
	else
		echo "Backup of /usr/bin/qemu-system-x86_64 taken"
	    	cp /usr/bin/qemu-system-x86_64 /usr/bin/qemu-system-x86_64_orig
	fi
	cp "$INSTALL_DIR/rpcore/bin/debug/rp_proxy" /usr/bin/qemu-system-x86_64

	chmod +x /usr/bin/qemu-system-x86_64
	touch /var/log/rp_proxy.log
	chmod 666 /var/log/rp_proxy.log
	chown nova:nova /var/log/rp_proxy.log
	cp "$INSTALL_DIR/rpcore/bin/scripts/rppy_ifc.py" $DIST_LOCATION/.
	cp "$INSTALL_DIR/rpcore/lib/librpchannel-g.so" /usr/lib
	ldconfig
	cd "$INSTALL_DIR/rpcore/bin/debug/"
	nohup ./rp_listner > rp_listner.log 2>&1 &
	libvirtd -d
	cd "$INSTALL_DIR"

}

function startNonTPMRpCore()
{
	echo "Starting non-TPM RPCORE...."
	
	export RPCORE_IPADDR=$CURRENT_IP
	export RPCORE_PORT=16005
	export LD_LIBRARY_PATH="$INSTALL_DIR/rpcore/lib:$LD_LIBRARY_PATH"
	cp -r "$INSTALL_DIR/rpcore/rptmp" /tmp
	cp /tmp/rptmp/config/TrustedOS/privatekey /tmp/rptmp/config/TrustedOS/privatekey.pem
	cd "$INSTALL_DIR/rpcore/bin/debug"
	nohup ./nontpmrpcore > nontpmrpcore.log 2>&1 &
	NON_TPM="true"
	cd "$INSTALL_DIR"
}

function updateRCLocal()
{
	if [ ! -f /root/services.sh ]
	then
	        echo "opt=\$1" > /root/services.sh
	        echo "service nova-api-metadata \$opt" >> /root/services.sh
	        echo "service nova-network \$opt" >> /root/services.sh
	        echo "service nova-compute \$opt" >> /root/services.sh
	        chmod +x /root/services.sh
	fi	

	# remove the file if previously created
        if [ -f $RC_LOCAL_FILE ]
        then
            rm $RC_LOCAL_FILE
        fi
	if [ $NON_TPM == "true"  ] ; then
	        echo "#!/bin/sh -e
	        chown -R nova:nova /var/run/libvirt/
	        killall -9 libvirtd
	        sleep 2
	        ldconfig
		export RPCORE_IPADDR=$CURRENT_IP
		export RPCORE_PORT=16005
		export LD_LIBRARY_PATH=\"$INSTALL_DIR/rpcore/lib:$LD_LIBRARY_PATH\"
		cp -r \"$INSTALL_DIR/rpcore/rptmp\" /tmp
		cp /tmp/rptmp/config/TrustedOS/privatekey /tmp/rptmp/config/TrustedOS/privatekey.pem
		cd \"$INSTALL_DIR/rpcore/bin/debug\"
		nohup ./nontpmrpcore > nontpmrpcore.log 2>&1 &
	        nohup ./rp_listner > rp_listner.log 2>&1 &
	        libvirtd -d
	        sleep 1
	        /root/services.sh restart
	        exit 0" >> $RC_LOCAL_FILE
	else
                echo "#!/bin/sh -e
                chown -R nova:nova /var/run/libvirt/
                killall -9 libvirtd
                sleep 2
                ldconfig
                nohup ./rp_listner > rp_listner.log 2>&1 &
                libvirtd -d
                sleep 1
                /root/services.sh restart
                exit 0" >> $RC_LOCAL_FILE
	fi	
        chmod +x $RC_LOCAL_FILE
}

function patchOpenstackComputePkgs()
{
	cd "$OPENSTACK_DIR"
	chmod +x "$INSTALL_DIR/Openstack_applyPatches.sh"
	"$INSTALL_DIR/Openstack_applyPatches.sh" --compute
	cd "$INSTALL_DIR"
}

function validate()
{
	# Validate the following 
	# qemu-kvm is libvirt 1.2.2 is installed
	# nova-compute is installed
	# checks for xenbr0 interface

	# Validate qemu-kmv installation	
	if [ ! -e $KVM_BINARY ] ; then
		echo "ERROR : Could not find KVM installed on this machine"
		echo "Please install it using apt-get qemu-kvm"
		exit
	fi
	
	# Validate xenbr0 installation
	ip addr | grep -i -c xenbr0 > /dev/null 
        if [ $? -ne 0 ]; then
                echo "ERROR : xenbr0 device not available, please setup xenbr0 over this machine"
                exit
        fi
}

function main_default()
{
	echo "Please enter the install location (default : /opt/RP_<BUILD_TIMESTAMP> )"
	read INSTALL_DIR
	if [ -z "$INSTALL_DIR" ] 
	then 
		BUILD_TIMESTAMP=`ls KVM_*.tar.gz | awk 'BEGIN{FS="_"} {print $3}' | cut -c-14`
		INSTALL_DIR="$DEFAULT_INSTALL_DIR/RP_$BUILD_TIMESTAMP"
	fi
	mkdir -p "$INSTALL_DIR"
	while : ; do
		echo "Please enter current machine IP"
		read CURRENT_IP
		if valid_ip $CURRENT_IP; then break; else echo "Incorrect IP format : Please Enter Again"; fi
	done

	FLAVOUR=`getFlavour`
	updateFlavourVariables
        cd "$INSTALL_DIR"

        echo "Installing pre-requisites ..."
        installKVMPackages

	echo "Untarring Resources ..."
        untarResources

        echo "Installing libvirt ..."
        installLibvirt

	echo "Validating installation ... "
	validate
	#read

	echo "Do you wish to install nontpmrpcore y/n (default: n)"
	read INSTALL_NON_TPM
	if [ "$INSTALL_NON_TPM" == "y" ]
	then
		startNonTPMRpCore	
	fi
	#read
	echo "Installing RPProxy and RPListener..."
	installRPProxyAndListner
	#read
	
	#echo "Applying openstack patches..."	
	#patchOpenstackComputePkgs 

	updateRCLocal
	echo "Install completed successfully !"
}

function installNovaCompute()
{
	while : ; do
           echo "Please enter current machine IP"
           read CURRENT_IP
           if valid_ip $CURRENT_IP; then break; else echo "Incorrect IP format : Please Enter Again"; fi
        done
	echo "Installing Nova compute"
	apt-get install 

}

function configNovaCompute()
{
	echo "This step will configure your nova-compute"
	cd $OPENSTACK_DIR/..
	./controller-config.sh	
}

function help_display()
{
	echo "Usage : ./vRTM_KVM_install.sh [Options]"
        echo "This script creates the installer tar for RPCore"
        echo "    default : Installs RPCore components"
	echo "	  --with-libvirt : This searches and installs the libvirt version "
	echo "			 packaged along with vRTM dist"
	# This will be a separate script
#        echo "    --nova-compute : Installs and configures nova-compute over the node"
	exit
}

MY_SCRIPT_NAME=$0

if [ "$#" -gt 1 ] ; then
	help_display
fi

if [ "$1" == "--help" ] ; then
       help_display
elif [ "$1" == "--with-libvirt" ] ; then
	BUILD_LIBVIRT="TRUE"
	main_default
else
	echo "Installing RPCore components and applies patch for Openstack compute"
	main_default
fi


