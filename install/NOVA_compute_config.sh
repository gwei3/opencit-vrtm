#!/bin/bash 
# This script will configure nova compute node

function ApplyPatchedFiles()
{
    cd $current_dir
    cd patched_files
    rm -f /tmp/patched_files
    touch /tmp/patched_files
    find . -type f > /tmp/patched_files
    for i in `cat /tmp/patched_files`;do mv ${i:1} ${i:1}.bak; done
    for i in `cat /tmp/patched_files`;do cp $i ${i:1};done
}


function set_keyvaluepair()
{
	TARGET_KEY=$1
	REPLACEMENT_VALUE=$2
	CONFIG_FILE=$3
#	echo $REPLACEMENT_VALUE
	sed  -i "s|\($TARGET_KEY *= *\).*|\1$REPLACEMENT_VALUE|" $CONFIG_FILE
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

function usage()
{
  echo "Usage: First, copy this script and the .tar.gz file created by
XEN_build.sh to the nova compute node VM.  Then in a root shell in the
directory containing the .tar.gz file created by XEN_build.sh:
./NOVA_compute_config.sh
NOTE: -- This script must be run on the nova compute node VM after running
NOVA_compute_install.sh on Dom0 to create the nova compute node VM."

  echo "options: --help"
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
        echo "Enter Openstack Nova Compute IP"
        read compute_ip
        if valid_ip $compute_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

while : ; do
        echo "Enter Dom0 IP"
        read dom0_ip
        if valid_ip $dom0_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

while : ; do
        echo "Enter gateway IP for Openstack Compute node"
        read gateway_ip
        if valid_ip $gateway_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

while : ; do
        echo "Enter dns-nameserver IP for Openstack Compute node"
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

echo Extracting the tar file .....
tarfile=$(ls -1dtr XEN_install*.tar.gz | tail -1)
tarfiletest=x$tarfile
if [ $tarfiletest = "x" ]
then
	echo "Cannot find the .tar.gz file created by XEN_build.sh."
	exit
fi

tarfile=$(pwd)/$tarfile
Base_Dir=/opt/RP_$(echo $tarfile | sed 's/.*XEN_install_//
s/\..*//')
INSTALL_DIR=$Base_Dir
mkdir -p $INSTALL_DIR
cd $INSTALL_DIR
tar xf $tarfile
current_dir=$Base_Dir/Setup-Standardization/Compute

echo "Applying the Patched Files ....."
sleep 1
ApplyPatchedFiles


cd $current_dir
cd config_files


echo "Updating the Configuration Files ....."
set_keyvaluepair metadata_host $controller_ip nova.conf
set_keyvaluepair rabbit_host  $controller_ip nova.conf
set_keyvaluepair my_ip $compute_ip nova.conf
set_keyvaluepair vncserver_proxyclient_address $compute_ip nova.conf
set_keyvaluepair novncproxy_base_url "http://$controller_ip:6080/vnc_auto.html" nova.conf
set_keyvaluepair glance_host $controller_ip nova.conf
set_keyvaluepair glance_api_servers  "$controller_ip:9292" nova.conf
set_keyvaluepair nova_url "http://$controller_ip:8774/v1.1/" nova.conf
set_keyvaluepair network_host $compute_ip nova.conf
set_keyvaluepair connection "mysql://nova:intelrp@$controller_ip/nova" nova.conf
set_keyvaluepair xenapi_connection_url "http://$dom0_ip" nova.conf
set_keyvaluepair flat_interface $flat_interface nova.conf
set_keyvaluepair public_interface $public_interface nova.conf
set_keyvaluepair xenapi_connection_url "http://$dom0_ip" nova-compute.conf
set_keyvaluepair xenapi_proxy_connection_url "http://$dom0_ip:8080"  nova.conf
set_keyvaluepair xenapi_proxy_connection_url "http://$dom0_ip:8080" nova-compute.conf
set_keyvaluepair auth_host $controller_ip  api-paste.ini
set_keyvaluepair admin_tenant_name service api-paste.ini
set_keyvaluepair admin_user nova api-paste.ini
set_keyvaluepair admin_password intelrp api-paste.ini

mv /etc/nova/nova.conf /etc/nova/nova.conf.bak 2> /dev/null
mv /etc/nova/nova-compute.conf /etc/nova/nova-compute.conf.bak 2> /dev/null
mv /etc/nova/api-paste.ini /etc/nova/api-paste.ini.bak 2> /dev/null

cd $current_dir
cp config_files/nova.conf /etc/nova/nova.conf
cp config_files/nova-compute.conf /etc/nova/nova-compute.conf
cp config_files/api-paste.ini /etc/nova/api-paste.ini

chown nova:nova /etc/nova/nova.conf /etc/nova/nova-compute.conf /etc/nova/api-paste.ini

cat /etc/network/interfaces | grep $compute_ip
if [ $? ==  1 ]
then

echo Updating /etc/network/interfaces
cp /etc/network/interfaces /etc/network/interfaces.bak

rm /etc/network/interfaces
echo "# This file describes the network interfaces available on your system
# and how to activate them. For more information, see interfaces(5).

# The primary network interface
auto eth0
iface eth0 inet static
	address $compute_ip
	netmask 255.255.255.0
	gateway $gateway_ip
	dns-nameservers $dns_ip" > /etc/network/interfaces

echo Restart networking
/etc/init.d/networking restart
fi

echo > /var/log/nova/nova-compute.log
echo > /var/log/nova/nova-network.log
echo > /var/log/nova/nova-api-metadata.log
service nova-api-metadata restart
service nova-network restart
service nova-compute restart
sleep 2 
service nova-api-metadata status
service nova-network status
service nova-compute status

echo "Setup Update Complete !!!!"
