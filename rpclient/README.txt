Prerequisites : 
1) Do the vm launch from Openstack. And copy the UUID of the launched vM.

Deployment :

1)CD to this folder <dcg_security-vrtm/rpclient>.
2)Set PYTHONPATH with following command , export PYTHONPATH=./py_rplib
3)Run the new_api_test.py as :

	 python2.7 new_api_test.py <IP_OF_RPCore> <VM_UUID>

where :
    <IP_OF_RPCore> : is IP of machine on which RPCore is running.
    <VM_UUID>      : is VM UUID copied from openstack.


