#!/bin/bash


# The script does the following
# 1. Creates the directory structure ../dist, kvm_build
# 2. Copies all the resources to kvm_build
# 3. Builds libvirt, rpcore
# 4. Creates a tar for the final install
# 5. Copies the tar and install scripts to dist

START_DIR=$PWD
SRC_ROOT_DIR=$START_DIR/../
BUILD_DIR=$START_DIR/KVM_build
DIST_DIR=$PWD/../dist
TBOOT_REPO=${TBOOT_REPO:-"$SRC_ROOT_DIR/../dcg_security-tboot-xm"}

BUILD_LIBVIRT="FALSE"
PACKAGE="rpcore/scripts rpcore/bin rpcore/rptmp rpcore/lib"

# This function returns either rhel fedora ubuntu suse
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
	grep -c -i suse /etc/*-release > /dev/null
	if [ $? -eq 0 ] ; then
		flavour="suse"
	fi 
        if [ "$flavour" == "" ] ; then  
                echo "Unsupported linux flavor, Supported versions are ubuntu, rhel, fedora"
                exit
        else
                echo $flavour
        fi
}

function makeDirStructure()
{
	if [ -d "$SRC_ROOT_DIR/rpcore" -a -d "$SRC_ROOT_DIR/rpclient" -a -d "$SRC_ROOT_DIR/docs" -a -d "$SRC_ROOT_DIR/install" -a -d "$TBOOT_REPO/imvm" ] 
	then
		echo "All resources found"
	else
		echo "One or more of the required dirs absent please check the following dir existence :"
		echo "1. rpcore, rpclient, docs and install at $SRC_ROOT_DIR"
		echo "2. Measurement Agent (imvm) is present at $TBOOT_REPO/imvm"
		exit
	fi
	mkdir -p "$BUILD_DIR"
	cp -r "$SRC_ROOT_DIR/rpcore" "$SRC_ROOT_DIR/rpclient" "$SRC_ROOT_DIR/docs" "$SRC_ROOT_DIR/install" "$BUILD_DIR"
}

function buildVerifier()
{
    cd $TBOOT_REPO/imvm/src
    make -f verifier-g.mak clean >> $BUILD_DIR/outfile 2>&1
    if [ $? -ne 0 ]; then
        echo "Verifier clean failed...Please see outfile for more details"
        exit
	else
		echo "Verifier clean successful"
    fi
	make -f verifier-g.mak >> "$BUILD_DIR/outfile" 2>&1
    if [ $? -ne 0 ]; then
        echo "Verifier build failed...Please see outfile for more details"
        exit
    else
        echo "Verifier build successful"
    fi
	cp ../bin/verifier "$BUILD_DIR/rpcore/bin/debug/."
	cd "$BUILD_DIR"
}

function buildRplistener()
{
	cd rpcore/src/rpproxy/kvm_proxy
   	make -f rp-proxy.mak clean >> "$BUILD_DIR/outfile" 2>&1
	if [ $? -ne 0 ]; then
        echo "RP-Proxy clean failed...Please see outfile for more details"
        exit
    else
        echo "RPProxy clean successful"
	fi

        make -f rp-proxy.mak >> "$BUILD_DIR/outfile" 2>&1
	if [ $? -ne 0 ]; then
                echo "RP-Proxy build failed...Please see outfile for more details"
                exit
        else
                echo "RPProxy build successful"
        fi
	
        cd "$BUILD_DIR"
}

function buildRpcore()
{
    cd "$BUILD_DIR/rpcore"
	#mkdir -p bin/debug bin/release build/debug build/release lib
	cd src

    if [ "$BUILD_LIBVIRT" == "TRUE" ] ; then
        echo "Backing up newly created libvirt.so file"
        cp ../lib/libvirt.so "$BUILD_DIR/."
    fi
    echo > "$BUILD_DIR/outfile"
    make clean >> "$BUILD_DIR/outfile" 2>&1
    if [ $? -ne 0 ]; then
        echo "RPcore clean failed...Please see outfile for more details"
        exit
    else
        echo "RPCore clean successful"
    fi

   if [ "$BUILD_LIBVIRT" == "TRUE" ] ; then
        echo "Restoring libvirt.so..."
        mv "$BUILD_DIR/libvirt.so" ../lib/libvirt.so
   fi
   
    PYTHON_HEADERS=""
    if [ -e /usr/include/python2.7 ]
    then
        PYTHON_HEADERS="PY=/usr/include/python2.7 PYTHON=python2.7"
    elif [ -e /usr/include/python2.6 ]
    then
        PYTHON_HEADERS="PY=/usr/include/python2.6 PYTHON=python2.6"
    fi
    make $PYTHON_HEADERS >> "$BUILD_DIR/outfile" 2>&1
	if [ $? -ne 0 ]; then
		echo "RPcore build failed...Please see outfile for more details"
		exit
	else
        echo "RPCore build successful"
	fi
    cd "$BUILD_DIR"
}


function install_kvm_packages_rhel()
{
	echo "Installing Required Packages ....."
	yum -y update
	yum -y groupinstall -y "Development Tools" "Development Libraries"
	yum install -y "kernel-devel-uname-r == $(uname -r)" 
	# Install the openstack repo
	yum install -y yum-plugin-priorities
	yum install -y https://repos.fedorapeople.org/repos/openstack/openstack-icehouse/rdo-release-icehouse-3.noarch.rpm

	yum install -y libvirt-devel libvirt libvirt-python libxml2
	#Libs required for compiling libvirt 
	yum install -y gcc-c++ gcc make yajl-devel device-mapper-devel libpciaccess-devel libnl-devel libxml2-devel trousers-devel openssl-devel
	yum install -y python-devel
	yum install -y openssh-server
	yum install -y trousers trousers-devel libaio libaio-devel 
	yum install -y tar dos2unix
}

function install_kvm_packages_ubuntu()
{
	echo "Installing Required Packages for ubuntu ....."
	apt-get -y  install python-software-properties
	add-apt-repository -y cloud-archive:icehouse
	apt-get -y update
	apt-get -y dist-upgrade
	apt-get -y install gcc libsdl1.2-dev zlib1g-dev libasound2-dev linux-kernel-headers pkg-config libgnutls-dev libpci-dev build-essential bzr bzr-builddeb cdbs debhelper devscripts dh-make diffutils dpatch fakeroot gnome-pkg-tools gnupg liburi-perl lintian patch patchutils pbuilder piuparts quilt ubuntu-dev-tools wget libglib2.0-dev libsdl1.2-dev libjpeg-dev libvde-dev libvdeplug2-dev libbrlapi-dev libaio-dev libfdt-dev texi2html texinfo info2man pod2pdf libnss3-dev libcap-dev libattr1-dev libtspi-dev gcc-4.6-multilib libpixman-1-dev libxml2-dev libssl-dev ant
	apt-get -y install libvirt-bin libvirt-dev qemu-kvm
	apt-get -y install libyajl-dev libdevmapper-dev libpciaccess-dev libnl-dev
	apt-get -y install bridge-utils dnsmasq pm-utils ebtables ntp
	apt-get -y install openssh-server
	apt-get -y install python-dev dos2unix
	
	echo "Starting ntp service ....."
	service ntp start
	chkconfig ntp on

}

function install_kvm_packages_suse()
{
	echo "Installing Required Packages for sue ....."
	zypper -n in make gcc gcc-c++ libxml2-devel libopenssl-devel pkg-config libgnutls-devel bzr debhelper devscripts dh-make diffutils perl-URI patch patchutils pbuilder quilt wget glib2-devel libjpeg8-devel libvdemgmt0-devel libvdeplug3-devel brlapi-devel libaio-devel libfdt1-devel texinfo libcap-devel libattr-devel libtspi1 libpixman-1-0-devel trousers-devel  ant
	zypper -n in libvirt libvirt-devel qemu-kvm
	zypper -n in libyajl-devel libpciaccess-devel libnl3-devel
	zypper -n in bridge-utils dnsmasq pm-utils ebtables ntp
	zypper -n in openssh
	zypper -n in python-devel dos2unix

	echo "Starting ntp service ....."
	service ntp start
	chkconfig ntp on 
}

function install_kvm_packages()
{
	if [ $FLAVOUR == "ubuntu" ] ; then
		install_kvm_packages_ubuntu
	elif [  $FLAVOUR == "rhel" -o $FLAVOUR == "fedora" ] ; then
		install_kvm_packages_rhel
	elif [ $FLAVOUR == "suse" ] ; then
		install_kvm_packages_suse	
	fi
}

function main()
{
	FLAVOUR=`getFlavour`
	echo "Installing the packages required for KVM... "	
	install_kvm_packages

        echo "Creating the required build structure... "
        makeDirStructure

	if [ "$BUILD_LIBVIRT" == "TRUE" ] ; then
		echo "Building libvirt and its dependencies..."
		cd "$BUILD_DIR"
		cp $START_DIR/vRTM_libvirt_build.sh .
		chmod +x ./vRTM_libvirt_build.sh
		dos2unix ./vRTM_libvirt_build.sh
		./vRTM_libvirt_build.sh
		echo "Inclding modified libvirt-1.2.2.tar.gz into dist package"
		PACKAGE=`echo $PACKAGE libvirt-1.2.2.tar.gz`
	fi

	echo "Building RPCore binaries... "
        buildRpcore
	cd "$BUILD_DIR"
	echo "Building Verifier binaries..."
	buildVerifier
	echo "Building RPListener binaries..."
	buildRplistener

	BUILD_VER=`date +%Y%m%d%H%M%S`
	grep -i ' error' "$BUILD_DIR/outfile"
	
	echo "Verifying for any errors, please verify the output below : "
	arg=`cat "$BUILD_DIR/outfile" | grep -i -v "print*" | grep -c ' error'`

	echo "Create the install tar file"
        mkdir -p "$DIST_DIR"
        cd "$BUILD_DIR"
        tar czf KVM_install_$BUILD_VER.tar.gz $PACKAGE
        mv KVM_install_$BUILD_VER.tar.gz "$DIST_DIR"
        cp install/vRTM_KVM_install.sh "$DIST_DIR"
	dos2unix "$DIST_DIR/vRTM_KVM_install.sh"
        
        arg=`cat "$BUILD_DIR/outfile" | grep -i -v "print*" | grep -c ' error'`

	if [ $arg -eq 0 ]
	then
	    echo "Build KVM_install_$BUILD_VER.tar.gz created successfully !!"
	else
	    echo "Verifying for any errors, please verify the output below : "
	    echo Install tar file has been created but it might might has some errors. Please see $BUILD_DIR/outfile file.
	fi

}

function help_display()
{
    echo "Usage: ./vRTM_KVM_build.sh [Options]"
    echo ""
    echo "This script does the following"
    echo "1. Creates the directory structure ../dist, ./kvm_build"
    echo "2. Copies all the resources to kvm_build"
    echo "3. Builds vRTM components"
    echo "4. Creates a tar for the final install in ../dist folder"
    echo ""
    echo "If --with-libvirt option is used, script will download libvirt 1.2.2 and use"
    echo "the downloaded version to compile the vRTM components"
    echo "Otherwise, it will use libvirt installed at default location."
    echo ""
    echo "Following options are available:"
    echo "--build"
    echo "--with-libvirt"
    echo "--clean"
    echo "--help"
}

if [ $# -gt 1 ] 
then	
	echo "ERROR: Extra arguments"
	help_display
fi

if [ $# -eq 0 ] || [ "$1" == "--build" ]
then
	echo "Building vRTM"
	main
elif [ "$1" == "--with-libvirt" ]
then
	echo "Building libvirt and vRTM"
	export BUILD_LIBVIRT="TRUE"
	main
	
elif [ "$1" == "--clean" ]
then
	echo "Removing older build folder"
	rm -rf "$BUILD_DIR"
	rm -rf "$DIST_DIR"
else 
	help_display
fi
