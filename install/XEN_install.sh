#!/bin/bash

function update_sources_list()
{
	cp /etc/apt/sources.list /etc/apt/sources.list.bak
	sed -i "/\bprecise\b/d" /etc/apt/sources.list
	sudo sed -i -e 's/[a-z][a-z].archive.ubuntu.com\|security.ubuntu.com/old-releases.ubuntu.com/g' /etc/apt/sources.list
	apt-get update
}

function create_local_repo()
{
        apt-get -y install dpkg-dev

	echo "Copying deb files ....."
	cd $Current_Dir
        if [ ! -d  supermin.d ]
        then
            mkdir supermin.d
        fi
        echo "Extracting............................................................"
        tar xvf archives.tar.gz
        dpkg-scanpackages  archives | gzip -9c > archives/Packages.gz

        mmm=$(echo $Current_Dir | sed 's/\//\\\//g')
	# Delete the line if there in case this script was run before
	sed -i "/deb file:/d" /etc/apt/sources.list
        sed -i "1s/^/deb file:\/\/$mmm\/ archives\/\n/" /etc/apt/sources.list

        apt-get update
}

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

function UpdatePlugins()
{
   rm -rf /usr/lib/xcp/plugins/*.pyc
   cd $current_dir/patched_files
   rm -f /tmp/patched_files
   touch /tmp/patched_files
   find . -type f > /tmp/patched_files
   for i in `cat /tmp/patched_files`;do mv ${i:1} ${i:1}.bak;done
   for i in `cat /tmp/patched_files`;do cp $i ${i:1};done
   chmod +x /usr/lib/xcp/plugins/*
}

function set_keyvaluepair()
{
    TARGET_KEY=$1
    REPLACEMENT_VALUE=$2
    CONFIG_FILE=$3
    sed  -i "s|\($TARGET_KEY *= *\).*|\1$REPLACEMENT_VALUE|" $CONFIG_FILE
}

function set_keyvaluepair_xen()
{
    TARGET_KEY=$1
    REPLACEMENT_VALUE=$2
    CONFIG_FILE=$3
    sed -i "s/\"$TARGET_KEY\" : .*/\"$TARGET_KEY\" : \"$REPLACEMENT_VALUE\",/" $CONFIG_FILE
}

function usage()
{
  echo "Usage: In a root shell in the directory containing the .tar.gz file created by XEN_build.sh: ./XEN_install.sh"
  echo "options: --help"
  exit
}

arg1=x$1
if [ $arg1 = "x--help" ]
then
  usage
fi

while : ; do
        echo "Enter Openstack XEN Dom0 Compute IP"
        read compute_ip
        if valid_ip $compute_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

while : ; do
        echo "Enter gateway IP for Openstack XEN Dom0 node"
        read gateway_ip
        if valid_ip $gateway_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

while : ; do
        echo "Enter dns-nameserver IP for Openstack XEN Dom0 node"
        read dns_ip
        if valid_ip $dns_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

# setup xcp-xapi section

echo Uninstalling virtualbox to make sure other installs are successful
apt-get -f -y install

tarfile=$(ls -1dtr XEN_install*.tar.gz | tail -1)
tarfiletest=x$tarfile
if [ $tarfiletest = "x" ]
then
	echo "Cannot find the .tar.gz file created by XEN_build.sh"
	exit
fi

