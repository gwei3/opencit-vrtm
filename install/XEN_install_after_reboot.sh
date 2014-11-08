#!/bin/bash

function valid_device()
{
        local sr_device=$1
        if [ -b $sr_device  ] ; then  
	       return 0
	fi        
        return 1
}

function valid_size()
{
        local sr_lvm_size=$1
	local size=$2
	if [ 0  -eq `echo "$sr_lvm_size > $size" | bc` ] ; then 
	        return 0
        fi
        return 1
}

function usage()
{
  echo "Usage: In a root shell in the directory containing the .tar.gz file created by XEN_build.sh: ./XEN_install_after_reboot.sh after running ./XEN_install.sh and rebooting"
  echo "options: --help"
  exit
}

arg1=x$1
if [ $arg1 = "x--help" ]
then
  usage
fi

echo "Do you want to create SR: Type YES/NO"
read option
if [ ! $option == "YES" ]; then
	echo At user request, SR is not being created
	exit 1
fi

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

Current_Dir=$Base_Dir/Setup-Standardization/Dom0
cd $Current_Dir

found=$(grep -c address /etc/network/interfaces)
if [ $found = "0" ]
then
	echo Please run XEN_install.sh before this script. /etc/network/interfaces is not set up yet.
	exit 1
fi
dom0_ip=$(grep address /etc/network/interfaces | sed 's/.* //')

echo Setting up the encrypted SR storage.
cp ./encrypted_sr.conf /etc/

while : ; do
        echo "Please Enter Device for SR creation: Example: /dev/sda4"
        read sr_device
        if valid_device $sr_device; then break; else echo "Incorrect device : Please Enter Again"; fi
done

volume_group_name=$(cat /etc/encrypted_sr.conf | grep volume_group_name | cut -d= -f2-)
lvm_name=$(cat /etc/encrypted_sr.conf | grep lvm_name | cut -d= -f2-)
encrypted_dev_name=$(cat /etc/encrypted_sr.conf | grep encrypted_dev_name | cut -d= -f2-)
echo volume_group_name=$volume_group_name
echo lvm_name=$lvm_name
echo encrypted_dev_name=$encrypted_dev_name

dev1="/dev/mapper/$volume_group_name-$lvm_name"
dev2="/dev/mapper/$encrypted_dev_name"

echo $dev1
echo $dev2
 
found=$(ls /dev/mapper | grep -c $lvm_name)
if [ ! $found = "0" ]
then
	echo Encrypted partition exists, deleting before recreating.
	crypt_dev_line=$(xe pbd-list | grep -n crypt-dev | sed 's/:.*//')
	crypt_dev_line_test=x$crypt_dev_line
	if [ ! $crypt_dev_line_test = "x" ]
	then
		uuid_line=$(echo $crypt_dev_line - 3 | bc)
		pbd_uuid=$(xe pbd-list | head -$uuid_line | tail -1 | sed 's/.*: //')
		echo Unplug the PBD
		xe pbd-unplug uuid=$pbd_uuid
	fi
	local_storage_line=$(xe sr-list | grep -n LocalStorage | sed 's/:.*//')
	uuid_line=$(echo $local_storage_line - 1 | bc)
	srlist_uuid=$(xe sr-list | head -$uuid_line | tail -1 | sed 's/.*: //')
	echo Forget the SR
	xe sr-forget uuid=$srlist_uuid
	sleep 1	# wait for forget completion
	echo cryptsetup luksClose --key-file=/etc/sr_encryption_key $dev1 $encrypted_dev_name
	cryptsetup luksClose --key-file=/etc/sr_encryption_key $dev1 $encrypted_dev_name

	echo dmsetup remove_all
	dmsetup remove_all
	sleep 1	# wait for removal completion
	echo lvremove -f /dev/mapper/$volume_group_name-$lvm_name
	lvremove -f /dev/mapper/$volume_group_name-$lvm_name
	echo vgremove -f XSLocalEXT-$srlist_uuid
	vgremove -f XSLocalEXT-$srlist_uuid
	echo vgremove $volume_group_name
	vgremove $volume_group_name
	echo pvremove /dev/mapper/$encrypted_dev_name
	pvremove /dev/mapper/$encrypted_dev_name
	echo rm /dev/mapper/$encrypted_dev_name
	rm /dev/mapper/$encrypted_dev_name
