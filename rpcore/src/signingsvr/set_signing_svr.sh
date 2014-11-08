#!/bin/bash

#cleanup for testing

echo "Please enter setup directory:"
read key_location
echo $key_location

[ -d $key_location/policy ] || mkdir -m 0755 $key_location/policy

~/rp/bin/release/rpapicryputil.exe -GenKeyFile RSA2048 policyKey.xml

mv policyKey.xml $key_location/policy/privatePolicyKey.xml
sed 's/<ds\:[D|P|Q].*$//' $key_location/policy/privatePolicyKey.xml




