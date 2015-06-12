import logging


class MeasuredLaunch(object):
    def __init__(self, rp_client):
        self.__rp_client = rp_client
        self.__logger = logging.getLogger(__name__)
        
    def get_rpcore_decision(self, kernel_file, ramdisk_file, disk_file, manifest_file):
        try:
            tc_buffer, stream = self.__rp_client.send(None, 'foo', 9, './vmtest', '-kernel', kernel_file, '-ramdisk', ramdisk_file, '-disk', disk_file, '-manifest', manifest_file)
            
            if tc_buffer is None or stream is None :
                self.__logger.error("Measured launch has been failed..")
                return -1
            self.__logger.info("TCBuffer = %s" % (tc_buffer.list()))
                
        except Exception:
            self.__logger.exception("")
        
        return self.__rp_client.stream_to_int(stream) 
    
    def map_rpid_uuid(self, rpid, uuid):
        
        try:
            
            tcbuffer, stream = self.__rp_client.send(None, "set_vm_uuid", 2, rpid, uuid )
            
            if tcbuffer is None or stream is None :
                self.__logger.error("Mapping failed...")
                return -1

        except Exception:
            self.__logger.exception("")
        
        return self.__rp_client.stream_to_int(stream)
        
    def delete_uuid(self, uuid):
        """
            Delete the vm uuid from RPCore.
            This method returns the status returned from RPCore.
            match this status to type.Status value to get to know what has happened on RPCore side.
        """
        try:
            tcbuffer, stream = self.__rp_client.send(None, "delete_vm", uuid )
            
            if tcbuffer is None or stream is None :
                self.__logger.error("Deletion Failed...")
                return -1
            
        except Exception as e:
            self.__logger.exception("")
            raise e

        return self.__rp_client.stream_to_int(stream) 
