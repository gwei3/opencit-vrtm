import StringIO
import binascii
import base64
import logging
import xmlrpclib

from com.intel.rpcore.client.type import QuoteConstant
import socket, struct, fcntl
from Crypto.Hash import SHA256

import lxml.etree as ET
import sys
import ctypes

error = False
class Attestation(object):
    '''
        Provides mechnism to communicate with RPCore
    
    '''
    szdigestAlg = "SHA256"
    g_RpQuotedkeyInfoTemplate = """<QuotedInfo xmlns:ds="http://www.w3.org/2000/09/xmldsig#">
           <ds:CanonicalizationMethod Algorithm="python xml canonicalization"/>
           <ds:QuoteMethod Algorithm="%s"/>
           <HostedCodeDigest alg="%s">%s</HostedCodeDigest>
           <Nonce>%s</Nonce>
           <extdata>%s</extdata>
</QuotedInfo>"""

    g_szRpQuoteTemplate = '<?xml version="1.0" encoding="UTF-8"?>\n' \
'<Quote format="xml" xmlns:ds="http://www.w3.org/2000/09/xmldsig#" Id="//www.rpkss.com/Programs/Unknown">\n' \
'        %s\n' \
'        <QuoteValue>\n' \
'           %s\n' \
'        </QuoteValue>\n' \
'        <HostCert>%s</HostCert>\n' \
'        <EvidenceList>%s</EvidenceList>\n' \
'</Quote>'

    g_vmPolicyTemplate = '<vm_policy> \n' \
	'<signature>%s</signature> \n' \
	'<document> \n' \
	'	<vm_manifest> \n' \
	'		 <customer_id>%s</customer_id> \n' \
	'		 <image_id>%s</image_id> \n' \
	'		 <manifest_hash>%s</manifest_hash> \n' \
	'	</vm_manifest> \n' \
	'</document> \n' \
	'</vm_policy>' 

    g_vm_response_to_mtw = """<vm_challenge_response>
        <rp_quote> %s </rp_quote>
	<rp_certificate>%s</rp_certificate>
	%s
	<tpm_quote> %s </tpm_quote> 
	<aik_certificate> %s </aik_certificate> 
	</vm_challenge_response>"""
	
    
    def __init__(self, rp_client, configuration):
        self.__rp_client = rp_client
        self.__logger = logging.getLogger(__name__)
        self.__configuration = configuration
    
    def __get_prog_hash(self, input_data):
        try:
            tc_buffer, stream = self.__rp_client.send(None, 'get_prog_hash', base64.b64encode(input_data))
            self.__logger.info("TCBuffer value " + str(tc_buffer.list()))
            if stream is None:
                return ""
            else:
                return binascii.b2a_hex(base64.b64decode(xmlrpclib.loads(stream)[0][0]))    
        except Exception:
            self.__logger.exception("")
            return None

    def __get_os_cert(self, input_data):
        try:
            tc_buffer, stream = self.__rp_client.send(None, 'get_host_cert', base64.b64encode(input_data))
            self.__logger.info("TCBuffer value " + str(tc_buffer.list()))
            if stream is None:
                return ""
            val = xmlrpclib.loads(stream)

            return base64.b64decode(val[0][0])
        except Exception:
            self.__logger.exception("")
            
        return None    
  
    def __get_aik_cert(self, input_data):
	try:
            tc_buffer, stream = self.__rp_client.send(None, 'get_aik_cert', base64.b64encode(input_data))
            self.__logger.info("TCBuffer value " + str(tc_buffer.list()))
            if stream is None:
                return ""
            val = xmlrpclib.loads(stream)

            return base64.b64decode(val[0][0])
        except Exception:
            self.__logger.exception("")

        return None

    def __get_tpm_quote(self, nonce):
	try:
            tc_buffer, stream = self.__rp_client.send(None, 'get_tpm_quote', base64.b64encode(nonce))
            self.__logger.info("TCBuffer value " + str(tc_buffer.list()))
            if stream is None:
                return ""
            val = xmlrpclib.loads(stream)

            return base64.b64decode(val[0][0])
        except Exception:
            self.__logger.exception("")

        return None

 
    def __attest_for(self, rgHash):
        try:
            tc_buffer, stream = self.__rp_client.send(None, 'attest_for', base64.b64encode(rgHash))
            self.__logger.info("TCBuffer value " + str(tc_buffer.list()))
            if stream is None:
		self.__logger.warning("stream is None")
                return None
            val = xmlrpclib.loads(stream)
            return base64.b64decode(val[0][0])
        except Exception:
            self.__logger.exception("")
        return None

    def __get_os_creds(self, input_data):
        return ""

    def get_quote(self, input):
	vmImageId = self.__configuration['VM_IMAGE_ID']
	vmCustomerId = self.__configuration['VM_CUSTOMER_ID']
	vmManifestHash = self.__configuration['VM_MANIFEST_HASH']
	vmManifestSignature = self.__configuration['VM_MANIFEST_SIGNATURE']
	self.__logger.info("vmImageId: "+vmImageId+" vmCustomerId: "+vmCustomerId+" vmManifestHash: "+vmManifestHash+" vmManifestSignature: "+vmManifestSignature)
	root = ET.fromstring(input)
	nonce = root[0].text
	vm_uuid = root[1].text
	if nonce is None or vm_uuid is None:
		self.__logger.error("nonce or vm_uuid not received")
		return None
	self.__logger.info("nonce: "+nonce+" vm_uuid: "+vm_uuid)
	#get IP addresss of VM
	vm_ip = ([(s.connect(('8.8.8.8', 80)), s.getsockname()[0], s.close()) for s in [socket.socket(socket.AF_INET, socket.SOCK_DGRAM)]][0][1])
	self.__logger.info("vm_ip: "+vm_ip)
	#GET VM HASH
	vmHash = self.__get_prog_hash(' ')
        if vmHash is None:
            self.__logger.error("VM image hash is not available please correct the rp_id")
            return False
	self.__logger.info("vmHash: "+vmHash)
	if len(vmHash) == 20:
            Attestation.szdigestAlg = "SHA1"
	#Base64 encode nonce and vmHash
	nonce = base64.b64encode(nonce)
	vmHash = base64.b64encode(vmHash)
	
	#Calculate SHA256 Hash of vmHash, nonce, vm_ip, vm_uuid, vmImageId
        hash_ = SHA256.new()
        hash_.update(vmHash)
	hash_.update(nonce)
	hash_.update(vm_ip)
	hash_.update(vm_uuid)
	hash_.update(vmImageId)
        rgHash = hash_.digest()

        self.__logger.debug( " Hash of vmHash, nonce, vm_ip, vm_uuid, vmImageId is %s" % (base64.b64encode(rgHash)))

	#Send calculated hash for attestation
        quoteValue = self.__attest_for(rgHash)

        if quoteValue is None:
            self.__logger.error("attest_for is failed please check RPCore logs")
            return False
	self.__logger.info("attested_hash: "+quoteValue)
	#GET RPCORE CERT
	szHostCert = self.__get_os_cert(' ')
        if szHostCert is None:
            self.__logger.error( "Host certificate is not available")
            return False
	self.__logger.info("rpcore cert: "+szHostCert)
	
	#Code to generate x509 attribute certificate
	
	#Code to form policy 
	vm_policy =  Attestation.g_vmPolicyTemplate%(vmManifestSignature, vmCustomerId, vmImageId, vmManifestHash) 

	#Create x509 attribute certificate
	#write rpcore cert in /etc/cert.pem
	file = open("/tmp/cert.pem", "w")
	file.write(szHostCert)
	file.close()
	
	#write nonce, image_id, vm_uuid, vm_ip, to file
	file = open("/etc/x509ac_input", "w")
	file.write("VM_UUID=\""+vm_uuid+"\";")
	file.write("\nVM_HASH=\""+vmHash+"\";")
	file.write("\nNONCE=\""+nonce+"\";")
	file.write("\nVM_IMAGE_ID=\""+vmImageId+"\";")
	file.write("\nVM_IP=\""+vm_ip+"\";")
	file.write("\nSIGNATURE_FROM_RPCORE=\""+(base64.b64encode(quoteValue))+"\";")
        file.close()

	_cert = ctypes.CDLL('libx509actest.so')
	_cert.create_attribute_cert.argtypes = ()
	_cert.create_attribute_cert()
	file = open('/tmp/attr_cert.pem', 'r')
	x509ac = file.read()
	file.close()

	#Get TPM Quote
	nonce = "1234"
	tpmQuote = self.__get_tpm_quote(nonce)
	if tpmQuote is None:
            self.__logger.error("get_tpm_quote is failed")
            return False
        self.__logger.info("tpmQuote: "+tpmQuote)

	#Get AIK CERT
	aikCert = self.__get_aik_cert(' ')
        if aikCert is None:
            self.__logger.error( "aik_cert is failed")
            return False
        self.__logger.info("rpcore cert: "+aikCert)

	response = Attestation.g_vm_response_to_mtw%(x509ac, szHostCert, vm_policy, tpmQuote, aikCert)
        return response
    
    def gen_hosted_component_quote(self, nonce, ext_data):
        codeDigest = self.__get_prog_hash(' ')
        if codeDigest is None:
            self.__logger.error("VM image hash is not available please correct the rp_id")
            return False
        # Convert the rp_id into Base64
        if nonce != "" or nonce != None:
            nonce = base64.b64encode(nonce)
        else:
            nonce = ""

        if len(codeDigest) == 20:
            Attestation.szdigestAlg = "SHA1" 

        szCodeDigest = base64.b64encode(codeDigest)
        # construct quotedInfo xml
        # This is hard coded in Vinay code so I am also hard coding it
        quoteType = Attestation.QUOTETYPESHA256FILEHASHRSA1024;
        szQuoteMethod = QuoteConstant.QUOTEMETHODSHA256FILEHASHRSA1024
        if quoteType == Attestation.QUOTETYPESHA256FILEHASHRSA2048 :
            szQuoteMethod = QuoteConstant.QUOTEMETHODSHA256FILEHASHRSA2048
        quotedInfo = Attestation.g_RpQuotedkeyInfoTemplate % (szQuoteMethod, Attestation.szdigestAlg, szCodeDigest, nonce, ext_data)
        
        et = ET.fromstring(quotedInfo)
        output = StringIO.StringIO()
        et.getroottree().write_c14n(output)

        szCanonicalQuotedBody = output.getvalue()
        sys.stderr.write( szCanonicalQuotedBody)
        hash_ = SHA256.new()
        hash_.update(szCanonicalQuotedBody)
        rgHash = hash_.digest()
        
        self.__logger.debug( " first time %s" % (base64.b64encode(rgHash)))
        
        quoteValue = self.__attest_for(rgHash)       

        if quoteValue is None:
            self.__logger.error("Attestation is failed please check RPCore logs")     
            return False

        szHostCert = self.__get_os_cert(' ')
        if szHostCert is None:
            self.__logger.error( "Host certificate is not available")
            return False
  
        szEvidence = self.__get_os_creds(' ')
        if szEvidence is None:
            self.__logger.error( "Evidence list is not available")
            return False
        szquoteValue = base64.b64encode(quoteValue)
        self.__logger.info("base64 converted signature %s " % (szquoteValue) )
        
        return Attestation.g_szRpQuoteTemplate % (szCanonicalQuotedBody, szquoteValue, szHostCert, szEvidence)
        
 
