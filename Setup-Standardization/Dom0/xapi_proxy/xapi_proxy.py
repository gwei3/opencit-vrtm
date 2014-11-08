import SimpleXMLRPCServer
import xmlrpclib
import time
import random
import sys
import traceback
import os
import shutil
import rpclient
from xapi_config import *


MEASURED_LAUNCH_ALLOWED = 'ML_ALLOWED'
MEASURED_LAUNCH_NOT_ALLOWED = 'ML_DENIED'
RESULT_STATUS_SUCCESS = 'Success'
RESULT_STATUS_FAILURE = 'Failure'
SYMMETRIC_KEY_LENGTH = 128
API_VERSION_1_1 = '1.1'
API_VERSION_1_2 = '1.2'
PARAM_DICT_INDEX = 1
XAPI_XMLRPC_SERVER_ADDRESS = "http://" + XAPI_SERVER_IP + ":" + str(XAPI_SERVER_PORT)


class XAPIProxy(xmlrpclib.ServerProxy):


    def __init__(self, uri, transport=None, encoding=None, verbose=0, allow_none=1):
        xmlrpclib.ServerProxy.__init__(self, uri, transport, encoding, verbose, allow_none)
        self.transport = transport
        self._session = None
        self.last_login_method = None
        self.last_login_params = None
        self.API_version = API_VERSION_1_1
        self.uri = uri
    

    def __getattr__(self, name):
            return xmlrpclib.ServerProxy.__getattr__(self, name)


    def __parse_result(self, result):
        return result


    def __gen_symmetric_key(self):
        rand_bits = random.getrandbits(SYMMETRIC_KEY_LENGTH)
        aes_key = '%032x' % rand_bits
        return aes_key


    def __measured_launch_verify(self, *args) :
        try:
            param_dict = args[PARAM_DICT_INDEX]
	    if param_dict['PV_kernel']:
	    	kernel_path = param_dict['PV_kernel']
       	    	ramdisk_path = param_dict['PV_ramdisk']
	    else:
		if os.path.isfile('/boot/guest/c17bb064-0ccc-499d-b27c-aefd430dee65-test'):
	            os.unlink('/boot/guest/c17bb064-0ccc-499d-b27c-aefd430dee65-test')
                    os.unlink('/boot/guest/5772c792-41fe-4ab8-84d4-d6e7061176ec-test')
                with open('/boot/guest/c17bb064-0ccc-499d-b27c-aefd430dee65-test', 'wb') as fout1:
                    fout1.write(os.urandom(1024))
                with open('/boot/guest/5772c792-41fe-4ab8-84d4-d6e7061176ec-test', 'wb') as fout2:
                    fout2.write(os.urandom(1024))

                kernel_path = "/boot/guest/c17bb064-0ccc-499d-b27c-aefd430dee65-test"
                ramdisk_path = "/boot/guest/5772c792-41fe-4ab8-84d4-d6e7061176ec-test"
		

            kernel_real_path = os.path.realpath(kernel_path)
            ramdisk_real_path = os.path.realpath(ramdisk_path)

            new_kernel_path = os.path.join(IMAGE_TMPFS_DIR, os.path.basename(kernel_path))
            new_ramdisk_path = os.path.join(IMAGE_TMPFS_DIR, os.path.basename(ramdisk_path))

            try:
                shutil.copyfile(kernel_real_path, new_kernel_path)
                shutil.copyfile(ramdisk_real_path, new_ramdisk_path)
            except IOError as e:
                print('Error copying files: %s' % e.strerror)

            os.unlink(kernel_path)
            os.unlink(ramdisk_path)

            os.symlink(new_kernel_path, kernel_path)
            os.symlink(new_ramdisk_path, ramdisk_path)

            rpcore_response_id = self.__get_launch_decision_from_rpcore(new_kernel_path, new_ramdisk_path)
            print "rpcore response is %s" %(str(rpcore_response_id))

            response = {}
            response['status'] = MEASURED_LAUNCH_NOT_ALLOWED
            response['args'] = args
            response['rp_id'] = 0

            if rpcore_response_id > 0 :
                sym_key = self.__gen_symmetric_key()
                arg_list = list(args)
                pv_args_orig = param_dict['PV_args']
                pv_args_updated = "%s symkey=%s rpcore_ip=%s rpcore_port=%s" % (pv_args_orig, sym_key, XAPI_SERVER_IP, RPCORE_PORT)
                arg_list[PARAM_DICT_INDEX]['PV_args'] = pv_args_updated
                updated_args = tuple(arg_list)
                response['status'] = MEASURED_LAUNCH_ALLOWED
                response['args'] = updated_args
                response['rp_id'] = rpcore_response_id
                

        except Exception as e:
            print "measured launch failed", e
            print traceback.format_exc()

        return response


    def __handle_measured_launch(self, full_params):
        resp = self.__measured_launch_verify(*full_params)
        rp_id = int(resp['rp_id'])
        if resp['status'] is MEASURED_LAUNCH_ALLOWED:
            full_params = resp['args']
            return (rp_id, full_params)
        else:
            return (rp_id, full_params)
       

    def dispatch_request(self, *args):
        try:
            methodname = args[0]
            full_params = list(args[1:])
            create_measured_vm = False
            rp_domid = 0

            if str(methodname).startswith('VM.create') and full_params[PARAM_DICT_INDEX]['is_measured'] == 'True':
                rp_domid, full_params = self.__handle_measured_launch(full_params)
		
                #full_params = self.__handle_measured_launch(full_params)
                create_measured_vm = True
                if rp_domid is 0:
                    result = {}
                    #res_map = {}
                    result['Status'] = RESULT_STATUS_SUCCESS
                   # res_map['vm_ref'] = MEASURED_LAUNCH_NOT_ALLOWED
                    #res_map['rp_id'] = 0
                    result['Value'] = MEASURED_LAUNCH_NOT_ALLOWED
                    return result 
    
            full_params_t = tuple(full_params)
            result = self.__parse_result(getattr(self, methodname)(*full_params_t))

            #if create_measured_vm:
                #vm_ref = result['Value']
                #res_map = {}
                #res_map['vm_ref'] = vm_ref
                #res_map['rp_id'] = rp_domid
                #result['Value'] = res_map

            return result
        except Exception as e:
            print "dispatching request failed ", e

    
    def session_login_with_password(self, *args):
        result = self.__parse_result(getattr(self, "session.login_with_password")(*args))
        return result


    def __get_launch_decision_from_rpcore(self, kernel_path, ramdisk_path):
        try:
            result = rpclient.get_rpcore_decision(kernel_path, ramdisk_path)
            return result
        except Exception as e:
            print "__get_launch_decision_from_rpcore failed ", e
            return 0



class _Dispatcher:
    def __init__(self, API_version, send, name):
        self.__API_version = API_version
        self.__send = send
        self.__name = name

    def __repr__(self):
        if self.__name:
            return '<XenAPI._Dispatcher for %s>' % self.__name
        else:
            return '<XenAPI._Dispatcher>'

    def __getattr__(self, name):
        if self.__name is None:
            return _Dispatcher(self.__API_version, self.__send, name)
        else:
            return _Dispatcher(self.__API_version, self.__send, "%s.%s" % (self.__name, name))

    def __call__(self, *args):
        return self.__send(self.__name, args)


proxy = XAPIProxy(XAPI_XMLRPC_SERVER_ADDRESS)
xmlrpc_server = SimpleXMLRPCServer.SimpleXMLRPCServer((XAPI_PROXYSERVER_IP, XMLRPC_PROXYSERVER_PORT), logRequests=False)
xmlrpc_server.register_introspection_functions()
xmlrpc_server.register_function(proxy.session_login_with_password, 'session.login_with_password')
xmlrpc_server.register_function(proxy.dispatch_request, 'dispatch_request')
xmlrpc_server.serve_forever()


