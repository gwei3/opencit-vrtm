import base64
from Crypto.PublicKey.RSA import _RSAobj
from Crypto.PublicKey.RSA import RSAImplementation
from Crypto.Util import number

from type import ALGO

def new_rsa_key(key_info_ele):
    rsa_key = RSAKey()
    szKeyType = key_info_ele.getchildren()[0].text
      
    if szKeyType == "RSAKeyType" :
        rsa_key.m_ukeyType = ALGO.RSAKEYTYPE
        rsa_key.m_uAlgorithm = ALGO.NOALG
        rsa_key.m_rgkeyName = key_info_ele.attrib["KeyName"]
        rsa_key.m_ikeySize = len(rsa_key.m_rgkeyName)
            
        if not (rsa_key.m_ikeySize < ALGO.KEYNAMEBUFSIZE) :
            rsa_key.m_ikeySize = 0
            rsa_key.m_rgkeyName = None

        key_value =  key_info_ele.getchildren()[1]              
        rsa_key_value = key_value.getchildren()[0]
        rsa_key.m_ikeySize = rsa_key_value.attrib["size"]
        #rsa_key.set_m_iByteSizeP(rsa_key_value.find("ds:P").text)
        #rsa_key.set_m_iByteSizeQ(rsa_key_value.find("ds:Q").text)
        chidl = rsa_key_value.getchildren()
        rsa_key.set_m_iByteSizeM(rsa_key_value.getchildren()[0].text)
        rsa_key.set_m_iByteSizeE(rsa_key_value.getchildren()[1].text)
        #rsa_key.set_m_iByteSizeD(rsa_key_value.find("ds:D").text)
        #rsa_key.set_m_iByteSizeDP(rsa_key_value.find("ds:DP").text)
        #rsa_key.set_m_iByteSizeDQ(rsa_key_value.find("ds:DQ").text)
        return rsa_key
    else:
        print 'Unknown key type...'
        return None
        
        
class RSAKey(_RSAobj):
    def __init__(self):
        _RSAobj.__init__(self, RSAImplementation(), None )
        self.m_ikeySize = None
        self.m_ukeyType = None
        self.m_uAlgorithm = None
        self.m_ikeySize = None
        self.m_ikeyNameSize = None
        self.m_iByteSizeM = None
        self.m_iByteSizeP = None
        self.m_iByteSizeQ = None
        self.m_iByteSizeE = None
        self.m_iByteSizeD = None
        self.m_iByteSizeDP = None
        self.m_iByteSizeDQ = None
        self.m_iByteSizePM1 = None 
        self.m_iByteSizeQM1 = None
        self.m_rgkeyName = None
        self.m_ukeyType = None
        self.m_uAlgorithm = None
        
        self.m_pbnM = None
        self.m_pbnP = None
        self.m_pbnQ = None
        self.m_pbnE = None
        self.m_pbnD = None
        self.m_pbnDP = None
        self.m_pbnDQ = None
        self.m_pbnPM1 = None;
        self.m_pbnQM1 = None

    def publickey(self):
        # m_iByteSizeM public key modulus and m_iByteSizeE public key exponent
        return self.implementation.construct((self.m_iByteSizeM, self.m_iByteSizeE))
        
    def __decode(self, value):
        if value is None :
            print "__decode has received none value"
            return None
        try :
            str_array = base64.b64decode(value)
            str_array = str_array[::-1]
            return number.bytes_to_long(str_array)
        except Exception as e :
            print e
            return None
        
    def set_m_iByteSizeM(self, value):
        self.m_iByteSizeM = self.__decode(value)
        
    def set_m_iByteSizeP(self, value):
        self.m_iByteSizeP = self.__decode(value)
        
    def set_m_iByteSizeQ(self, value):
        self.m_iByteSizeQ = self.__decode(value)

    def set_m_iByteSizeE(self, value):
        self.m_iByteSizeE = self.__decode(value)
        
    def set_m_iByteSizeD(self, value):
        self.m_iByteSizeD = self.__decode(value)
        
    def set_m_iByteSizeDP(self, value):
        self.m_iByteSizeDP = self.__decode(value)
        
    def set_m_iByteSizeDQ(self, value):
        self.m_iByteSizeDQ = self.__decode(value)
        
        
