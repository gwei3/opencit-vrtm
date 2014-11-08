import socket
import xmlrpclib
import logging
import os

from socket import socket as Socket
from stream import RPStream
from stream import TCBuffer
from type import RPAPICode
class RPClient(object):
    '''
        Provides mechnism to communicate with RPCore
    
    '''
    def __init__(self, rpcore_ip_address=None, rpcore_port=16005, connection_time_out=60, dom_id='100'):
        '''
        Initialize the RPClient to default values
        
        '''
        if rpcore_ip_address is None:
            rpcore_ip_address = socket.gethostname();
            
        self.__rpcore_ip_address = rpcore_ip_address
        self.__rpcore_port = rpcore_port
        self.__connection_time_out = connection_time_out
        
        self.__dom_id = dom_id
        self.__rp_stream = RPStream()
        
        self.LOG = logging.getLogger(__name__)
        socket.setdefaulttimeout(self.__connection_time_out)
        
        self.LOG.debug(self.__dict__)
    
    # Need to do white listing of arguments        
    def get_rpcore_decision(self, kernel_file, ramdisk_file, disk_file, manifest_file):
        """
            This method return the rpcore id.
            RPCore id is started from 1000.
        """
        try:

            __socket = Socket(socket.AF_INET, socket.SOCK_STREAM,0)
            __socket.settimeout(500)        
            __socket.connect((self.__rpcore_ip_address, self.__rpcore_port))
            # send dom_id to RPCore
            self.__send(__socket, self.__rp_stream.pack_rpcore_dom_id(self.__dom_id))
            # this is the request to RPCore for making measured decision
            if kernel_file is '' and ramdisk_file is '':
		        params = (5, './vmtest', '-disk', disk_file, '-manifest', manifest_file)
    	    else:
	            params = (9, './vmtest', '-kernel', kernel_file, '-ramdisk', ramdisk_file, '-disk', disk_file, '-manifest', manifest_file)
            method_name = "foo"
            # create xml representation
            xml_string = xmlrpclib.dumps(params, method_name);
            # At the time of sending request to RPCore m_reqID and m_ReqSize is mendatory
            tcbuffer = TCBuffer(m_reqSize=len(xml_string))
        
            stream = self.__rp_stream.pack_rpcore_stream(tcbuffer, xml_string, 's') ;
        
            self.__send(__socket, stream)
        
            # receive data from rpcore
            tcbuffer, stream = self.__recv(__socket)
            # need to do error checking on return tcbuffer
            __socket.close();
        except Exception as e:
            __socket.close()
            self.LOG.error(e)
            raise e   
        
        return self.__check_for_none(tcbuffer, stream)[1] 
    
    def map_rpid_uuid(self, rpid, uuid, vdi_uuid):
        try:
            __socket = Socket(socket.AF_INET, socket.SOCK_STREAM,0)
            __socket.settimeout(500)
            __socket.connect((self.__rpcore_ip_address, self.__rpcore_port))
            # send dom_id to RPCore
            self.__send(__socket, self.__rp_stream.pack_rpcore_dom_id(self.__dom_id))
            # this is the request to RPCore for making measured decision
            params = (3, rpid, uuid, vdi_uuid)
            method_name = "set_vm_uuid"
            # create xml representation
            xml_string = xmlrpclib.dumps(params, method_name);
            # At the time of sending request to RPCore m_reqID and m_ReqSize is mendatory
            tcbuffer = TCBuffer(m_reqID=RPAPICode.VM2RP_SETUUID, m_reqSize=len(xml_string))
           
            stream = self.__rp_stream.pack_rpcore_stream(tcbuffer, xml_string, 's') ;

            self.__send(__socket, stream)
            # receive data from rpcore
            tcbuffer, stream = self.__recv(__socket)
            # need to do error checking on return tcbuffer
            __socket.close()
        except Exception as e:
            __socket.close()
            self.LOG.error(e)
            raise e
        
        return self.__check_for_none(tcbuffer, stream)

    def check_vm_vdi(self, uuid, vdi_uuid):
    	try:
            __socket = Socket(socket.AF_INET, socket.SOCK_STREAM,0)
            __socket.settimeout(500)
            __socket.connect((self.__rpcore_ip_address, self.__rpcore_port))
            # send dom_id to RPCore
            self.__send(__socket, self.__rp_stream.pack_rpcore_dom_id(self.__dom_id))
            # this is the request to RPCore for making measured decision
            params = (2, uuid, vdi_uuid)
            method_name = "map_vm_vdi"
            # create xml representation
            xml_string = xmlrpclib.dumps(params, method_name);
            # At the time of sending request to RPCore m_reqID and m_ReqSize is mendatory
            tcbuffer = TCBuffer(m_reqID=RPAPICode.VM2RP_CHECK_VM_VDI, m_reqSize=len(xml_string))

            stream = self.__rp_stream.pack_rpcore_stream(tcbuffer, xml_string, 's') ;

            self.__send(__socket, stream)
            # receive data from rpcore
            tcbuffer, stream = self.__recv(__socket)
            # need to do error checking on return tcbuffer
            __socket.close()
        except Exception as e:
            __socket.close()
            self.LOG.error(e)
            raise e

        return self.__check_for_none(tcbuffer, stream)[1]


    def is_measured(self, uuid):
        """
            Delete the vm uuid from RPCore.
            This method returns the status returned from RPCore.
            match this status to type.Status value to get to know what has happened on RPCore side.
        """
        try:
           __socket = Socket(socket.AF_INET, socket.SOCK_STREAM,0)
           __socket.settimeout(500)
           __socket.connect((self.__rpcore_ip_address, self.__rpcore_port))
           # send dom_id to RPCore
           self.__send(__socket, self.__rp_stream.pack_rpcore_dom_id(self.__dom_id))
           # this is the request to RPCore for making measured decision
           params = (uuid,)
           method_name = "is_measured"
           # create xml representation
           xml_string = xmlrpclib.dumps(params, method_name);
           # At the time of sending request to RPCore m_reqID and m_ReqSize is mendatory
           tcbuffer = TCBuffer(m_reqID=RPAPICode.VM2RP_IS_MEASURED, m_reqSize=len(xml_string))

           stream = self.__rp_stream.pack_rpcore_stream(tcbuffer, xml_string, 's') ;

           self.__send(__socket, stream)
           # receive data from rpcore
           tcbuffer, stream = self.__recv(__socket)
           # close socket
           __socket.close()
        except Exception as e:
           __socket.close()
           self.LOG.error(e)
           raise e

        return self.__check_for_none(tcbuffer, stream)[1]

        
    def delete_uuid(self, uuid):
        """
            Delete the vm uuid from RPCore.
            This method returns the status returned from RPCore.
            match this status to type.Status value to get to know what has happened on RPCore side.
        """
        try:
           __socket = Socket(socket.AF_INET, socket.SOCK_STREAM,0)
           __socket.settimeout(500)
           __socket.connect((self.__rpcore_ip_address, self.__rpcore_port))
           # send dom_id to RPCore
           self.__send(__socket, self.__rp_stream.pack_rpcore_dom_id(self.__dom_id))
           # this is the request to RPCore for making measured decision
           params = (uuid,)
           method_name = "delete_vm"
           # create xml representation
           xml_string = xmlrpclib.dumps(params, method_name);
           # At the time of sending request to RPCore m_reqID and m_ReqSize is mendatory
           tcbuffer = TCBuffer(m_reqID=RPAPICode.VM2RP_TERMINATEAPP, m_reqSize=len(xml_string))
        
           stream = self.__rp_stream.pack_rpcore_stream(tcbuffer, xml_string, 's') ;
        
           self.__send(__socket, stream)
           # receive data from rpcore
           tcbuffer, stream = self.__recv(__socket)
           # close socket
           __socket.close()
        except Exception as e:
           __socket.close()
           self.LOG.error(e)
           raise e

        return self.__check_for_none(tcbuffer, stream)
            
    
    def __send(self, __socket, stream):
        totalsent = 0
        _buffer = stream.tostring()
        length = len(_buffer)
        try: 
            while totalsent < length :
                sent = __socket.send(_buffer[totalsent:])
                if sent == 0:
                    raise RuntimeError('Connection is broken....')
                totalsent = totalsent + sent
        except Exception as e:
            print e
            raise e
            
    def __recv(self, __socket):
        try:
            chunk = __socket.recv(TCBuffer.SIZE)
        
            tcbuffer = TCBuffer(*(self.__rp_stream.unpack_rpcore_stream(chunk, TCBuffer.FORMAT)))
        
            message_size = tcbuffer.message_size()

            if message_size == 0 :
                return (tcbuffer, None)

            stream = __socket.recv(message_size)
            
            total_length = len(stream)
            
            while total_length < message_size:
                stream = stream + __socket.recv(message_size - total_length)
                total_length = len(stream)
        
            return (tcbuffer, stream)
        except Exception as e:
            self.LOG.error( 'Failed to receive data from RPCore')
            raise e
        
    def __check_for_none(self, tcbuffer, stream):
        status = -1
        if tcbuffer != None:
            status =  tcbuffer.message_status()
        else:
            print 'TCBuffer not received during deletion'
            
        ret_val = -1
        if stream != None:
            ret_val = self.__rp_stream.unpack_rpcore_stream(stream, 'i')[0]
        else:
            self.LOG.debug( 'ret_val from RPCore is not available....')            
        
        self.LOG.debug( 'RPCore status and return value %s %d' % ( status, ret_val ) )
        
        return (status, ret_val)
        
if __name__ == '__main__':
    rpclient = RPClient('172.30.30.147');
    rpid  =  rpclient.get_rpcore_decision("/root/mohd/images/6c23d296-d4ae-4e2b-9e5c-54596316c368", "/root/mohd/images/81e9fc4c-9210-4814-b829-0ab2f3024098")
    print 'rpid is ' , str(rpid)
    rpclient.map_rpid_uuid(str(rpid), 'dddef85a-651d-ae99-fcd9-719988da2c58')
    rpclient.delete_uuid('dddef85a-651d-ae99-fcd9-719988da2c58')
            
    
    
    
    
