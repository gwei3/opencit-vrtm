
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

Current_Dir=$(pwd)

function create_local_repo()
{
apt-get update

apt-get --force-yes -y install dpkg-dev

echo "Copying deb files ....."
cd /opt/RP_$Current_version/Setup-Standardization/KVM
if [ ! -d  supermin.d ]
then
    mkdir supermin.d
fi
tar zxf $Current_Dir/mysteryhill/Setup-Standardization/KVM/archives.tar.gz
dpkg-scanpackages  archives | gzip -9c > archives/Packages.gz

mmm=$(echo /opt/RP_$Current_version/Setup-Standardization/KVM | sed 's/\//\\\//g')
# Delete the line if there in case this script was run before
sed -i "/deb file:/d" /etc/apt/sources.list
sed -i "1s/^/deb file:\/\/$mmm\/ archives\/\n/" /etc/apt/sources.list
#echo "deb file://$Current_Dir/ archives/" >> /etc/apt/sources.list

apt-get update
}

function install_kvm_packages()
{
echo "Installing Required Packages ....."
apt-get update
apt-get --force-yes -y install gcc libsdl1.2-dev zlib1g-dev libasound2-dev linux-kernel-headers pkg-config libgnutls-dev libpci-dev build-essential bzr bzr-builddeb cdbs debhelper devscripts dh-make diffutils dpatch fakeroot gnome-pkg-tools gnupg liburi-perl lintian patch patchutils pbuilder piuparts quilt ubuntu-dev-tools wget libglib2.0-dev libsdl1.2-dev libjpeg-dev libvde-dev libvdeplug2-dev libbrlapi-dev libaio-dev libfdt-dev texi2html texinfo info2man pod2pdf libnss3-dev libcap-dev libattr1-dev libtspi-dev gcc-4.6-multilib libpixman-1-dev libxml2-dev libssl-dev subversion git ant

apt-get --force-yes -y install libyajl-dev libdevmapper-dev libpciaccess-dev libnl-dev
apt-get --force-yes -y install bridge-utils dnsmasq pm-utils ebtables ntp chkconfig
apt-get --force-yes -y install python-mysqldb=1.2.3-1ubuntu0.1
# apt-get --force-yes -y install nova-compute-kvm=1:2013.2.2-0ubuntu1~cloud0 nova-compute=1:2013.2.2-0ubuntu1~cloud0 python-guestfs=1:1.14.8-1
# apt-get --force-yes -y install nova-network=1:2013.2.2-0ubuntu1~cloud0 nova-api-metadata=1:2013.2.2-0ubuntu1~cloud0
apt-get --force-yes -y install openssh-server
apt-get --force-yes -y install python-dev

echo "Starting ntp service ....."
service ntp start
chkconfig ntp on

}

arg1=x$1
if [ $arg1 = "x--help" ]
then
  echo "Usage: In a root shell in the directory to clone mysteryhill repository to or where it exists: KVM_build.sh [option]"
  echo "options: --help, --installpkgs"
  echo "Run with --installpkgs option if build errors."
  exit
fi

while : ; do
        echo "Repo machine IP address - hit only enter for 192.55.66.14"
        read avalonia_ip
	arg=x$avalonia_ip
	if [ $arg = "x" ]
	then
		avalonia_ip=192.55.66.14
		break
	fi
        if valid_ip $avalonia_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

while : ; do
	echo "Please enter username enabled for ssh to Avalonia - 192.55.66.14"
	read user
	found=`ls /home | grep -c $user`

	if [ $found = "0" ]
	then
	  echo User $user is unknown.  Please Enter again
	else
	  break;
	fi
done

echo Make sure git is installed
apt-get --force-yes -y install git

# Check if mysteryhill directory already exists here
if [ -d mysteryhill ]
then
  # Get the latest versions of the code
  cd mysteryhill
  chown -R $user:$user .
  sudo -u $user git checkout dev
  sudo -u $user git fetch
  sudo -u $user git rebase origin/dev