tarfile=$(pwd)/$tarfile
Base_Dir=/opt/RP_$(echo $tarfile | sed 's/.*XEN_install_//
s/\..*//')
INSTALL_DIR=$Base_Dir
mkdir -p $INSTALL_DIR
cd $INSTALL_DIR
tar xf $tarfile
Current_Dir=$Base_Dir/Setup-Standardization/Dom0
cd $Current_Dir

update_sources_list
create_local_repo
apt-get --allow-unauthenticated -y install xcp-xapi
set_keyvaluepair TOOLSTACK xapi /etc/default/xen
update-rc.d xendomains disable
mkdir /usr/share/qemu
ln -s /usr/share/qemu-linaro/keymaps /usr/share/qemu/keymaps
set_keyvaluepair GRUB_DEFAULT "\"Ubuntu GNU/Linux, with Xen hypervisor\"" /etc/default/grub 
set_keyvaluepair GRUB_CMDLINE_LINUX "\"biosdevname=0\"" /etc/default/grub 
update-grub
mkdir -p /etc/xcp
rm /etc/xcp/network.conf
touch /etc/xcp/network.conf
echo "bridge" > /etc/xcp/network.conf

apt-get --allow-unauthenticated -y install bc g++ make python-dev tpm-tools tpm-tools-dbg trousers libtspi1 libtspi-dev tboot libxml2-dev libssl-dev cryptsetup-bin
update-grub

found=$(grep -c xenbr0 /etc/network/interfaces)

if [ $found = "0" ]
then
echo Updating /etc/network/interfaces
rm /etc/network/interfaces

echo "# This file describes the network interfaces available on your system
# and how to activate them. For more information, see interfaces(5).

# The loopback network interface
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet manual

auto xenbr0
iface xenbr0 inet static
bridge_ports eth0
	address $compute_ip
	netmask 255.255.255.0
	gateway $gateway_ip
	dns-nameservers $dns_ip" > /etc/network/interfaces

echo Restart networking
/etc/init.d/networking restart
fi

kversion=$(uname -r | sed 's/\(......\).*/\1/')
if [ ! $kversion = "3.11.0" ]
then
    #Upgrade the kernel section
    a=`uname -r`
    apt-get --allow-unauthenticated -y install "linux-headers-$a"
    apt-get --allow-unauthenticated -y install blktap-dkms
    dpkg-reconfigure blktap-dkms
    found=$(grep -c precise /etc/apt/sources.list)

    if [ $found = "0" ]
    then
	cp /etc/apt/sources.list /etc/apt/sources.list.bak 
	echo "deb http://security.ubuntu.com/ubuntu precise-security main" >> /etc/apt/sources.list
	apt-get update
    fi
    apt-get --allow-unauthenticated -y install python-apport
    apt-get --allow-unauthenticated -y install linux-headers-3.11.0-26-generic linux-image-3.11.0-26-generic 
    dpkg-reconfigure blktap-dkms
fi

# dom0 config section
current_dir=$Current_Dir

echo "Copying MH Agent to /usr/local/bin/mhagent"
mv /usr/local/bin/mhagent /usr/local/bin/mhagent.bak 2> /dev/null
cd $current_dir
cp mhagent /usr/local/bin/mhagent
chmod +x /usr/local/bin/mhagent

echo Update tcconfig.xml
sed -i -e s?\\\(\<directory\>\\\).*\\\(\<.directory\>\\\)?\\\1$Base_Dir/rpcore/rptmp/config\\\2? -e s?\\\(\<signing_server_ip\>\\\).*\\\(\<.signing_server_ip\>\\\)?\\\1$compute_ip\\\2? -e s?\\\(\<rpcore_ip\>\\\).*\\\(\<.rpcore_ip\>\\\)?\\\1$compute_ip\\\2? $Base_Dir/rpcore/rptmp/config/tcconfig.xml
# Set the RP_CONFIG environment variable
export RP_CONFIG=$Base_Dir/rpcore/rptmp

echo "Installing virtualbox-fuse and qemu-utils for mounting purpose"
cd $current_dir
apt-get --allow-unauthenticated -y install qemu-utils
if [ $? ];
then
        echo "################## Installed qemu-utils"
else
        echo "################### Error in installation of qemu-utils"
fi
apt-get --allow-unauthenticated -y install kpartx
if [ $? ];
then
        echo "################## Installed kpartx"
else
        echo "################### Error in installation of kpartx"
fi

echo Installing virtualbox last because --force-depends creates an error for later installs.
apt-get --allow-unauthenticated -y install virtualbox
dpkg -i --force-depends virtualbox-fuse_4.1.18-dfsg-1ubuntu1_amd64.deb

echo "Updating Plugins at /usr/lib/xcp/plugins"
UpdatePlugins

mkdir -p $INSTALL_DIR

echo "Copying mount script to $INSTALL_DIR/scripts/"
cd $current_dir
mkdir -p $INSTALL_DIR/scripts
cp mount_vm_image.sh $INSTALL_DIR/scripts/

echo "Copying sr restore script to $INSTALL_DIR"
cp setup_encrypted_sr_after_reboot.sh $INSTALL_DIR

dom0_ip=$compute_ip
export RPCORE_IPADDR=$dom0_ip
export RPCORE_PORT=16005
export IP_ADDRESS=$dom0_ip

cd $current_dir
cp intel_rpcore_plugin.conf /etc/

set_keyvaluepair_xen RPCORE_PORT  $RPCORE_PORT /etc/intel_rpcore_plugin.conf
set_keyvaluepair_xen RPCORE_IPADDR $RPCORE_IPADDR /etc/intel_rpcore_plugin.conf
cp -r xapi-access-control-proxy $INSTALL_DIR
cd $INSTALL_DIR/xapi-access-control-proxy
chmod a+x xapi-access-control.py

echo Setting root password
passwd root << END
intelrp
intelrp
END

echo Restart machine and run XEN_install_after_reboot.sh script
sleep 2
echo

