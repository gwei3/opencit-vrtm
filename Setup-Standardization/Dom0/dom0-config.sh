#!/bin/bash +x

current_dir=`pwd`
INSTALL_DIR="/opt/RP"


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

function set_keyvaluepair(){
    TARGET_KEY=$1
    REPLACEMENT_VALUE=$2
    CONFIG_FILE=$3
    sed -i "s/\"$TARGET_KEY\" : .*/\"$TARGET_KEY\" : \"$REPLACEMENT_VALUE\",/" $CONFIG_FILE
}

echo "Copying MH Agent to /usr/local/bin/mhagent"
mv /usr/local/bin/mhagent /usr/local/bin/mhagent.bak 2> /dev/null
cd $current_dir
cp mhagent /usr/local/bin/mhagent
chmod +x /usr/local/bin/mhagent

while : ; do
        echo "Enter Dom0 IP"
        read dom0_ip
        if valid_ip $dom0_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

export RPCORE_IPADDR=$dom0_ip
export RPCORE_PORT=16005
export IP_ADDRESS=$dom0_ip

mkdir -p /boot/guest

echo "Installing qemu-utils for mounting purpose"
cd $current_dir

apt-get install kpartx
if [ $? ];
then
        echo "################## Installed kpartx"
else
        echo "################### Error in installation of kpartx"
fi

apt-get --force-yes -y install qemu-utils
if [ $? ];
then
        echo "################## Installed qemu-utils"
else
        echo "################### Error in installation of qemu-utils"
fi

apt-get --force-yes -y install virtualbox

mkdir -p $INSTALL_DIR/scripts
cp virtualbox-fuse_4.1.18-dfsg-1ubuntu1_amd64.deb /opt/RP/scripts/

dpkg -i --force-depends /opt/RP/scripts/virtualbox-fuse_4.1.18-dfsg-1ubuntu1_amd64.deb

vdf=`cat /etc/rc.local | grep "virtualbox-fuse_4.1.18-dfsg-1ubuntu1_amd64.deb"`
if [ $vdf ]
then
  sed -i '$d' /etc/rc.local
  echo "dpkg -i --force-depends /opt/RP/scripts/virtualbox-fuse_4.1.18-dfsg-1ubuntu1_amd64.deb" >> /etc/rc.local
  echo "exit 0" >> /etc/rc.local
fi

echo "Updating Plugins at /usr/lib/xcp/plugins"
UpdatePlugins

echo "Restarting XCP XAPI"
service xcp-xapi restart

mkdir -p $INSTALL_DIR

#echo "Copying mount script to /opt/RP/scripts/"
#cd $current_dir
#cp mount_vm_image.sh $INSTALL_DIR/scripts/

#echo "Copying IMVM(verifier) to /opt/RP/IMVM/"
#cd $current_dir
#mkdir -p $INSTALL_DIR/IMVM
#cp verifier verifier.c verifier-g.mak $INSTALL_DIR/IMVM/

cd $current_dir
echo "Copying sr creation script to /opt/RP/"
cp setup_encrypted_sr.sh  setup_encrypted_sr_after_reboot.sh $INSTALL_DIR/

echo "Starting RPCORE...."
export LD_LIBRARY_PATH=$INSTALL_DIR/rpcore/lib:$LD_LIBRARY_PATH
cp -r $INSTALL_DIR/rpcore/rptmp /tmp
cd $INSTALL_DIR/rpcore/bin/debug
nohup ./nontpmrpcore &

sleep 2
echo "Copying xapi-access-control-proxy to /opt/RP/"
echo "Starting xapi-access-control-proxy...."
cd $current_dir
cp intel_rpcore_plugin.conf /etc/

set_keyvaluepair RPCORE_PORT  $RPCORE_PORT /etc/intel_rpcore_plugin.conf
set_keyvaluepair RPCORE_IPADDR $RPCORE_IPADDR /etc/intel_rpcore_plugin.conf
cp -r xapi-access-control-proxy $INSTALL_DIR
cd $INSTALL_DIR/xapi-access-control-proxy
chmod a+x xapi-access-control.py
nohup python xapi-access-control.py &

echo "IMVM, XAPI-ACCESS-CONTROL-PROXY and MOUNT-SCRIPT located at /opt/RP/"
echo "Setup Updation complete....."