else
  # Get a clone of the code base
  sudo -u $user git clone repo@192.55.66.14:/home/repo/dev/mysteryhill
  cd mysteryhill
  sudo -u $user git checkout dev
fi

Current_version="`date +%Y%m%d%H%M%S`"

mkdir -p /opt/RP_$Current_version
chown $user:$user /opt
chown $user:$user /opt/RP_$Current_version

echo Copying source files, mount_vm_image.sh, and conf_files - mhagent, driver.py, utils.py to /opt/RP_$Current_version
sudo -u $user cp -a rpcore /opt/RP_$Current_version
mkdir -p /opt/RP_$Current_version/Setup-Standardization/KVM
chown $user:$user /opt/RP_$Current_version/Setup-Standardization/KVM
sudo -u $user cp -a Setup-Standardization/KVM/conf_files /opt/RP_$Current_version/Setup-Standardization/KVM
sudo -u $user cp -p Setup-Standardization/KVM/archives.tar.gz /opt/RP_$Current_version/Setup-Standardization/KVM

if [ $arg1 = "x--installpkgs" ]
then
  create_local_repo
  install_kvm_packages
fi

cd /opt/RP_$Current_version/rpcore/src

echo Building rpcore/src
sudo -u $user make clean > outfile 2>&1
sudo -u $user make >> outfile 2>&1

if [ ! -d  /home/$user/Downloads ]
then
    mkdir /home/$user/Downloads
fi
cd /home/$user/Downloads
echo Get the libvirt source - needed for header files
if [ ! -f libvirt-1.2.1.tar.gz ]
then
    sudo -u $user wget http://libvirt.org/sources/libvirt-1.2.1.tar.gz
fi

if [ ! -f libvirt-1.2.1/src/.libs/libvirt.so.0.1002.1 ]
then
    sudo -u $user tar xf libvirt-1.2.1.tar.gz

    cd libvirt-1.2.1
    echo Building libvirt - may take a few minutes
    sudo -u $user ./configure > outfile 2>&1
    sudo -u $user make >> outfile 2>&1
else
    cd libvirt-1.2.1
fi
sudo -u $user cp -a include/libvirt /opt/RP_$Current_version/rpcore/src/rpchannel
sudo -u $user cp -pL src/.libs/libvirt.so.0.1002.1 /opt/RP_$Current_version/rpcore/lib/libvirt.so

echo Building verifier
cd /opt/RP_$Current_version/rpcore/src/imvm
sudo -u $user make -f verifier-g.mak clean >> /opt/RP_$Current_version/rpcore/src/outfile 2>&1
sudo -u $user make -f verifier-g.mak >> /opt/RP_$Current_version/rpcore/src/outfile 2>&1

echo Building rp_listner and rp_proxy
cd /opt/RP_$Current_version/rpcore/src/rpproxy/kvm_proxy
sudo -u $user mkdir -p /opt/RP_$Current_version/rpcore/build/debug/rpproxyobjects
sudo -u $user make -f rp-proxy.mak clean >> /opt/RP_$Current_version/rpcore/src/outfile 2>&1
sudo -u $user make -f rp-proxy.mak >> /opt/RP_$Current_version/rpcore/src/outfile 2>&1

grep -i ' error' ../../outfile

arg=x$(grep -i ' error' ../../outfile)
if [ $arg = "x" ]
then
    echo Create the install tar file
    cd /opt/RP_$Current_version
    sudo -u $user mkdir KVM
    sudo -u $user tar czf KVM_install_$Current_version.tar.gz rpcore/scripts rpcore/bin rpcore/rptmp rpcore/lib Setup-Standardization/KVM/conf_files Setup-Standardization/KVM/archives.tar.gz
    mv KVM_install_$Current_version.tar.gz KVM
else
    echo Install tar file will not be created because of build error - see outfile.
fi

