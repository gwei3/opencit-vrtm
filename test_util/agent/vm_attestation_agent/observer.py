import logging 
from threading import Thread

class Observer(Thread):
    MESSAGE_SIZE = 4096
    
    def __init__(self, attest):
        super(Observer, self).__init__()
        self.__logger = logging.getLogger(__name__)     
        self.__attestation = attest
        
    def setQueue(self, queue):
        self.__queue = queue

    def run(self):
        self.__logger.debug("Inside observer %s" % (self.getName()))
        
        while True :
            socket = self.__queue.get(block=True)
            challenge = self.__recv(socket)
            
            if  not ( challenge is None ) :
                quote = self.__attestation.get_quote(challenge)
                self.__send(socket, quote)
            else :
                self.__logger.warn("%s : challenge is not received "  % (self.getName()))
                
            socket.close()
            
        self.__logger.debug("Exit from observer %s" % (self.getName()))

    def __send(self, __socket, stream):
        totalsent = 0
        length = len(stream)
        try: 
            while totalsent < length :
                sent = __socket.send(stream[totalsent:])
                if sent == 0:
                    raise RuntimeError('Connection is broken....')
                totalsent = totalsent + sent
        except Exception:
            self.__logger.exception("")
            
    def __recv(self, __socket):
        
        try:
            __socket.settimeout(1) 
            stream = __socket.recv(Observer.MESSAGE_SIZE)
            return stream
        except Exception:
            self.__logger.exception("")
