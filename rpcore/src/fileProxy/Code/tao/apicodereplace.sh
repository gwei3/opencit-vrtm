#!/bin/sh
srch=$1
rplc=$2
sed -i "s/$srch/$rplc/g" *