fi

pvcreate -ff $sr_device
vgcreate $volume_group_name $sr_device

bytes=`/sbin/blockdev --getsize64 $sr_device`
size=$(echo "scale=3;${bytes%/*}/1024/1024/1024"|bc)


while : ; do
        echo "Please Enter Size in GB for LVM of SR, Please enter size less than '$size'GB size: Dont append G/GB, Example: 100 ]"
        read sr_lvm_size
	if valid_size $sr_lvm_size $size; then break; else echo "Incorrect size : Please Enter Again"; fi
done

gb=G
s=$sr_lvm_size$gb

lvcreate --size $s -n $lvm_name $volume_group_name
echo "Created LVM: /dev/mapper/$volume_group_name-$lvm_name"

fdisk -l | grep '/dev/mapper/$volume_group_name-$lvm_name'
modprobe dm-crypt
modprobe dm-mod
dd if=/dev/urandom of=/etc/sr_encryption_key bs=1k count=2
echo "Generated encryption key :/etc/sr_encryption_key" 
cryptsetup luksFormat --key-file=/etc/sr_encryption_key $dev1
echo "LUKS Format: /dev/mapper/VolumeGroup-mylvm"
cryptsetup luksOpen --key-file=/etc/sr_encryption_key $dev1 $encrypted_dev_name

echo "LUKS Open : /dev/mapper/$volume_group_name-$lvm_name"
echo "Crypt Device : /dev/mapper/$encrypted_dev_name"
mkfs.ext4 $dev2
echo "Formatting with ext4 : /dev/mapper/$encrypted_dev_name"

host=`xe host-list --minimal`
echo "Creating SR"
sr=`xe sr-create device-config:device=$dev2 host-uuid=$host name-label=LocalStorage  type=ext content-type=user`
echo $sr
echo "sr_input_device=$sr_device" >> /etc/encrypted_sr.conf
echo "sr_uuid=$sr" >> /etc/encrypted_sr.conf
xe sr-param-list uuid=$sr
echo "Default SR Set"
pool_id=`xe pool-list --minimal`
xe pool-param-set uuid=$pool_id default-SR=$sr
echo Setting up the encrypted SR storage is complete if all went well.

echo Make the /boot/guest soft link
if [ -d /boot/guest ]
then
    # Remove the guest directory and files
    rm -rf /boot/guest
fi
if [ -L /boot/guest ]
then
    # Remove the previous soft link
    rm /boot/guest
fi
sr_path=$(df -h | grep sr-mount | sed 's/.* //')
mkdir $sr_path/guest
ln -s $sr_path/guest /boot

# remove the file if previously created
if [ -f /etc/rc.local ]
then
    rm /etc/rc.local
fi
sr_uuid=$(cat /etc/encrypted_sr.conf | grep sr_uuid | cut -d= -f2-) 
echo "#!/bin/sh
$INSTALL_DIR/setup_encrypted_sr_after_reboot.sh
" > /etc/rc.local
chmod 777 /etc/rc.local
echo "SR: $sr_uuid is set to activate from /etc/rc.local now"

export RPCORE_IPADDR=$dom0_ip
export RPCORE_PORT=16005
export IP_ADDRESS=$dom0_ip

echo "Starting RPCORE...."
export LD_LIBRARY_PATH=$INSTALL_DIR/rpcore/lib:$LD_LIBRARY_PATH
cp -r $INSTALL_DIR/rpcore/rptmp /tmp
cd $INSTALL_DIR/rpcore/bin/debug
nohup ./nontpmrpcore &

sleep 2
echo "Starting xapi-access-control-proxy...."
cd $INSTALL_DIR/xapi-access-control-proxy
chmod a+x xapi-access-control.py
nohup python xapi-access-control.py &

echo Setting root password
passwd root << END
intelrp
intelrp
END

echo "IMVM, XAPI-ACCESS-CONTROL-PROXY and MOUNT-SCRIPT located at /opt/RP/"
echo "Setup Updation complete.....Please restart this XEN Dom0 compute node..... and then run NOVA_compute_install.sh"
sleep 2
echo

