
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

function set_keyvaluepair()
{
	TARGET_KEY=$1
	REPLACEMENT_VALUE=$2
	CONFIG_FILE=$3
	sed  -i "s|\($TARGET_KEY *= *\).*|\1$REPLACEMENT_VALUE|" $CONFIG_FILE
}

function configure_kvm()
{
cd $Current_Dir

echo "Applying the kvm configuration ....."
set_keyvaluepair metadata_host $controller_ip $Current_Dir/conf_files/nova.conf
set_keyvaluepair rabbit_host  $controller_ip $Current_Dir/conf_files/nova.conf
set_keyvaluepair my_ip $compute_ip $Current_Dir/conf_files/nova.conf
set_keyvaluepair vncserver_proxyclient_address $compute_ip $Current_Dir/conf_files/nova.conf
set_keyvaluepair novncproxy_base_url "http://$controller_ip:6080/vnc_auto.html" $Current_Dir/conf_files/nova.conf
set_keyvaluepair glance_host $controller_ip $Current_Dir/conf_files/nova.conf
set_keyvaluepair glance_api_servers  "$controller_ip:9292" $Current_Dir/conf_files/nova.conf
set_keyvaluepair nova_url "http://$controller_ip:8774/v1.1/" $Current_Dir/conf_files/nova.conf
set_keyvaluepair network_host $compute_ip $Current_Dir/conf_files/nova.conf
set_keyvaluepair connection "mysql://nova:intelrp@$controller_ip/nova" $Current_Dir/conf_files/nova.conf
set_keyvaluepair auth_host $controller_ip  $Current_Dir/conf_files/api-paste.ini
set_keyvaluepair flat_interface $flat_interface $Current_Dir/conf_files/nova.conf
set_keyvaluepair public_interface $public_interface $Current_Dir/conf_files/nova.conf

# Delete the line if previously added
sed -i "/net.ipv4.ip_forward=/d" /etc/sysctl.conf
echo "net.ipv4.ip_forward=1" >> /etc/sysctl.conf
sysctl -p

dpkg-statoverride  --update --add root root 0644 /boot/vmlinuz-$(uname -r)

cp $Current_Dir/conf_files/statoverride /etc/kernel/postinst.d/statoverride
chmod +x /etc/kernel/postinst.d/statoverride

cp $Current_Dir/conf_files/libvirtmod.so /usr/lib/python2.7/dist-packages/

rm /var/lib/nova/nova.sqlite

if [ ! -d  /etc/nova ]
then
    mkdir /etc/nova
fi
mv /etc/nova/nova.conf /etc/nova/nova.conf.bak
mv /etc/nova/nova-compute.conf /etc/nova/nova-compute.conf.bak
mv /etc/nova/api-paste.ini /etc/nova/api-paste.ini.bak
cp $Current_Dir/conf_files/nova.conf /etc/nova/nova.conf
cp $Current_Dir/conf_files/nova-compute.conf /etc/nova/nova-compute.conf
cp $Current_Dir/conf_files/api-paste.ini /etc/nova/api-paste.ini

groupadd nova
usermod -g nova nova
chown -R nova:nova /etc/nova
chmod 640 /etc/nova/nova.conf

# remove the file if previously created
if [ -f /etc/rc.local ]
then
    rm /etc/rc.local
fi
echo "#!/bin/sh -e
chown -R nova:nova /var/run/libvirt/
export RPCORE_IPADDR=$compute_ip
export RPCORE_PORT=16005
export LD_LIBRARY_PATH=$INSTALL_DIR/rpcore/lib:$LD_LIBRARY_PATH
cp -r $INSTALL_DIR/rpcore/rptmp /tmp
cd $INSTALL_DIR/rpcore/bin/debug
nohup ./nontpmrpcore &
killall -9 libvirtd
sleep 2
ldconfig
nohup ./rp_listner &
libvirtd -d
sleep 1
chown -R nova:nova /var/run/libvirt/
chmod 777 /var/run/libvirt/libvirt-sock
/root/services.sh restart
exit 0" >> /etc/rc.local
chmod +x /etc/rc.local

# kill libvirtd, it will start automatically
killall -9 libvirtd
sleep 2

if [ ! -d  /usr/share/pyshared/nova/virt/libvirt ]
then
    mkdir -p /usr/share/pyshared/nova/virt/libvirt
fi
chown -R nova:nova /var/run/libvirt/
mv /usr/share/pyshared/nova/virt/libvirt/driver.py /usr/share/pyshared/nova/virt/libvirt/driver.py.bak
mv /usr/share/pyshared/nova/virt/libvirt/utils.py /usr/share/pyshared/nova/virt/libvirt/utils.py.bak
cp $Current_Dir/conf_files/driver.py /usr/share/pyshared/nova/virt/libvirt/driver.py
cp $Current_Dir/conf_files/utils.py /usr/share/pyshared/nova/virt/libvirt/utils.py
cp $Current_Dir/conf_files/mhagent /usr/local/bin/mhagent
chmod +x /usr/local/bin/mhagent
rm -f /usr/lib/python2.7/dist-packages/nova/virt/libvirt/driver.pyc
rm -f /usr/lib/python2.7/dist-packages/nova/virt/libvirt/utils.pyc
chown nova:nova /usr/local/bin/mhagent
rm -f /var/log/mhagent.log
touch /var/log/mhagent.log
chown nova:nova  /var/log/mhagent.log
rm /var/lib/nova/instances/_base/*

# kill any running guest
killall -9 /usr/bin/qemu-system-x86_64_orig
killall -9 /usr/bin/qemu-system-x86_64

echo Copy qemu-system-x86_64 to qemu-system-x86_64_orig
# Rename to qemu-system-x86_64_orig
# First check if qemu has already been built
if [ -d /usr/share/qemu/keymaps ]
then
    # Check if rp_proxy already copied to qemu-system-x86_64 from previously run script
    found=$(grep -c rp_proxy /usr/bin/qemu-system-x86_64)

    if [ ! $found = "0" ]
    then
	if [ -f /usr/bin/qemu-system-x86_64_bak ]
	then
	    cp /usr/bin/qemu-system-x86_64_bak /usr/bin/qemu-system-x86_64_orig
	else
	    echo Please run this script again with the --installpkgs switch.
	    echo Unable to continue.
	    exit
	fi
    else
	# make install for qemu is done and rp_proxy not copied to qemu-system-x86_64 yet
	cp /usr/bin/qemu-system-x86_64 /usr/bin/qemu-system-x86_64_bak
	mv /usr/bin/qemu-system-x86_64 /usr/bin/qemu-system-x86_64_orig
    fi
else
    echo Please run this script again with the --installpkgs switch.
    echo Unable to continue.
    exit
fi

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
	address 192.168.1.14
	netmask 255.255.255.0
	gateway $gateway_ip
	dns-nameservers $dns_ip" > /etc/network/interfaces
sed -i -e s/192.168.1.14/$compute_ip/ /etc/network/interfaces

echo Restart networking
/etc/init.d/networking restart
fi
}

function start_rpcore()
{
echo "Starting RPCORE...."
export RPCORE_IPADDR=$compute_ip
export RPCORE_PORT=16005
found=`env | grep -c 'LD_LIBRARY_PATH.*rpcore.lib'`

if [ $found = "0" ]
then
  export LD_LIBRARY_PATH=$INSTALL_DIR/rpcore/lib:$LD_LIBRARY_PATH
fi
cp -r $INSTALL_DIR/rpcore/rptmp /tmp
cd $INSTALL_DIR/rpcore/bin/debug
nohup ./nontpmrpcore &
}

function start_rplistener()
{
echo "Starting RPLISTENER...."
killall -9 libvirtd
sleep 2

# libvirt will call kvm or qemu-system-x86_64 (actually rp_proxy)
# ( We want rp_proxy to intercept libvirt call to qemu-system-x86_64 )
echo "#! /bin/sh" > /usr/bin/kvm
echo "exec qemu-system-x86_64 -enable-kvm \"\$@\""  >> /usr/bin/kvm
chmod +x /usr/bin/kvm
cp $INSTALL_DIR/rpcore/bin/debug/rp_proxy  /usr/bin/qemu-system-x86_64
chmod +x /usr/bin/qemu-system-x86_64
touch /var/log/rp_proxy.log
chmod 666 /var/log/rp_proxy.log
chown nova:nova /var/log/rp_proxy.log
cp $INSTALL_DIR/rpcore/bin/scripts/rppy_ifc.py /usr/lib/python2.7/
cp $INSTALL_DIR/rpcore/lib/librpchannel-g.so /usr/lib
ldconfig
cd $INSTALL_DIR/rpcore/bin/debug
nohup ./rp_listner &
libvirtd -d

chown -R nova:nova /var/run/libvirt/
}

function create_local_repo()
{
apt-get update

apt-get --force-yes -y install dpkg-dev

echo "Copying deb files ....."
cd $Current_Dir
if [ ! -d  supermin.d ]
then
    mkdir supermin.d
fi
tar zxf archives.tar.gz
dpkg-scanpackages  archives | gzip -9c > archives/Packages.gz

mmm=$(echo $Current_Dir | sed 's/\//\\\//g')
# Delete the line if there in case this script was run before
sed -i "/deb file:/d" /etc/apt/sources.list
sed -i "1s/^/deb file:\/\/$mmm\/ archives\/\n/" /etc/apt/sources.list
}

function install_kvm_packages()
{
echo "Installing Required Packages ....."
apt-get update
apt-get --force-yes -y install gcc libsdl1.2-dev zlib1g-dev libasound2-dev linux-kernel-headers pkg-config libgnutls-dev libpci-dev build-essential bzr bzr-builddeb cdbs debhelper devscripts dh-make diffutils dpatch fakeroot gnome-pkg-tools gnupg liburi-perl lintian patch patchutils pbuilder piuparts quilt ubuntu-dev-tools wget libglib2.0-dev libsdl1.2-dev libjpeg-dev libvde-dev libvdeplug2-dev libbrlapi-dev libaio-dev libfdt-dev texi2html texinfo info2man pod2pdf libnss3-dev libcap-dev libattr1-dev libtspi-dev gcc-4.6-multilib libpixman-1-dev libxml2-dev libssl-dev git

apt-get --force-yes -y install libyajl-dev libdevmapper-dev libpciaccess-dev libnl-dev
apt-get --force-yes -y install bridge-utils dnsmasq pm-utils ebtables ntp chkconfig guestfish
apt-get --force-yes -y install python-mysqldb=1.2.3-1ubuntu0.1
apt-get --force-yes -y install nova-compute-kvm=1:2013.2.2-0ubuntu1~cloud0 nova-compute=1:2013.2.2-0ubuntu1~cloud0 python-guestfs=1:1.14.8-1
apt-get --force-yes -y install nova-network=1:2013.2.2-0ubuntu1~cloud0 nova-api-metadata=1:2013.2.2-0ubuntu1~cloud0 python-guestfs=1:1.14.8-1
apt-get --force-yes -y install openssh-server
apt-get --force-yes -y install python-dev
apt-get --force-yes -y install tboot trousers tpm-tools libtspi-dev cryptsetup-bin

echo "Starting ntp service ....."
service ntp start
chkconfig ntp on

}

function make_kvm()
{
echo "Verifying the kvm kernel module ....."
lsmod | grep kvm
sleep 2

modprobe kvm

echo "Determining if the system is capable of running hardware accelerated KVM virtual machines ....."
kvm-ok
sleep 2

cd $Current_Dir

echo "Downloading and Building qemu-1.7.0 ....."
if [ ! -f qemu-1.7.0.tar.bz2 ]
then
    wget http://wiki.qemu-project.org/download/qemu-1.7.0.tar.bz2
fi
tar -jxvf qemu-1.7.0.tar.bz2
cd qemu-1.7.0/
./configure --prefix=/usr  --target-list=i386-softmmu,x86_64-softmmu --enable-kvm --disable-werror --enable-debug 2>&1 | tee -a $Base_Dir/outfile
make -j 4 2>&1 | tee -a $Base_Dir/outfile
make install 2>&1 | tee -a $Base_Dir/outfile

echo "qemu version is ....."
qemu-system-x86_64 --version
sleep 2

cd $Current_Dir

echo "Downloading and Building libvirt-1.2.1 ....."
if [ ! -f libvirt-1.2.1.tar.gz ]
then
    wget  http://libvirt.org/sources/libvirt-1.2.1.tar.gz
fi
tar -zxvf libvirt-1.2.1.tar.gz
cd libvirt-1.2.1/
sed -i 's/int timeout =.*/int timeout = 60;/g' src/qemu/qemu_monitor.c
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc --with-xen=no --with-esx=no 2>&1 | tee -a $Base_Dir/outfile
make -j 4 2>&1 | tee -a $Base_Dir/outfile
make install 2>&1 | tee -a $Base_Dir/outfile

echo "libvirt version is ....."
libvirtd --version
sleep 2
}

