#!/bin/bash

## This script is used for building libvirt
## It cam be used as a standalone script
## OR
## as a function module to be called via other script

## Note : Standalone module expectes the build-essential packages 
## to be installed

BUILD_DIR=${BUILD_DIR:-"."}
TIMEOUT=60

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

function installLibvirtRequiredPackages_rhel()
{
        echo "Installing Required Packages ....."
        yum update
        yum groupinstall -y "Development Tools" "Development Libraries"
        yum install -y "kernel-devel-uname-r == $(uname -r)"
        # Install the openstack repo
        yum install -y yum-plugin-priorities
        yum install -y https://repos.fedorapeople.org/repos/openstack/openstack-icehouse/rdo-release-icehouse-3.noarch.rpm

        yum install -y libvirt-devel libvirt libvirt-python
        #Libs required for compiling libvirt
        yum install -y gcc-c++ gcc make yajl-devel device-mapper-devel libpciaccess-devel libnl-devel libxml2-devel
        yum install -y python-devel
        yum install -y openssh-server
	yum install -y wget	
}


function installLibvirtRequiredPackages_ubuntu()
{
	apt-get -y install gcc libsdl1.2-dev zlib1g-dev libasound2-dev linux-kernel-headers pkg-config libgnutls-dev libpci-dev build-essential bzr bzr-builddeb cdbs debhelper devscripts dh-make diffutils dpatch fakeroot gnome-pkg-tools gnupg liburi-perl lintian patch patchutils pbuilder piuparts quilt ubuntu-dev-tools wget libglib2.0-dev libsdl1.2-dev libjpeg-dev libvde-dev libvdeplug2-dev libbrlapi-dev libaio-dev libfdt-dev texi2html texinfo info2man pod2pdf libnss3-dev libcap-dev libattr1-dev libtspi-dev gcc-4.6-multilib libpixman-1-dev libxml2-dev libssl-dev wget
	apt-get -y install libyajl-dev libdevmapper-dev libpciaccess-dev libnl-dev
	apt-get -y install bridge-utils dnsmasq pm-utils ebtables ntp chkconfig
        apt-get -y install openssh-server
        apt-get -y install python-dev
	apt-get -y install wget
}

function  installLibvirtRequiredPackages()
{
        if [ $FLAVOUR == "ubuntu" ] ; then
              installLibvirtRequiredPackages_ubuntu
        elif [ $FLAVOUR == "rhel" -o $FLAVOUR == "fedora" ] ; then
               installLibvirtRequiredPackages_rhel
        fi
}

function buildLibvirt()
{
        if [ ! -f libvirt-1.2.2.tar.gz ]
        then
                echo "Downloading libvirt-1.2.2"
                wget http://libvirt.org/sources/libvirt-1.2.2.tar.gz
                if [ $? -ne 0 ] ; then
                        echo "Libvirt download failed... Pl check outfile for errors"
                        exit
                fi
        fi

        if [ ! -f libvirt-1.2.2/src/.libs/libvirt.so.0.1002.2 ]
        then
                tar xf libvirt-1.2.2.tar.gz
                cd libvirt-1.2.2
		echo "Updating the timeout to $TIMEOUT"
		# Updating the timeout for libvirt 
		# TODO : use $TIMEOUT var in sed
		sed -i 's/int timeout =.*/int timeout = 60;/g' src/qemu/qemu_monitor.c
		# Package the updated libvirt
		cd ..
		tar czf libvirt-1.2.2.tar.gz libvirt-1.2.2
		cd -
                echo "Building libvirt - may take a few minutes"
                ./configure > $BUILD_DIR/outfile 2>&1
		if [ $? -ne 0 ] ; then
                        echo "Libvirt configuration failed ... Pl check outfile for errors"
                        exit
                fi
                make >> $BUILD_DIR/outfile 2>&1
                if [ $? -ne 0 ] ; then
                        echo "Libvirt build failed ... Pl check outfile for errors"
                        exit
                fi
                cd ..
        fi
        cp -a libvirt-1.2.2/include/libvirt rpcore/src/rpchannel
        # This dir is not created in the structure
        mkdir -p rpcore/lib
        cp -pL libvirt-1.2.2/src/.libs/libvirt.so.0.1002.2 rpcore/lib/libvirt.so
        cd $BUILD_DIR
}


function main()
{
	FLAVOUR=`getFlavour`
	echo "Installing libvirt required packages... This may take some time"
	installLibvirtRequiredPackages
	echo "Building libvirt packages ... This may also take some more time"
	buildLibvirt
}


function help()
{
	echo "This script builds libvirt 1.2.2 and places the respective libraries in RPCore build locations"
	echo "Usage : ./$0"
	
}

if [ $# -eq 0 ]; then
	main
else
	help
fi

