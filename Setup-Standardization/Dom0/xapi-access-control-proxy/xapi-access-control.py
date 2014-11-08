#!/usr/bin/python
import socket
import os
import select
import time
import sys
import sys, time
import logging
from com.intel.rpcore.plugin.exception import AccessControlException
from xml.dom import minidom
from subprocess import Popen, PIPE
from com.intel.rpcore.plugin import CONFIG
from com.intel.rpcore.plugin import KEY
from com.intel.rpcore.plugin import rpclient
import xmlrpclib as lib
try:
    import xmlrpc.client as xmlrpc
except ImportError:
    import xmlrpclib as xmlrpc

import requests

buffer_size = 4096
delay = 0.0001
forward_to = ('127.0.0.1', 80)
fw = 0
vm_ref_list = []

""" This class used to make xenapi requests """
class RequestsXAPI(xmlrpc.Transport):
    """
    Drop in Transport for xmlrpclib that uses Requests instead of httplib
    """
    # change our user agent to reflect Requests
    user_agent = "Python XMLRPC with Requests (python-requests.org)"

    # override this if you'd like to https
    use_https = False

    def request(self, request_body):
        """
        Make an xmlrpc request.
        """
        headers = {'User-Agent': self.user_agent}
        """ XAPI listens on port 80 of localhost """
        url = "http://127.0.0.1:80"
        try:
            resp = requests.post(url, data=request_body, headers=headers)
        except ValueError:
            raise
        except Exception:
            raise # something went wrong
        else:
            try:
                resp.raise_for_status()
            except requests.RequestException as e:
                raise xmlrpc.ProtocolError(url, resp.status_code,
                                                        str(e), resp.headers)
            else:
                return self.parse_response(resp)

    def parse_response(self, resp):
        """
        Parse the xmlrpc response.
        """
        p, u = self.getparser()
        p.feed(resp.text)
        p.close()
        return u.close()


""" This class forwards incoming request to xapi """
class Forward_request:
    def __init__(self):
        self.forward = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def start(self, host, port):
        try:
            self.forward.connect((host, port))
            return self.forward
        except Exception, e:
            logging.error('Exception:%s ' % e)
        return False

