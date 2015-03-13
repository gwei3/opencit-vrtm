from socket import socket as Socket
from _socket import SHUT_RDWR
from type import method_name_to_rpapicode
import socket
import sys
import xmlrpclib
import logging
import errno
from stream import RPStream
from stream import TCBuffer

class RPClient(object):
    QUOTETYPESHA256FILEHASHRSA1024 = 3
    QUOTETYPESHA256FILEHASHRSA2048 = 4	
    def __init__(self, rpcore_ip_address=None, rpcore_port=16005, connection_time_out=20, dom_id='100'):
        '''
        Initialize the RPClient to default values
        
        '''
        if rpcore_ip_address is None:
            rpcore_ip_address = socket.gethostname();
            
        self.__rpcore_ip_address = rpcore_ip_address
        self.__rpcore_port = rpcore_port
        self.__connection_time_out = connection_time_out
        
        self.__dom_id = dom_id
        self.is_connected = False
        self.__rp_stream = RPStream()
        
        
        self.__socket = Socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        socket.setdefaulttimeout(self.__connection_time_out)
        self.__tc_buffer = TCBuffer()
        self.retry = 3
    
        try : 
            self.__logger = logging.getLogger(__name__)
        except Exception as e :
            self.__logger = logging

        self.__logger.debug(self.__dict__)
        
    def reset_dom_id(self, dom_id):
            self.__dom_id = dom_id
    
    def send(self, tcbuffer, method_name, *method_parameter):
            err =  self.__socket.connect_ex((self.__rpcore_ip_address, self.__rpcore_port))
            if err == 0 and not(self.is_connected):
                # send dom_id to RPCore
                self.is_connected = True
                self.__logger.debug("Registered the dom id %s" % (self.__dom_id))
                self.__send(self.__socket, self.__rp_stream.pack_rpcore_dom_id(self.__dom_id))
            else :
                self.__logger.debug("Already connected")

            xml_string = xmlrpclib.dumps( tuple(method_parameter), method_name);
            if tcbuffer is None:
                rpapi_code = method_name_to_rpapicode[method_name]
                if rpapi_code is None:
                    self.__logger.error("RPCore API code for method %s not found" % (method_name))
                    raise Exception("RPCore API code for method %s not found" % (method_name))
                self.__tc_buffer.set_m_reqID(rpapi_code)
                tcbuffer = self.__tc_buffer
                    
            tcbuffer.set_m_reqSize(len(xml_string))
            self.__logger.info("xml rpc %s and it's length %d" % (xml_string, len(xml_string)))
            stream = self.__rp_stream.pack_rpcore_stream(tcbuffer, xml_string, 's') ;
     	    self.__logger.debug("TC Buffer is before send:" +  str(tcbuffer.list()))
            self.__send(self.__socket, stream)
            tcbuffer, stream = self.__recv(self.__socket)
        
            return (tcbuffer, xmlrpclib.loads(stream))
    
    def stream_to_int(self, stream):
            if stream is None:
                return None
            return self.__rp_stream.unpack_rpcore_stream(stream, "i")[0]
        
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
                raise e
            
    def __recv(self, __socket):
            chunk = __socket.recv(TCBuffer.SIZE)
                
            tcbuffer = TCBuffer(*(self.__rp_stream.unpack_rpcore_stream(chunk, TCBuffer.FORMAT)))
        
            message_size = tcbuffer.get_m_reqSize()
            self.__logger.debug("TC Buffer received:" +  str(tcbuffer.list()))
            if message_size == 0 :
               return (tcbuffer, None)

            stream = __socket.recv(message_size)
            if stream is None :
                self.__logger.error("RPCore response is Null ...")
                return (tcbuffer, None)
            
            total_length = len(stream)
            
            while total_length < message_size:
                stream = stream + __socket.recv(message_size - total_length)
                total_length = len(stream)

            return (tcbuffer, stream)

    def close(self):
        try:
            self.__socket.shutdown(SHUT_RDWR);
            self.__socket.close()
            self.__logger.debug("RPCore connection is closed")
            
        except Exception as e:
            self.__logger.exception("")
        finally:
            self.is_connected = False
            self.__socket = Socket(socket.AF_INET, socket.SOCK_STREAM, 0)   
            
            
            
            

