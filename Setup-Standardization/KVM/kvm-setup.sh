#!/bin/bash

Current_Dir=$(pwd)

function create_local_repo() {
apt-get update

apt-get -y install dpkg-dev

echo "Copying deb files ....."
cd $Current_Dir
if [ ! -d  supermin.d ]
then
    mkdir supermin.d
fi
tar zxf archives.tar.gz
dpkg-scanpackages  archives | gzip -9c > archives/Packages.gz

mmm=$(echo $Current_Dir | sed 's/\//\\\//g')
sed -i "1s/^/deb file:\/\/$mmm\/ archives\/\n/" /etc/apt/sources.list
#echo "deb file://$Current_Dir/ archives/" >> /etc/apt/sources.list

apt-get update
}

function install_kvm_packages() {
echo "Installing Required Packages ....."
apt-get -y install gcc libsdl1.2-dev zlib1g-dev libasound2-dev linux-kernel-headers pkg-config libgnutls-dev libpci-dev build-essential bzr bzr-builddeb cdbs debhelper devscripts dh-make diffutils dpatch fakeroot gnome-pkg-tools gnupg liburi-perl lintian patch patchutils pbuilder piuparts quilt ubuntu-dev-tools wget libglib2.0-dev libsdl1.2-dev libjpeg-dev libvde-dev libvdeplug2-dev libbrlapi-dev libaio-dev libfdt-dev texi2html texinfo info2man pod2pdf libnss3-dev libcap-dev libattr1-dev libtspi-dev gcc-4.6-multilib libpixman-1-dev libxml2-dev libssl-dev wget git

apt-get -y install libyajl-dev libdevmapper-dev libpciaccess-dev libnl-dev
apt-get -y install bridge-utils dnsmasq pm-utils ebtables ntp chkconfig guestfish
apt-get -y install python-mysqldb=1.2.3-1ubuntu0.1
apt-get -y install nova-compute-kvm=1:2013.2.2-0ubuntu1~cloud0 nova-compute=1:2013.2.2-0ubuntu1~cloud0 python-guestfs=1:1.14.8-1
apt-get -y install nova-network=1:2013.2.2-0ubuntu1~cloud0 nova-api-metadata=1:2013.2.2-0ubuntu1~cloud0
apt-get -y install openssh-server
apt-get -y install python-dev
apt-get -y install tboot trousers tpm-tools libtspi-dev cryptsetup-bin
apt-get -y install qemu-utils kpartx


echo "Starting ntp service ....."
service ntp start
chkconfig ntp on

}

function configure_kvm() {
echo "Verifying the kvm kernel module ....."
lsmod | grep kvm
sleep 2

modprobe kvm

echo "Determining if the system is capable of running hardware accelerated KVM virtual machines ....."
kvm-ok
sleep 2

cd $Current_Dir

echo "Downloading and Building qemu-1.7.0 ....."
wget http://wiki.qemu-project.org/download/qemu-1.7.0.tar.bz2
tar -jxvf qemu-1.7.0.tar.bz2
cd qemu-1.7.0/
./configure --prefix=/usr  --target-list=i386-softmmu,x86_64-softmmu --enable-kvm --disable-werror --enable-debug
make -j 4
make install

echo "qemu version is ....."
qemu-system-x86_64 --version
sleep 2

cd $Current_Dir

echo "Downloading and Building libvirt-1.2.1 ....."
wget  http://libvirt.org/sources/libvirt-1.2.1.tar.gz
tar -zxvf libvirt-1.2.1.tar.gz
cd libvirt-1.2.1/
sed -i 's/int timeout =.*/int timeout = 60;/g' src/qemu/qemu_monitor.c
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc --with-xen=no --with-esx=no
make -j 4
make install

echo "libvirt version is ....."
libvirtd --version
sleep 2
}

#Function Calls
create_local_repo
install_kvm_packages
configure_kvm

echo "Setup Completed Successfully !!!!"