"""This class responsible for access conttol, it validates xapi requests and decides whether to forward or reject requests"""
class AccessControl:
    def __init__(self):
        self.data = None

    """ This function forms xml input for xmlrpc request to xenapi, makes xmlrpc request and parses response"""
    def call_xenapi(self, method_name, session_ref, ref):
        input_params = (session_ref, ref)
        data = lib.dumps(input_params, method_name)
        req = RequestsXAPI()
        res = req.request(data)
        if len(res) > 0:
            if res[0].has_key('Value'):
                return res[0]['Value']
            else:
                raise AccessControlException("Xenapi call failed")
        else:
            raise AccessControlException("Xenapi call failed")

    """ This function parses input request data to get session_ref and vm_ref """
    def get_session_and_vm_ref_from_request_data(self):
        data = self.data
        loc=data.find("<?xml")
        xmlstr=data[loc:]
        xmldoc = minidom.parseString(xmlstr)
        itemlist = xmldoc.getElementsByTagName("string")
        session_ref = itemlist[0].firstChild.data
        vm_ref = itemlist[1].firstChild.data
        return (session_ref,vm_ref)

    """ This function calls xenapi-VM.get_other_config, to check if is_measured flag is set"""
    def check_is_measured_flag(self, vm_ref, session_ref):
        value = self.call_xenapi('VM.get_other_config', session_ref, vm_ref)
        is_measured = value['is_measured']
        return is_measured

    """ This function retrieves kernel, ramdisk, disk and manifest file from vm_ref, and calls RPCore function 'get_rpcore_decision' and 'map_rpid_uuid' """
    def measure_vm(self, vm_ref, session_ref):
        vbd_refs = self.call_xenapi('VM.get_VBDs', session_ref, vm_ref)
        vbd_ref = vbd_refs[0]
        vm_uuid = self.call_xenapi('VM.get_uuid', session_ref, vm_ref)
        vdi_ref = self.call_xenapi('VBD.get_VDI', session_ref, vbd_ref)
        vdi_uuid = self.call_xenapi('VDI.get_uuid', session_ref, vdi_ref)
        sr_ref = self.call_xenapi('VDI.get_SR', session_ref, vdi_ref)
        sr_uuid = self.call_xenapi('SR.get_uuid', session_ref, sr_ref)


        """ SR_MOUNT_PATH is taken from configuratin file /etc/intel_rpcore_plugin.conf   This is path where sr volume is mounted on system, by default it is /run/sr-mount"""
        sr_path = CONFIG['SR_MOUNT_PATH']+sr_uuid
        """ VHD filename is same as vdi uuid of that VM with '.vhd' extension"""
        vhd_filename = vdi_uuid+'.vhd'
        """ Manifest filename is also same as vdi uuid of that VM with '.manifest' extension"""
        manifest_filename = vdi_uuid+'.manifest'
        """ Both VHD file and Manifest file resides on SR in same directory"""
        manifest_filepath = sr_path+"/"+manifest_filename
        vhd_file_path = sr_path+"/"+vhd_filename
        """ Here vhd-util query is fired to find parent vhd of input vhd file, When caching is used, and same Image is launched twice then in second attempt child vhd file gets created with points to parent vhd file, parent vhd file is one downloaded in first attempt, for imvm verification, we need to send parent vhd which actually contains files for that VM """
        parent = Popen(["/usr/bin/vhd-util","query","-p", "-n", vhd_file_path], stdout=PIPE).stdout.read()
        if "has no parent" in parent:
            vhd_for_verification = vhd_file_path
        else:
            vhd_for_verification = parent

        kernel = self.call_xenapi('VM.get_PV_kernel', session_ref, vm_ref)
        ramdisk = self.call_xenapi('VM.get_PV_ramdisk', session_ref, vm_ref)

        logging.debug("kernel: %s" % kernel)
        logging.debug("ramdisk: %s " % ramdisk)
        logging.debug("manifest: %s" % manifest_filepath)
        logging.debug("vhd: %s" % vhd_for_verification)

        """ RPCore call-get_rpcore_decision is made with rpclient module which returns rpid"""
        try:
            rpcore_response_id = rpclient.get_rpcore_decision(kernel, ramdisk, vhd_for_verification, manifest_filepath)
        except Exception as e:
            logging.debug('Exception:%s ' % e)
            return False

        logging.debug('rpcore response is %s' % (str(rpcore_response_id)))
        if rpcore_response_id <= 0:
            allow = False
            logging.debug("RPCORE Measurement Failed")
        else:
            allow = True    
            logging.debug("RPCORE Measurement Success")
            try:
                rp_domid = rpcore_response_id
                """ RPCore call-.map_rpid_uuid is made with rpclient module, This function keeps mapping of rp_domid, vm_uuid and vdi_uuid"""
                rpclient.map_rpid_uuid(rp_domid, vm_uuid, vdi_uuid)
            except Exception as e:
                logging.debug('Exception:%s ' % e)


        return allow
        
    """ This function parse input xml for VBD.create xenapi call, it returns session_ref, vdi_ref and vm_ref"""
    def parse_vbd_create_call(self):
        loc=self.data.find("<methodCall>")
        xmlstr=self.data[loc:]
        xmldoc = minidom.parseString(xmlstr)
        itemlist = xmldoc.getElementsByTagName("member")

        for s in itemlist :
            p = s.toxml()
            if "<name>VDI</name>" in p:
                loc1 = p.find("OpaqueRef:")
                loc2 = p.find("</string>")
                vdi_ref = p[loc1:loc2]

            if "<name>VM</name>" in p:
                loc1 = p.find("OpaqueRef:")
                loc2 = p.find("</string>")
                vm_ref = p[loc1:loc2]

        item = xmldoc.getElementsByTagName("string")
        session_ref = item[0].firstChild.data
        return (session_ref, vdi_ref, vm_ref)

    """ This function checks if vdi in put request is already been used/mapped to measured VM, RPCore function: check_vm_vdi is called, if it returns value 1, dont allow vbd create"""
    def validate_vbd_create_call(self):
        values = self.parse_vbd_create_call()
        session_ref = values[0]
        vdi_ref = values[1]
        vm_ref = values[2]

        vdi_uuid = self.call_xenapi('VDI.get_uuid', session_ref, vdi_ref)
        vm_uuid = self.call_xenapi('VM.get_uuid', session_ref, vm_ref)

        try:
            rpcore_res = rpclient.check_vm_vdi(vm_uuid, vdi_uuid)
        except Exception as e:
            logging.debug('Exception:%s ' % e)
            return False

        if rpcore_res == 0:
            logging.debug("VBD create allowed")
            return True
        elif rpcore_res == 1:
            logging.debug("VBD create not allowed")
            return False
        else:
            return False
    """ This function checks if consecutive request come for VM start, When RPCore denies to start VM, nova compute resends VM.start_on request, which is identified here"""
    def check_repeat_request(self, vm_ref):
        global vm_ref_list
        if len(vm_ref_list) == 0:
            vm_ref_list.append(vm_ref)
            return False
        else:
            for vm in vm_ref_list:
                if vm == vm_ref:
                    return True
            
        vm_ref_list.append(vm_ref)
        return False

    """" This function removes vm_ref from vm_ref_list filled"""
    def remove_from_vm_ref_list(self, vm_ref):
        global vm_ref_list
        for vm in vm_ref_list:
            if vm == vm_ref:
                vm_ref_list.remove(vm)
                break
            

    """ This function is called for every xenapi request made from nova compute, It first checks for which xenapi request is made, and calls respective function to decide if its safe to forward request to XAPI, if it founds its not safe then request is not forwarded to XAPI and custom error response is sent to Nova Compute  """
    def check_input_request_data(self, data):
        self.data = data
        allow = True
        if "VM.start_on" in data:
             values = self.get_session_and_vm_ref_from_request_data()
             session_ref = values[0]
             vm_ref = values[1]
             repeat = self.check_repeat_request(vm_ref)
             if repeat == False:
                 is_measured = self.check_is_measured_flag(vm_ref, session_ref)
                 if is_measured=="True":
                    allow = self.measure_vm(vm_ref, session_ref)
                 else:
                    allow = True
             else:
                allow = False  

        elif "VBD.create" in data:
             allow = self.validate_vbd_create_call()

        elif "VM.destroy" in data:
             values = self.get_session_and_vm_ref_from_request_data()
             session_ref = values[0]
             vm_ref = values[1]
             vm_uuid = self.call_xenapi('VM.get_uuid', session_ref, vm_ref)
             try:
                rpclient.delete_uuid(vm_uuid)
             except Exception as e:
                logging.debug('Exception:%s ' % e)
 
             logging.debug("RPClient delete uuid called for VM : %s" % vm_uuid)
             self.remove_from_vm_ref_list(vm_ref)
    
        elif "VM.clean_reboot" in data or "VM.reboot" in data or "VM.hard_reboot" in data:
            values = self.get_session_and_vm_ref_from_request_data()
            session_ref = values[0]
            vm_ref = values[1]
            self.remove_from_vm_ref_list(vm_ref)
            repeat = self.check_repeat_request(vm_ref)
            if repeat == False:
                is_measured = self.check_is_measured_flag(vm_ref, session_ref)
                if is_measured=="True":
                    power_state = self.call_xenapi('VM.get_power_state', session_ref, vm_ref)
                    if power_state == "Running":
                        allow = self.measure_vm(vm_ref, session_ref)
                        if allow == False:
                            self.call_xenapi('VM.hard_shutdown', session_ref, vm_ref)
                            logging.debug("VM Verification Failed: Shutting down VM")
                    else:
                        allow = False
                else:
                    allow = True
            else:
                allow = False

        elif "VM.clean_shutdown" in data or "VM.hard_shutdown" in data:
            values = self.get_session_and_vm_ref_from_request_data()
            session_ref = values[0]
            vm_ref = values[1]
            self.remove_from_vm_ref_list(vm_ref)
 

        return allow

