#!/bin/bash

# This script is pre-heats guestmount to reduce the consecutive mounting times for disk images.

# Following is what this script does :
# 1. Download a cirros image
# 2. guestmount it at some random location
# 3. Wait for 2 secs
# 4. unmount the image

# This script internally preheats the guest-mount
# By preheating it means that it creates all the files required by supermin- 
# application which is being used internally by guestmount

mountLoc=/tmp/$RANDOM
cirrosImageFile=cirros-0.3.3-x86_64-disk.img
mkdir -p $mountLoc
if [ -e /tmp/$cirrosImageFile ] ; then
	echo "Found cirros file in /tmp"
else
	echo "Downloading the cirros file"
	wget http://download.cirros-cloud.net/0.3.3/cirros-0.3.3-x86_64-disk.img
	mv $cirrosImageFile /tmp/.
fi

time guestmount -a /tmp/$cirrosImageFile -i $mountLoc
echo "Guestmount mounted"
sleep 2
umount $mountLoc
rm -rf $mountLoc
