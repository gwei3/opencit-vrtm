#!/bin/bash

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

function set_keyvaluepair(){
	TARGET_KEY=$1
	REPLACEMENT_VALUE=$2
	CONFIG_FILE=$3
	sed  -i "s|\($TARGET_KEY *= *\).*|\1$REPLACEMENT_VALUE|" $CONFIG_FILE
}

while : ; do
	echo "Enter Openstack Controller IP"
	read controller_ip
	if valid_ip $controller_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

while : ; do
        echo "Enter Openstack Compute IP"
        read compute_ip
        if valid_ip $compute_ip; then break; else echo "Incorrect IP format : Please Enter Again"; fi
done

echo "Enter flat interface (default is eth0)"
read flat_interface
if [ -z "$flat_interface" ]; then flat_interface='eth0'; fi
echo "Flat interface is $flat_interface"

echo "Enter public interface (default is eth0)"
read public_interface
if [ -z "$public_interface" ]; then public_interface='eth0'; fi
echo "Public interface is $public_interface"

Current_Dir=$(pwd)


function configure_kvm() {
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

sed -i 's/.*net.ipv4.ip_forward=[0-9]*/net.ipv4.ip_forward=1/g' /etc/sysctl.conf
grep -q "net.ipv4.ip_forward" /etc/sysctl.conf
if [ $? -ne 0 ]; then 
    echo "net.ipv4.ip_forward=1" >> /etc/sysctl.conf
fi
sysctl -p

dpkg-statoverride  --update --add root root 0644 /boot/vmlinuz-$(uname -r)

cp $Current_Dir/conf_files/statoverride /etc/kernel/postinst.d/statoverride
chmod +x /etc/kernel/postinst.d/statoverride

cp $Current_Dir/conf_files/libvirtmod.so /usr/lib/python2.7/dist-packages/

rm /var/lib/nova/nova.sqlite

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

echo "#!/bin/sh -e" > /etc/rc.local
echo "chown -R nova:nova /var/run/libvirt/" >> /etc/rc.local
echo "exit 0" >> /etc/rc.local

# kill libvirtd, it will start automatically
killall -9 libvirtd
sleep 2

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

if [ -f /usr/bin/qemu-system-x86_64_bak ]
then
    cp /usr/bin/qemu-system-x86_64_bak /usr/bin/qemu-system-x86_64_orig
else
    cp /usr/bin/qemu-system-x86_64 /usr/bin/qemu-system-x86_64_bak
    mv /usr/bin/qemu-system-x86_64 /usr/bin/qemu-system-x86_64_orig
fi
}

function start_rpcore() {
echo "Starting RPCORE...."
export RPCORE_IPADDR=$compute_ip
export RPCORE_PORT=16005
export LD_LIBRARY_PATH=$INSTALL_DIR/rpcore/lib:$LD_LIBRARY_PATH
cp -r $INSTALL_DIR/rpcore/rptmp /tmp
cd $INSTALL_DIR/rpcore/bin/debug
nohup ./nontpmrpcore &
}

function start_rplistener() {
echo "Starting RPLISTENER...."
killall -9 libvirtd

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
cd $INSTALL_DIR/rpcore/bin/debug/
nohup ./rp_listner &
libvirtd -d

chown -R nova:nova /var/run/libvirt/
}

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

echo "Restarting all Services ....."
/root/services.sh restart

echo "Checking Status of  all Services ....."
/root/services.sh status

echo "IMVM and MOUNT-SCRIPT located at /opt/RP/"
echo "Configuration Updation completed....."

