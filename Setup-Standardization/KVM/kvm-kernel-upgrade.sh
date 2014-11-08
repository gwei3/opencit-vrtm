#!/bin/bash
a=`uname -r`
apt-get --force-yes -y install "linux-headers-$a"
cp /etc/apt/sources.list /etc/apt/sources.list.bak 
echo "deb http://security.ubuntu.com/ubuntu precise-security main" >> /etc/apt/sources.list
apt-get update
apt-get --force-yes -y install python-apport
#linux_headers=`apt-cache search "linux-headers-3.11.0-" | sort | tail -1 |  awk '{print $1}'`
kernel=`apt-cache search "linux-image-3.11.0-" | sort | tail -1 |  awk '{print $1}'`
version=`echo $kernel  | awk -F linux-image- '{print $2}'`
linux_headers="linux-headers-$version"
apt-get --force-yes -y install $linux_headers $kernel
echo "Please reboot" 
