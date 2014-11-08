#!/bin/bash
sr_uuid=$(cat /etc/encrypted_sr.conf | grep sr_uuid | cut -d= -f2-) 
volume_group_name=$(cat /etc/encrypted_sr.conf | grep volume_group_name | cut -d= -f2-)
encrypted_dev_name=$(cat /etc/encrypted_sr.conf | grep encrypted_dev_name | cut -d= -f2-)
lvm_name=$(cat /etc/encrypted_sr.conf | grep lvm_name | cut -d= -f2-)
cryptsetup luksOpen --key-file=/etc/sr_encryption_key /dev/mapper/$volume_group_name-$lvm_name  $encrypted_dev_name
pbd=$(xe pbd-list sr-uuid=$sr_uuid --minimal)
xe pbd-plug uuid=$pbd
echo "SR: $sr_uuid is active now"