""" This class is proxy server_socket code which creates server_socket socket and listens(0.0.0.0:8080), and forwards request to destination(127.0.0.1:80)"""
class ProxyServer:
    input_request_list = []
    channel = {}
    session = None
    def __init__(self, host, port):
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((host, port))
        self.server_socket.listen(5)

    """ In this function, The input_request_list stores all the available sockets, in while loop every request is served one by one, whenever a new connection is received it is passed to accept_connection method, which manages endpoints. And if its not new connection then it is incoming request data, It can be from Nova_compute_client or xapi. If Length of data is 0 then its close connection request, It is handled by close_connection method. else input data is passed to receive_request method, this method parses input data and decides whether to forward/block request. and passes response to appropriate endpoints."""
    def handle_requests(self):
        accessControl = AccessControl()
        self.input_request_list.append(self.server_socket)
        while 1:
            time.sleep(delay)
            ss = select.select
            input_request, outputready, exceptready = ss(self.input_request_list, [], [])
            for self.s in input_request:
                if self.s == self.server_socket:
                    self.accept_connection()
                    break
                try:
                    self.data = self.s.recv(buffer_size)
                except Exception as e:
                    logging.debug('Exception:%s ' % e)

                if len(self.data) == 0:
                    self.close_connection()
                else:
                    self.receive_request(accessControl)

    """ This method creates a new connection with the XAPI. (proxy -> XAPI), and accepts the current nova_compute_client connection (nova_compute_client->proxy). Both of these connection appended to input list, clientsock is nova_client connection and forward is xapi connection. channel maintains actual endpoints, that is Nova_client and XAPI"""
    def accept_connection(self):
        forward = Forward_request().start(forward_to[0], forward_to[1])
        clientsock, clientaddr = self.server_socket.accept()
        global fw
        fw = forward
        if forward:
            logging.debug("Connected: %s",  clientaddr)
            self.input_request_list.append(clientsock)
            self.input_request_list.append(forward)
            self.channel[clientsock] = forward
            self.channel[forward] = clientsock
        else:
            logging.debug("Can't establish connection with remote server_socket")
            logging.debug("Closing connection with client side: %s", clientaddr)
            clientsock.close()

    """ Disables and removes the socket connection between the proxy and the XAPI and the one between the Nova_compute_client and the proxy itself. """
    def close_connection(self):
        #logging.debug("Disconnected : %s", self.s.getpeername())
        #remove objects from input_request_list
        self.input_request_list.remove(self.s)
        self.input_request_list.remove(self.channel[self.s])
        out = self.channel[self.s]
        # close the connection with client
        self.channel[out].close()  # equivalent to do self.s.close()
        # close the connection with remote server_socket
        self.channel[self.s].close()
        # delete both objects from channel dict
        del self.channel[out]
        del self.channel[self.s]

    """ This function is called when Proxy does not forward input request to XAPI and it sends custom error response back to nova-compute"""
    def response_for_error(self, resp_string):
        response_body = "<methodResponse><params><param><value><struct><member><name>Status</name><value>Success</value></member><member><name>Value</name><value>"+resp_string+"</value></member></struct></value></param></params></methodResponse>"
        response_body_raw = ''.join(response_body)

        response_headers = {
            'Content-Type': 'text/html; encoding=utf8',
            'Content-Length': len(response_body),
            'Connection': 'keep-alive',
            'cache-control':'no-cache, no-store',
            'content-type':'text/xml',
            'Access-Control-Allow-Headers':'X-Requested-With'
        }

        response_headers_raw = ''.join('%s: %s\n' % (k, v) for k, v in \
                                                response_headers.iteritems())

        # Reply as HTTP/1.1 server_socket, saying "HTTP OK" (code 200).
        response_proto = 'HTTP/1.1'
        response_status = '200'
        response_status_text = 'OK' # this can be random

        # sending all this stuff
        self.channel[fw].send('%s %s %s' % (response_proto, response_status, response_status_text))
        self.channel[fw].send(response_headers_raw)
        self.channel[fw].send('\n') # to separate headers from body
        self.channel[fw].send(response_body_raw)
        # ad closing connection, as we stated before
        logging.debug("ERROR Response : %s" % response_body)
        self.close_connection()

    """ This method is used to process and forward the data to the original destination ( Nova_client <- proxy -> XAPI ). """
    def receive_request(self, accessControl):
        """ access control function are called to validate input request """
        allow = accessControl.check_input_request_data(self.data)
        """ If allow is true then forward request to intended destination else it wont forward request to XAPI and will send error response back to Nova Compute"""
        if allow:
            self.channel[self.s].send(self.data)
        else:
            self.response_for_error("RPCORE_DENIED")


if __name__ == '__main__':
    server_socket = ProxyServer('', 8080)
    try:
        server_socket.handle_requests()
    except KeyboardInterrupt:
        logging.debug("Ctrl C - Stopping server_socket")
        sys.exit(1)
                                 
