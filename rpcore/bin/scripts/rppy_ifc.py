#!/usr/bin/python

#refereces: https://wiki.python.org/moin/XmlRpc

import xmlrpclib
import base64

class rpc_encoder:
	
	def __init__(self):
		self.args = ()
    
	def get_xml(self, method, response = 0):
		if response == 0:
			xmlstr = xmlrpclib.dumps( self.args, method)
		else:
			xmlstr = xmlrpclib.dumps( self.args[:1], methodresponse=1)
			
		print xmlstr 
		return xmlstr

	def Int(self, i):
		self.args = self.args + (i,)
	
	def String(self, s):
		self.args = self.args + (s,)
	
	def Buf(self, buf):
		b64 = base64.b64encode(buf)
		self.args = self.args + (b64,)
		
	def encode_call(self, method, buf):
		self.Buf(buf)
		rc = self.get_xml(method)
		print rc
		return rc
	
	def encode_response(self, method, buf):
		self.Buf(buf)
		rc = self.get_xml("", 1)
		return rc
	
class rpc_decoder:
	def __init__(self):
		print "Initializing rpc_decoder\n"
		self.args = ()
		self.length = 0;
		
	def size(self):
		return self.length
	
	def items(self, i):
		if ( i < self.length ):
			return self.args[i]
		else:
			return null
			
	def xmlrpc_buf(self, strxml):
#		print strxml
		data = xmlrpclib.loads(strxml)
#		print data[0][0]
		buf = base64.b64decode( data[0][0] )
		return buf

	def xmlrpc_args(self, strxml):
#		print "in function xmlrpc_args"
		if  xmlrpclib is None:
			print "xmlrpc is null"
		data = xmlrpclib.loads(strxml)
		self.name = data[1]
		self.length = len(data[0])
		self.args = self.args + data[0]
#		print data 
		return data[1]

def foo():
	re = rpc_encoder()
	rd = rpc_decoder()
	
#	re.Int(10)
	s = "nanoo takalu"
#	re.String(s)
	
	b =  buffer(s)
	
	xmlstr = re.encode_call("dabba", b)
	#rd.xmlrpc_buf(xmlstr)
	
	#xmlstr = re.encode_response("dabba", b)
	print rd.xmlrpc_args(xmlstr)	
	print rd.size()
	print rd.items(0)
	
	print "done"

if __name__ == "__main__":
	foo()
