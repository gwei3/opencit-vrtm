#!/bin/bash

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

function usage()
{
  echo "Usage: In a root shell in the directory to clone mysteryhill repository to or where it exists: ./Client_build.sh [option]"

  echo "options: --help"
  exit
}

arg1=x$1
if [ $arg1 = "x--help" ]
then
  usage
fi

Current_Dir=$(pwd)

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

while : ; do
	echo "Enter the Glance IP address"
	read glance_ip
	if valid_ip $glance_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

while : ; do
	echo "Enter the KMS Server IP address"
	read kms_ip
	if valid_ip $kms_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

echo Uninstalling virtualbox to make sure other installs are successful
apt-get -f -y install

# losetup comes as part of the initial OS install
apt-get --force-yes -y install git ant qemu-utils kpartx expect

# Check if mysteryhill directory already exists here
if [ -d mysteryhill ]
then
  # Get the latest versions of the code
  cd mysteryhill
  chown -R $user:$user .
  git checkout dev
  sudo -u $user git fetch
  sudo -u $user git rebase origin/dev
else
  # Get a clone of the code base
  sudo -u $user git clone repo@192.55.66.14:/home/repo/dev/mysteryhill
  cd mysteryhill
  git checkout dev
fi

echo Install virtualbox for vdfuse for mount_vm_image.sh
echo Installing virtualbox last because --force-depends creates an error for later installs.
apt-get --allow-unauthenticated -y install virtualbox
dpkg -i --force-depends Setup-Standardization/Dom0/virtualbox-fuse_4.1.18-dfsg-1ubuntu1_amd64.deb

Current_version="`date +%Y%m%d%H%M%S`"

mkdir -p /opt/RP_$Current_version

echo Copying source files to /opt/RP_$Current_version
cp -a ManifestTool /opt/RP_$Current_version


# Prepare to build the ManifestTool (client_tool)
cd $Current_Dir
found=`java -version |& grep -c '1.7'`

if [ $found = "0" ]
then
  echo We do not have version 1.7 - Downloading version 1.7

  sudo -u $user scp -p a@192.55.66.14:~/Milestone-3-intermediate/jdk-7u55* .
  mkdir -p /usr/lib/jvm
  cd /usr/lib/jvm
  tar xf $Current_Dir/jdk-7u55*
  update-alternatives --install "/usr/bin/java" "java" "/usr/lib/jvm/jdk1.7.0_55/bin/java" 1
  update-alternatives --install "/usr/bin/javac" "javac" "/usr/lib/jvm/jdk1.7.0_55/bin/javac" 1
  update-alternatives --install "/usr/bin/javaws" "javaws" "/usr/lib/jvm/jdk1.7.0_55/bin/javaws" 1
  update-alternatives --config java
  update-alternatives --config javac
  update-alternatives --config javaws
else
  echo We have version 1.7
fi

# Now build the ManifestTool (client_tool)
echo "Build the ManifestTool - client_tool"
cd /opt/RP_$Current_version/ManifestTool
ant init
ant compile
ant dist

echo Setting Glance IP in config.properties
set_keyvaluepair Glance_IP $glance_ip resources/config.properties

echo Setting KMS_Server IP in config.properties
set_keyvaluepair KMS_Server_IP $kms_ip resources/config.properties

