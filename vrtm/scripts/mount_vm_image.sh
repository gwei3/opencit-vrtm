#!/bin/bash
set -x

## Ret codes
## 0 : Operation successful
## 1 : Invalid input and Usage displayed
## 2 : Image does not exist
## 3 : unmount failed
## 4 : guestmount failed with error code


function unmount_vm_image() {
        echo "################ Unmounting the mount path"
        mountPathCheck=$(mount | grep -o "$mountPath")
        if [ ! -z $mountPathCheck ]
        then
                umount $mountPath 2>/dev/null
		retcode=$?
		if [ $retcode -ne 0 ]
		then
			echo "Umount for $mountPath failed with code : $retcode"
			exit 3
		fi
        fi
}

usage() {
        echo "Usage: $0 Image-File mount-path for mounting images"
	echo "       $0 mount-path for unmounting images"		    
        exit 1
}

function mount_disk_guestmount()
{
	imagePath=$(readlink -f $imagePath)
	guestMountBinary=`which guestmount`
	if [ $guestMountBinary == "" ] ; then
		echo "guestmount binary not found, please install libguestfs"
		echo "and libguestfs-tools ( or its corresponding packages as per"
		echo "your linux flavour)"
	fi
	## Proceed mounting with guestmount
	export LIBGUESTFS_BACKEND=direct
	time $guestMountBinary -a $imagePath -i $mountPath
	retcode=$?
	if [ $retcode -eq 0 ] ; then
		echo "Mounted the disk image successfully"
	else
		echo "Guestmount failed and returned with exit code $retcode"
		exit 4
	fi
}

if [ $# -eq 1 ]
then
        mountPath=$1
        unmount_vm_image
	rm -rf `dirname $mountPath`
        exit 0
fi

imagePath=$1
mountPath="$2/mount"

if [ ! -e $imagePath ] ; then
	echo "Image $imagePath does not exist..."
	exit 2
fi

if [ -d $mountPath ] ; then
	echo "mountpoint $mountPath already exists.. Unmounting it"
	unmount_vm_image
else
	echo "mountpoint $mountPath does not exist, creating one"
	mkdir -p $mountPath
fi

imageFormat=$(qemu-img info $imagePath | grep "file format" | awk -F ':' '{ print $2 }' | sed -e 's/ //g')

echo "Disk image format : $imageFormat"
mount_disk_guestmount
exit 0

