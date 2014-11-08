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


cp ./encrypted_sr.conf /etc/

while : ; do
        echo "Please Enter Device for SR creation: Example: /dev/sda4"
        read sr_device
        if valid_device $sr_device; then break; else echo "Incorrect device : Please Enter Again"; fi
done

volume_group_name=$(cat /etc/encrypted_sr.conf | grep volume_group_name | cut -d= -f2-)
lvm_name=$(cat /etc/encrypted_sr.conf | grep lvm_name | cut -d= -f2-)
encrypted_dev_name=$(cat /etc/encrypted_sr.conf | grep encrypted_dev_name | cut -d= -f2-)
echo volume_group_name
echo lvm_name
echo encrypted_dev_name

dev1="/dev/mapper/$volume_group_name-$lvm_name"
dev2="/dev/mapper/$encrypted_dev_name"

echo $dev1
echo $dev2
 
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
apt-get  --force-yes -y install cryptsetup-bin
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




