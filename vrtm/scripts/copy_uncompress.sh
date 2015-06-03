#!/bin/sh
dir="/home/v/rp/bin/scripts"
#echo $dir
mkdir -p $1
cd $1
echo "$dir/tftpc $2 "
$dir/tftpc $2
#uncompress the tar file
filename=`echo $2 | awk  -F-g '{print $2}'`
tar xvf $filename
