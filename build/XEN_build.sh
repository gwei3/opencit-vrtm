

Current_Dir=$(pwd)

arg1=x$1
if [ $arg1 = "x--help" ]
then
  echo "Usage: In a root shell in the directory to clone mysteryhill repository to or where it exists: KVM_build.sh [option]"
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

echo Making sure git, gcc and g++ as well as other required packages are installed
apt-get --allow-unauthenticated  -y install gcc libsdl1.2-dev zlib1g-dev libasound2-dev linux-headers-`uname -r` pkg-config libgnutls-dev libpci-dev build-essential bzr bzr-builddeb cdbs debhelper devscripts dh-make diffutils dpatch fakeroot gnome-pkg-tools gnupg liburi-perl lintian patch patchutils pbuilder piuparts quilt ubuntu-dev-tools wget libglib2.0-dev libsdl1.2-dev libjpeg-dev libvde-dev libvdeplug2-dev libbrlapi-dev libaio-dev libfdt-dev texi2html texinfo info2man pod2pdf libnss3-dev libcap-dev libattr1-dev libtspi-dev gcc-4.6-multilib libpixman-1-dev libxml2-dev libssl-dev subversion git ant

gccvers=$(gcc --version | head -1 | sed 's/.* //')
gppvers=$(g++ --version | head -1 | sed 's/.* //')
if [ $gccvers = $gppvers ]
then
	echo g++ version matches gcc version
else
	wget http://ubuntu.secs.oakland.edu/pool/main/g/gcc-defaults/gcc_4.6.3-1ubuntu5_amd64.deb
	echo Install gcc_4.6.3-1ubuntu5_amd64.deb to get gcc and g++ versions to match
	dpkg -i gcc_4.6.3-1ubuntu5_amd64.deb
fi

chown $user:$user /opt

# Check if mysteryhill-binaries directory already exists here
if [ -d mysteryhill-binaries ]
then
  # Get the latest versions of the code
  echo Update mysteryhill-binaries
  cd mysteryhill-binaries
  chown -R $user:$user .
  sudo -u $user git fetch
  sudo -u $user git rebase origin
  cd ..
else
  # Get a clone of the code base
  echo Now clone mysteryhill-binaries
  sudo -u $user git clone repo@192.55.66.14:/home/repo/dev/mysteryhill-binaries
fi

# Check if mysteryhill directory already exists here
if [ -d mysteryhill ]
then
  # Get the latest versions of the code
  echo Update mysteryhill
  cd mysteryhill
  chown -R $user:$user .
  sudo -u $user git checkout dev
  sudo -u $user git fetch
  sudo -u $user git rebase origin/dev
else
  # Get a clone of the code base
  echo Now clone mysteryhill
  sudo -u $user git clone repo@192.55.66.14:/home/repo/dev/mysteryhill
  cd mysteryhill
  sudo -u $user git checkout dev
fi

Current_version="`date +%Y%m%d%H%M%S`"

sudo -u $user mkdir -p /opt/RP_$Current_version

echo Copying source files from Setup-Standardization/Dom0 to /opt/RP_$Current_version
sudo -u $user cp -a rpcore /opt/RP_$Current_version
sudo -u $user mkdir -p /opt/RP_$Current_version/Setup-Standardization
sudo -u $user cp -a Setup-Standardization/Dom0 /opt/RP_$Current_version/Setup-Standardization
sudo -u $user cp -a Setup-Standardization/Compute /opt/RP_$Current_version/Setup-Standardization
sudo -u $user cp -p ../mysteryhill-binaries/xen_packages/archives.tar.gz /opt/RP_$Current_version/Setup-Standardization/Dom0

cd /opt/RP_$Current_version/rpcore/src

echo Building rpcore/src
sudo -u $user make clean > outfile 2>&1
sudo -u $user make >> outfile 2>&1

echo Building verifier
cd /opt/RP_$Current_version/rpcore/src/imvm
sudo -u $user make -f verifier-g.mak clean >> /opt/RP_$Current_version/rpcore/src/outfile 2>&1
sudo -u $user make -f verifier-g.mak >> /opt/RP_$Current_version/rpcore/src/outfile 2>&1

grep -i ' error' /opt/RP_$Current_version/rpcore/src/outfile

arg=x$(grep -i ' error' /opt/RP_$Current_version/rpcore/src/outfile)
if [ $arg = "x" ]
then
    echo Create the install tar file
    cd /opt/RP_$Current_version
    sudo -u $user mkdir Dom0
    sudo -u $user tar czf XEN_install_$Current_version.tar.gz rpcore/scripts rpcore/bin rpcore/rptmp rpcore/lib Setup-Standardization/Dom0 Setup-Standardization/Compute
    mv XEN_install_$Current_version.tar.gz Dom0
else
    echo Install tar file will not be created because of build error - see outfile.
fi

