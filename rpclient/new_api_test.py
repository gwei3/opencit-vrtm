import logging
import logging.config

class MeasuredLaunch(object):
    def __init__(self, rp_client):
        self.__rp_client = rp_client
        self.__logger = logging.getLogger(__name__)
        
    def get_rpid(self,uuid):

        tc_buffer,stream = self.__rp_client.send(None,'get_rpid',base64.b64encode(uuid))
        return base64.b64decode(stream[0][0])

    def get_vmmeta(self,rpid):
        tcbuffer,stream = self.__rp_client.send(None,'get_vmmeta',base64.b64encode(rpid))
        return stream[0]
    def get_verification_status(self,uuid):
        tcbuffer,stream = self.__rp_client.send(None,'get_verification_status',base64.b64encode(uuid))
        return base64.b64decode(stream[0][0])

if __name__ == "__main__" :
   from com.intel.rpcore.client.rpclient import RPClient    
   import sys  

   rpclient = RPClient(sys.argv[1] , 16005 , connection_time_out = 420, dom_id="100")

   ml = MeasuredLaunch( rpclient ) 
   import base64 

   rp_id = ml.get_rpid(sys.argv[2])

   print "========================================\n"
   print "Received RP ID =  %s\n" % (rp_id)
   t = ml.get_vmmeta(rp_id)
   print "\nReceived VM META = ", t
   status = ml.get_verification_status(sys.argv[2])
   print "\n verification status = %s" %(status)
   print "\n==========================================" 