function usage()
{
  echo "Usage: In a root shell in the directory containing the .tar.gz file created by KVM_build.sh: ./KVM_install.sh"
  echo "options: --help, --installpkgs"
  echo "Run with --installpkgs option if this is a new OS install."
  exit
}

arg1=x$1
if [ $arg1 = "x--help" ]
then
  usage
fi

while : ; do
	echo "Enter Openstack Controller IP"
	read controller_ip
	if valid_ip $controller_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

while : ; do
        echo "Enter Openstack KVM Compute IP"
        read compute_ip
        if valid_ip $compute_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

while : ; do
        echo "Enter gateway IP for Openstack KVM Compute node"
        read gateway_ip
        if valid_ip $gateway_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

while : ; do
        echo "Enter dns-nameserver IP for Openstack KVM Compute node"
        read dns_ip
        if valid_ip $dns_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

echo "Enter flat interface (default is eth0)"
read flat_interface
if [ -z "$flat_interface" ]; then flat_interface='eth0'; fi
echo "Flat interface is $flat_interface"

echo "Enter public interface (default is eth0)"
read public_interface
if [ -z "$public_interface" ]; then public_interface='eth0'; fi
echo "Public interface is $public_interface"

tarfile=$(ls -1dtr KVM_install*.tar.gz | tail -1)
tarfiletest=x$tarfile
if [ $tarfiletest = "x" ]
then
	echo "Cannot find the .tar.gz file created by KVM_build.sh."
	exit
