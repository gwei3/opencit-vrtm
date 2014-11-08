#!/bin/sh
dir="/home/v/rp/bin/scripts"

#echo $dir
mkdir -p $1

cd $1
echo "$dir/tftpc $2 "

#this is commented for testing
#$dir/tftpc $2
