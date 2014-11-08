import rpclient
import time

for i in range(0,10):
    print "hi"
    result = rpclient.get_rpcore_decision("/tmp/rpimg/kernel", "/tmp/rpimg/ramdisk")
    time.sleep(2)
    print result




