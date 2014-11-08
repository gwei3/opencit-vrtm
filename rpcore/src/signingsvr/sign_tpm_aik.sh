#!/bin/bash

rpbin=~/rp/bin/release
#echo "Please enter AIK location:"
read key_location

#echo "Config directory is $key_location"
key_location=.

#sign tpm key
$rpbin/signerutil -SignHexModulus $key_location/policy/privatePolicyKey.xml $key_location/aikhexmod.txt $key_location/cert


#

