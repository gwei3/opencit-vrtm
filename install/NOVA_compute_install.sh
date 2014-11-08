#!/bin/bash

function usage()
{
  echo "Usage: Add the ssh public key from this machine to ~repo/.ssh/authorized_keys
and ~a/.ssh/authorized_keys on the machine at 192.55.66.14 FIRST.
OR, download base_image_compute_new to the directory this will be run from.
Then in a root shell: ./NOVA_compute_install.sh

Run this script to set up and start the NOVA compute node. Then log into
it as root with the usual password and run NOVA_compute_config.sh while
logged into the NOVA compute node."
  echo "options: --help"
  exit
}

arg1=x$1
if [ $arg1 = "x--help" ]
then
  usage
fi

# Compute setup section
Base_Image=`locate base_image_compute_new | tail -1`

if [ ! "x" = x$Base_Image ]
then
  echo We have base image
else
  # Check in this directory - may not be in locate database yet
  Base_Image=`ls base_image_compute_new`
  if [ ! "x" = x$Base_Image ]
  then
    echo We have base image
  else
    echo We have no base image - Downloading...
    while : ; do
	    echo "Please enter username enabled for ssh to Avalonia - 192.55.66.14"
	    read user
	    if [ "x" = x$user ]
	    then
	      user="emptyusername"
	    fi
	    found=`ls /home | grep -c $user`

	    if [ $found = "0" ]
	    then
	      echo User $user is unknown.  Please Enter again
	    else
	      break;
	    fi
    done

    sudo -u $user scp -p repo@192.55.66.14:/home/repo/Setup-iso-images/xen_images/base_image_compute_new .
    Base_Image=./base_image_compute_new
  fi
fi

echo Running xe vm-import filename=$Base_Image
Base_UUID=$(xe vm-import filename=$Base_Image)

echo Starting the compute image: xe vm-start uuid=$Base_UUID
xe vm-start uuid=$Base_UUID

echo Now start the vm console: xe console uuid=$Base_UUID
echo Then set the ip address: ifconfig eth0 192.168.1.xx




