import logging.config
import ConfigParser
import socket
import sys
from Queue import Queue
from config import section, port, max_queue_length, max_observer,\
    max_waiting_client, vm_meta_info_path, RPCORE_IPADDRESS, RPCORE_PORT, RP_ID, \
    VM_IMAGE_ID, VM_CUSTOMER_ID, VM_MANIFEST_HASH, VM_MANIFEST_SIGNATURE

from observer import Observer
from com.intel.rpcore.client.rpclient import RPClient
from attestation import Attestation

# Initialize logging
logging.config.fileConfig(fname="vm_attestation_agent_logging.cfg")
# Load Configuration
raw_parser = ConfigParser.RawConfigParser()
raw_parser.read("config.cfg")

# Initialize variables
listening_port = raw_parser.getint(section, port)
_max_queue_length = raw_parser.getint(section, max_queue_length)
_max_observer = raw_parser.getint(section, max_observer)
_max_waiting_client = raw_parser.getint(section, max_waiting_client)
_vm_meta_info_path = raw_parser.get(section, vm_meta_info_path)

# Log cfg values
logging.debug("%s=%d" % (port, listening_port))
logging.debug("%s=%d" % (max_queue_length, _max_queue_length))
logging.debug("%s=%d" % (max_observer, _max_observer))
logging.debug("%s=%d" % (max_waiting_client, _max_waiting_client))
logging.debug("%s=%s" % (vm_meta_info_path, _vm_meta_info_path))

# Initialize Queue
queue = Queue(maxsize=_max_queue_length)

#Initialize RPClient 
d = {}
try :
    with open(_vm_meta_info_path) as f:
	for line in f:
            delimiter = "="
            index = line.index(delimiter)
            key = line[0:index]
            val = line[index+1:]
            d[key.strip()] = val.strip()
except Exception:
    logging.exception("")
    exit(1)
        
if d[RPCORE_IPADDRESS] is None or d[RPCORE_PORT] is None or d[RP_ID] is None :
    logging.error("RPCore IP Address or port or RPID is not available ")
    exit(1)

if d[VM_IMAGE_ID] is None or d[VM_CUSTOMER_ID] is None or d[VM_MANIFEST_HASH] is None or d[VM_MANIFEST_SIGNATURE] is None:
    logging.error("VM_IMAGE_ID or VM_CUSTOMER_ID or VM_MANIFEST_HASH is not available ")
    exit(1)    

configuration={'VM_IMAGE_ID':d[VM_IMAGE_ID],'VM_CUSTOMER_ID':d[VM_CUSTOMER_ID],'VM_MANIFEST_HASH':d[VM_MANIFEST_HASH],'VM_MANIFEST_SIGNATURE':d[VM_MANIFEST_SIGNATURE]}    
# Initialize observer 
for i in range(1, (_max_observer+1)):
    rp_client = RPClient(rpcore_ip_address=d[RPCORE_IPADDRESS], rpcore_port=int(d[RPCORE_PORT]), dom_id=d[RP_ID])
    attest = Attestation(rp_client=rp_client, configuration=configuration)
    observer = Observer(attest)
    observer.setName("observer %s" %(i))
    observer.setQueue(queue)
    observer.setDaemon(True)
    observer.start()
    
# Initialize Server socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((socket.gethostname(), listening_port))
server_socket.listen(_max_waiting_client)
server_socket.setblocking(True)
logging.info("server is listening on %s at port %d" %(socket.gethostname(),listening_port))

while True :
    try: 
	socket, addr = server_socket.accept()
        queue.put_nowait(socket)
    except KeyboardInterrupt:
        logging.debug("Ctrl C - Stopping server_socket")
        server_socket.close()
        sys.exit(0)
    except Exception :
        logging.exception("")   
     

