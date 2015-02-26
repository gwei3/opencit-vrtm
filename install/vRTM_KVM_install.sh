#!/bin/bash

#This script installs RPCore, RPProxy, RPListner, and openstack patches


RES_DIR=$PWD
DEFAULT_INSTALL_DIR=/opt

OPENSTACK_DIR="Openstack/patch"

LINUX_FLAVOUR="ubuntu"
NON_TPM="false"


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

function installKVMPackages()
{
	echo "Installing Required Packages ....."
	# apt-get -y install gcc libsdl1.2-dev zlib1g-dev libasound2-dev linux-kernel-headers pkg-config libgnutls-dev libpci-dev build-essential bzr bzr-builddeb cdbs debhelper devscripts dh-make diffutils dpatch fakeroot gnome-pkg-tools gnupg liburi-perl lintian patch patchutils pbuilder piuparts quilt ubuntu-dev-tools wget libglib2.0-dev libsdl1.2-dev libjpeg-dev libvde-dev libvdeplug2-dev libbrlapi-dev libaio-dev libfdt-dev texi2html texinfo info2man pod2pdf libnss3-dev libcap-dev libattr1-dev libtspi-dev gcc-4.6-multilib libpixman-1-dev libxml2-dev libssl-dev wget git
	# apt-get -y install libyajl-dev libdevmapper-dev libpciaccess-dev libnl-dev
	apt-get -y install bridge-utils dnsmasq pm-utils ebtables ntp chkconfig guestfish
	apt-get -y install openssh-server
	apt-get -y install python-dev
	apt-get -y install tboot trousers tpm-tools libtspi-dev cryptsetup-bin
	apt-get -y install qemu-utils kpartx
	apt-get -y install iptables libblkid1 libc6 libcap-ng0 libdevmapper1.02.1 libgnutls26 libnl-3-200 libnuma1  libparted0debian1  libpcap0.8 libpciaccess0 libreadline6  libudev0  libxml2  libyajl1
	
	echo "Starting ntp service ....."
	service ntp start
	chkconfig ntp on
}

function installLibvirt()
{
	# Openstack icehouse repo contains libvirt 1.2.2
	# Adding the repository

	apt-get -y install python-software-properties
	add-apt-repository -y cloud-archive:icehouse

	echo "Updating repositories .. this may take a while "
	apt-get update > /dev/null
	if [ $? -ne 0 ] ; then
		echo "apt-get update failed, kindly resume after manually executing apt-get update"
	fi
	apt-get -y install libvirt-bin libvirt-dev libvirt0 python-libvirt
	
	# Touch them only if they are commented	
	sed -i 's/^#.*unix_sock_group.*/unix_sock_group="libvirtd"/g' /etc/libvirt/libvirtd.conf
	sed -i 's/^#.*unix_sock_rw_perms.*/unix_sock_rw_perms="0770"/g' /etc/libvirt/libvirtd.conf
	sed -i 's/^#.*unix_sock_ro_perms.*/unix_sock_ro_perms="0777"/g' /etc/libvirt/libvirtd.conf
	sed -i 's/^#.*auth_unix_ro.*/auth_unix_ro="none"/g' /etc/libvirt/libvirtd.conf
	sed -i 's/^#.*auth_unix_rw.*/auth_unix_rw="none"/g' /etc/libvirt/libvirtd.conf

}

function installRPProxyAndListner()
{
	echo "Installing RPProxy and Starting RPListner...."
	killall -9 libvirtd
	
	echo "#! /bin/sh" > /usr/bin/kvm
	echo "exec qemu-system-x86_64 -enable-kvm \"\$@\""  >> /usr/bin/kvm
	chmod +x /usr/bin/kvm
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
	cp "$INSTALL_DIR/rpcore/bin/scripts/rppy_ifc.py" /usr/lib/python2.7/
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
        if [ -f /etc/rc.local ]
        then
            rm /etc/rc.local
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
		cd \"$INSTALL_DIR/rpcore/bin/debug\"
		nohup ./nontpmrpcore > nontpmrpcore.log 2>&1 &
	        nohup ./rp_listner > rp_listner.log 2>&1 &
	        libvirtd -d
	        sleep 1
	        /root/services.sh restart
	        exit 0" >> /etc/rc.local
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
                exit 0" >> /etc/rc.local
	fi	
        chmod +x /etc/rc.local
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
	if [ ! -e /usr/bin/kvm ] ; then
		echo "ERROR : Could not find KVM installed on this machine"
		echo "Please install it using apt-get qemu-kvm"
		exit
	fi
	
	# Validate xenbr0 installation
	ifconfig xenbr0 > /dev/null 
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

        cd "$INSTALL_DIR"
        echo "Installing libvirt ..."
        installLibvirt

	echo "Installing pre-requisites ..."
	installKVMPackages

	echo "Untarring Resources ..."
	untarResources
	#read

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
	# This will be a separate script
#        echo "    --nova-compute : Installs and configures nova-compute over the node"
	exit
}

MY_SCRIPT_NAME=$0

if [ "$#" -gt 0 ] ; then
	help_display
fi

if [ "$1" == "--help" ] ; then
       help_display
else
	echo "Installing RPCore components and applies patch for Openstack compute"
	main_default
fi


