import socket
from threading import Thread 
class T (Thread):
    def run(self):
        sock = socket.create_connection(("127.0.1.1", 9090))
        sock.settimeout(1)
        sock.sendall("<vm_challenge_request> <nonce>1234</nonce> <uuid>f81d4fae-7dec-11d0-a765-00a0c91e6bf6</uuid> </vm_challenge_request>")
        print ( sock.recv(4096))
        

        
i=1
#MAX_QUEUE_LEANGTH=1000
#MAX_OBSERVER=10
#MAX_WAITING_CLIENT=100

# for above values server able to process 4000 request

while i < 2 :
    t = T()
    t.start()
    i = i +1



