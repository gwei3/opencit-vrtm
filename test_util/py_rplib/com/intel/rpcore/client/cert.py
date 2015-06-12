import raw_time as time
import rsa

class PrincipalCert:
    def __init__(self, hostCertElement):
        self.m_szSignature= hostCertElement;
        self.m_szSignedInfo= None;
        self.m_szSignatureValue= None;
        self.m_szSignatureMethod= None;
        self.m_szCanonicalizationMethod= None;
        self.m_szRevocationInfo= None;
        self.m_pSignerKeyInfo= None;
        self.m_pSubjectKeyInfo= None;
        self.m_szPrincipalName= None;
        self.m_szPrincipalType= None;
        self.m_fSigValuesValid= False;
        self.m_pRootElement= None;
        self.not_before = None;
        self.not_after = None;
   
    def get_principal_type(self):
        return self.m_szPrincipalType
    
    def get_principal_name(self):
        return self.m_szPrincipalName
    
    def get_canonicalization_method(self):
        return self.m_szCanonicalizationMethod
    
    def parse_principal_cert_elements(self):
        if self.m_szSignature is None:
            return False
        #self.m_szSignedInfo = self.m_szSignature.find("ds:SignedInfo")
        self.m_szSignedInfo = self.m_szSignature.getchildren()[0].getchildren()[0]
        self.m_szSignatureMethod = self.m_szSignedInfo.getchildren()[1].attrib["Algorithm"]
        self.m_szCanonicalizationMethod = self.m_szSignedInfo.getchildren()[0].attrib["Algorithm"]
        
        certificate =  self.m_szSignedInfo.getchildren()[2]
        revocation_policy = certificate.getchildren()[8].text
        if revocation_policy is None:
            print "parsePrincipalCertElementfromRoot: Cant find RevocationPolicy\n"
            return False
         
        subject_key = certificate.getchildren()[6]
        if subject_key is None:
            print "parsePrincipalCertElementfromRoot: Can't find SubjectKey\n"
            return False
        
        pSubjectKeyInfoNode= subject_key.getchildren()[0]
        if pSubjectKeyInfoNode is None:
            print "parsePrincipalCertElementfromRoot: Cant find SubjectKey KeyInfo\n"
            return False
        
        self.m_pSubjectKeyInfo = rsa.new_rsa_key(pSubjectKeyInfoNode)
        if self.m_pSubjectKeyInfo is None :
            print "parsePrincipalCertElementfromRoot: Cant init KeyInfo\n"
            return False;
        
        self.m_szPrincipalName = certificate.getchildren()[5].text # SubjectName
        if self.m_szPrincipalName is None :
            print "Can't find subject name..."
            return False

        self.m_szPrincipalType = certificate.getchildren()[1].text # PrincipalType
        if self.m_szPrincipalType is None :
            print "Can't find principle type..."
            return False
        
        if not time.timefromString(certificate.getchildren()[4].getchildren()[0].text): # NotBefore
            print "parsePrincipalCertElementfromRoot: Cant interpret NotBefore value\n"
            return False
        if not time.timefromString(certificate.getchildren()[4].getchildren()[1].text): # NotAfter
            print "parsePrincipalCertElementfromRoot: Cant interpret NotAfter value\n"
            return False
            
        self.m_szSignatureValue = self.m_szSignature.getchildren()[0].getchildren()[1].text # ds:SignatureValue
        if self.m_szSignatureValue is None :
            print "m_szSignatureValue is None"
            return False
        
        self.m_fSigValuesValid = True
    
        return True 
    
    
    def get_subject_key_info(self):
        return self.m_pSubjectKeyInfo
    
    def get_validity_period(self):
        return None # need to check / discuss the functionality
    
    def get_canonical_was_signed(self):
        return self.m_szSignedInfo
    
    def get_revocation_policy(self):
        return self.m_szRevocationInfo
    
    def get_signature_value(self):
        return self.m_szSignatureValue
    
    def get_signature_algorithm(self):
        return self.m_szSignatureMethod
    
    def same_as(self, principalCertObj):
        return None # check what this method doing 