fi

tarfile=$(pwd)/$tarfile
Base_Dir=/opt/RP_$(echo $tarfile | sed 's/.*KVM_install_//
s/\..*//')
INSTALL_DIR=$Base_Dir
mkdir -p $INSTALL_DIR
cd $INSTALL_DIR
tar xf $tarfile
Current_Dir=$Base_Dir/Setup-Standardization/KVM

if [ $arg1 = "x--installpkgs" ]
then
  create_local_repo
  install_kvm_packages
  make_kvm
fi

echo Update tcconfig.xml
sed -i -e s?\\\(\<directory\>\\\).*\\\(\<.directory\>\\\)?\\\1$Base_Dir/rpcore/rptmp/config\\\2? -e s?\\\(\<signing_server_ip\>\\\).*\\\(\<.signing_server_ip\>\\\)?\\\1$compute_ip\\\2? -e s?\\\(\<rpcore_ip\>\\\).*\\\(\<.rpcore_ip\>\\\)?\\\1$compute_ip\\\2? $Base_Dir/rpcore/rptmp/config/tcconfig.xml
# Set the RP_CONFIG environment variable
export RP_CONFIG=$Base_Dir/rpcore/rptmp

echo "Installing kpartx and qemu-utils for mounting purpose"
cd $Current_Dir
apt-get --force-yes -y install qemu-utils
if [ $? ];
then
        echo "################## Installed qemu-utils"
else
        echo "################### Error in installation of qemu-utils"
fi
apt-get --force-yes -y install kpartx
if [ $? ];
then
        echo "################## Installed kpartx"
else
        echo "################### Error in installation of kpartx"
fi


#Function Calls
configure_kvm
start_rpcore
start_rplistener



if [ ! -f /root/services.sh ]
then
	echo "opt=\$1" > /root/services.sh
	echo "service nova-api-metadata \$opt" >> /root/services.sh
	echo "service nova-network \$opt" >> /root/services.sh 
	echo "service nova-compute \$opt" >> /root/services.sh
	chmod +x /root/services.sh
fi

# /var/run/libvirt/libvirt-sock gets its ownership changed to root
# during this time.
sleep 2

echo "Make /var/run/libvirt/libvirt-sock accessible to all users ....."
chmod 777 /var/run/libvirt/libvirt-sock

echo "Restarting all Services ....."
/root/services.sh restart

echo "Checking Status of  all Services ....."
/root/services.sh status

echo "IMVM and MOUNT-SCRIPT located in subdirectories of $INSTALL_DIR"

# copy monit scripts to monit directories
cp -f $INSTALL_DIR/rpcore/scripts/rplistener.monit  /etc/monit/conf.d/
cp -f $INSTALL_DIR/rpcore/scripts/vrtm.monit  /etc/monit/conf.d/
service monit restart

echo "Configuration Updation completed....."
