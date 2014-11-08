#!/bin/bash

rpbin=~/rp/bin/release
#echo "Please enter setup directory:"
#read key_location

#echo "Config directory is $key_location"
key_location=.
#create server root key

[ -d $key_location/policy ] || mkdir -m 0755 $key_location/policy

echo "Generating server key-pair at $key_location/policy"
$rpbin/signerutil -GenKeyFile RSA2048 policyKey.xml

mv policyKey.xml $key_location/policy/privatePolicyKey.xml
sed 's/<ds\:[D|P|Q].*$//' $key_location/policy/privatePolicyKey.xml > $key_location/policy/pubPolicyKey.xml

echo "Server root setup"

$rpbin/signerutil -Sign $key_location/policy/privatePolicyKey.xml rsa2048-sha256-pkcspad $key_location/policy/pubPolicyKey.xml $key_location/policy/policyCert.xml
