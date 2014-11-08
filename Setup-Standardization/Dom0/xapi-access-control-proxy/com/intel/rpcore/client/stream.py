from struct import Struct
from array  import array
from type import Satus
from type import RPAPICode
class RPStream(object):
    def __format(self, msg_length=None, msg_data_type=None):
        if msg_length is None or msg_data_type is None:
            return TCBuffer.FORMAT
        
        if msg_data_type == 's':
            return TCBuffer.FORMAT + str(msg_length) + msg_data_type
        
        return TCBuffer.FORMAT + msg_data_type
    
    def pack_rpcore_dom_id(self, dom_id):
        """
            RPCore need '\0' terminated dom_id before starting any type of
            communication.
        """
        stream = array('B', '\0' * (len(dom_id)+1) )
        struct_obj = Struct(str(len(dom_id)) + 's' )
        struct_obj.pack_into(stream, 0, dom_id)
        
        return stream
        
    def pack_rpcore_stream(self, tcbuffer, message, message_data_type):
        """ 
          Converts the TCBuffer object and message into RPCore stream
        """  
        buffer_ = array('B', '\0' * (tcbuffer.message_size()+TCBuffer.SIZE))
        struct_obj = Struct(self.__format(tcbuffer.message_size(), message_data_type))
        
        arguments = [0] + tcbuffer.list() + [message]
        
        struct_obj.pack_into(buffer_, *arguments)
        
        return buffer_
    
#     def unpack_rpcore_stream(self, stream, message_data_type):
#         """
#           Converts the RPCore stream into python objects.
#           It returns a tuble containing TCBuffer object and message
#         """
#         # calculate message size
#         _buffer = array('B', stream)
#         message_length = _buffer.buffer_info()[1] - TCBuffer.SIZE
#           
#         struct_obj = Struct(self.__format(message_length, message_data_type))
#         
#         result = struct_obj.unpack_from(stream)
#         
#         tcbuffer, message = TCBuffer(*result[0:len(TCBuffer.FORMAT)]) , result[len(TCBuffer.FORMAT)]
#         
#         return (tcbuffer, message)
    
    def unpack_rpcore_stream(self, chunk, data_format):
        _buffer = array('B', chunk)
        
        struct_obj = Struct(data_format)
        
        return struct_obj.unpack_from(_buffer)
        
    def unpack_tcbuffer(self, chunk):
        buffer_ = array('B',chunk)
        struct_obj = Struct(TCBuffer.FORMAT)
        return TCBuffer(* (struct_obj.unpack_from(buffer_)) )

class TCBuffer(object):
    """
        Equivalent to C tcBuffer structure of RPCore
    """
    # default formate for RPStream is 'iIIIis' where i for int , I for unsigned
    # int and s for char[].
    FORMAT = 'iIIIi'  
    # sizeof(tcbuffer)
    SIZE=20
    
    def __init__(self, m_procid = 0, m_reqID = RPAPICode.VM2RP_STARTAPP, 
                       m_reqSize = 0, 
                       m_ustatus = 0,
                       m_origprocid = 0):
        """
            m_reqID and m_reqSize are mendatory arguments for making rpcore request.
            
        """
        self.__m_procid = m_procid 
        self.__m_reqID = m_reqID
        self.__m_reqSize = m_reqSize
        self.__m_ustatus = m_ustatus
        self.__m_origprocid = m_origprocid
    
    # don't change the position of attribute, this list is in sequence of C tcbuffer structure
    # position change may affect the communication with RPCore    
    def list(self):
        return [self.__m_procid,
                self.__m_reqID,
                self.__m_reqSize,
                self.__m_ustatus,
                self.__m_origprocid
                ]
        
    def message_size(self):
        return self.__m_reqSize
    
    def message_process_id(self):
        return self.__m_procid

    def message_id(self):
        return self.__m_reqID

    def message_status(self):
        return self.__m_ustatus
    
    def message_orig_procid(self):
        return self.__m_procid

        
        
if __name__ == '__main__':
    
    tcbuffer = TCBuffer(m_reqSize=len('I am comming'),m_ustatus=120)
    
    rpstream = RPStream()
    
    buffer_ = rpstream.pack_rpcore_stream(tcbuffer, 'I am comming')
    
    tcbuffer, message = rpstream.unpack_rpcore_stream(buffer_)
    
    print tcbuffer.list()

    print message

