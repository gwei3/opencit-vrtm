//  File: taoInit.cpp
//      John Manferdelli
//  Description: Key negotiation for the Tao.
//               This is the revised version after the paper
//
//  Copyright (c) 2012, John Manferdelli
//  Some contributions copyright (c) 2012, Intel Corporation
//
// Use, duplication and disclosure of this file and derived works of
// this file are subject to and licensed under the Apache License dated
// January, 2004, (the "License").  This License is contained in the
// top level directory originally provided with the CloudProxy Project.
// Your right to use or distribute this file, or derived works thereof,
// is subject to your being bound by those terms and your use indicates
// consent to those terms.
//
// If you distribute this file (or portions derived therefrom), you must
// include License in or with the file and, in the event you do not include
// the entire License in the file, the file must contain a reference
// to the location of the License.


#include "jlmTypes.h"
#include "logging.h"
#include "jlmcrypto.h"
#include "jlmUtility.h"
#include "modesandpadding.h"
#include "sha256.h"
#include "sha1.h"
#include "tao.h"
#include "bignum.h"
#include "mpFunctions.h"
#include "trustedKeyNego.h"
#ifdef TPMSUPPORT
#include "TPMHostsupport.h"
#include "hashprep.h"
#endif
#include "linuxHostsupport.h"
#ifdef   NEWANDREORGANIZED
#include "cert.h"
#include "quote.h"
#include "cryptoHelper.h"
#else
#include "rsaHelper.h"
#include "claims.h"
#include "secPrincipal.h"
#endif

#include <string.h>
#include <time.h>

#include "rp_api_code.h"
#include "channelcoding.h"

// -------------------------------------------------------------------------

extern tcChannel   g_reqChannel;
taoInit::taoInit(taoHostServices* host)
{
    m_myHost= host;

    m_symKeyValid= false;
    m_symKeyType= 0;
    m_symKeySize= 0;
    m_symKey= NULL;

    m_privateKeyValid= false;
    m_privateKeyType= 0;
    m_privateKeySize= 0;
    m_privateKey= NULL;

    m_myCertificateValid= false;
    m_myCertificateType= 0;
    m_myCertificateSize= 0;
    m_myCertificate= NULL;

    m_myMeasurementValid= false;
    m_myMeasurementType= 0;
    m_myMeasurementSize= 32;

    m_ancestorEvidenceValid= false;
    m_ancestorEvidenceSize= 0;
    m_ancestorEvidence= NULL;

    m_sizeserializedPrivateKey= 0;
    m_szserializedPrivateKey= NULL;

    m_publicKeyValid= false;
    m_publicKeySize= 0; 
    m_publicKey= NULL;

    m_serializedpublicKeySize= 0;
    m_serializedpublicKey= NULL;
    m_publicKeyBlockSize= 0;
}


taoInit::~taoInit()
{
}



const char* g_quotedkeyInfoTemplate= 
"<QuotedInfo>\n" \
"    <ds:CanonicalizationMethod Algorithm=\"http://www.manferdelli.com/2011/Xml/canonicalization/tinyxmlcanonical#\"/>  " \
"    <ds:QuoteMethod Algorithm=\"%s\"/>\n" \
"%s\n" \
"</QuotedInfo>\n";


const char* g_EvidenceListTemplate= 
"<EvidenceList count='%d'>\n" \
"%s\n"
"</EvidenceList>\n";


char* constructEvidenceList(const char* szEvidence, const char* szEvidenceSupport)
{
    int     sizeList;
    char*   szEvidenceList= NULL;
    char*   szReturn= NULL;

    if(szEvidence==NULL)
        return NULL;

    if(szEvidenceSupport==NULL)
        sizeList= strlen(szEvidence)+256;
    else
        sizeList= strlen(szEvidence)+strlen(szEvidenceSupport)+256;

    szEvidenceList= (char*) malloc(sizeList);
    if(szEvidenceList==NULL)
        return NULL;

    // Fix: later include evidence support
    sprintf(szEvidenceList, g_EvidenceListTemplate, 1, szEvidence);
    szReturn= canonicalizeXML(szEvidenceList);
    if(szEvidenceList!=NULL)
        free(szEvidenceList);
    return szReturn;
}


bool taoInit::generateCertRequest(u32 keyType, const char* szKeyName, 
                                const char* szSubjectName, const char* szSubjectId, char* msg_buf, int maxSize)
{
    // this is the host certificate
    int             sizeHostCert= 0;
    u32             typeHostCert;
    char*           szHostCert= NULL;
    char*           szHostKey= NULL;
    PrincipalCert   hostCert;

    // this is host key evidence
    int             sizeEvidence= 0;
    char*           szEvidence= NULL;                   

    // this is the Quote XML
    char            quotedInfo[4096];                   

    // this is the canonicalize Quote XML
    char*           szCanonicalQuotedBody= NULL;        
    Sha256          oHash;

    // this is the hash of the Quote XML
    int             sizequotedHash= SHA256DIGESTBYTESIZE;
    byte            rgHash[SHA256DIGESTBYTESIZE];       

    // this is the TPM signed quote value
    int             sizequoteValue= 512;
    byte            quoteValue[512];                    

    // this is my measurement
    u32             codeDigestType= 0;
    int             sizeCodeDigest= SHA256DIGESTBYTESIZE;
    byte            codeDigest[SHA256DIGESTBYTESIZE];   
    Sha1            oSha1Hash;
    
    char			locality[8] = {0};

    // this is the final formatted Quote
    u32             quoteType=0;
    char*           szQuote= NULL;                      


    quotedInfo[0]= 0;
    // quote key valid?
    if(m_myHost==NULL || !m_myHost->m_hostValid) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: host invalid\n");
        return false;
    }
//
// v: AR: clean up previous tao state memory
//
    // compute quote type from host type and key type
    switch(m_myHost->m_hostType) {
      default:
      case PLATFORMTYPENONE:
      case PLATFORMTYPEHYPERVISOR:
        return false;
      case PLATFORMTYPEHW:
        quoteType= QUOTETYPETPM12RSA2048;
        break;
      case PLATFORMTYPELINUX:
        if(keyType==KEYTYPERSA1024INTERNALSTRUCT)
            quoteType= QUOTETYPESHA256FILEHASHRSA1024;
        else if(keyType==KEYTYPERSA2048INTERNALSTRUCT)
            quoteType= QUOTETYPETPM12RSA2048;
        else
            return false;
        break;
      case PLATFORMTYPELINUXAPP:
        if(keyType==KEYTYPERSA1024INTERNALSTRUCT)
            quoteType= QUOTETYPESHA256FILEHASHRSA1024;
        else if(keyType==KEYTYPERSA2048INTERNALSTRUCT)
            quoteType= QUOTETYPETPM12RSA2048;
        else
            return false;
        break;
    }

    // generate key pair
    if(!genprivateKeyPair(keyType, szKeyName)) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't generate keypair\n");
        return false;
    }
    RSAKey* pKey= (RSAKey*)m_privateKey;

    // serialize public key
    m_serializedpublicKey= pKey->SerializePublictoString();
    m_publicKeyBlockSize= pKey->m_iByteSizeM;
    if(m_serializedpublicKey==NULL) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: can't serialize public key\n");
        return false;
    }
    m_serializedpublicKeySize= strlen(m_serializedpublicKey)+1;

    // get code digest
    if(!m_myHost->GetHostedMeasurement(&sizeCodeDigest, &codeDigestType, codeDigest)) {
        fprintf(g_logFile, "generatequoteandcertifyKey: Can't get code digest\n");
        return false;
    }
    m_myMeasurementType= codeDigestType;
    if(sizeCodeDigest>m_myMeasurementSize) {
        fprintf(g_logFile, "generatequoteandcertifyKey: code digest too big\n");
        return false;
    }
    
    m_myMeasurementSize= sizeCodeDigest;
    memcpy(m_myMeasurement, codeDigest, m_myMeasurementSize);
    m_myMeasurementValid= true;


    switch(quoteType) {

      default:
      case QUOTETYPENONE:
      case QUOTETYPETPM12RSA1024:
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: bad quote type\n");
        return false;

      case QUOTETYPETPM12RSA2048:
        // Construct quote body
        sprintf(quotedInfo, g_quotedkeyInfoTemplate, QUOTEMETHODTPM12RSA2048, 
                m_serializedpublicKey);
        szCanonicalQuotedBody= canonicalizeXML(quotedInfo);
        if(szCanonicalQuotedBody==NULL) {
            fprintf(g_logFile, 
                "GenerateQuoteAndCertifyKey: Can't canonicalize quoted info\n");
            return false;
        }
        // hash it
        oSha1Hash.Init();
        oSha1Hash.Update((byte*) szCanonicalQuotedBody, strlen(szCanonicalQuotedBody));
        oSha1Hash.Final();
        oSha1Hash.getDigest(rgHash);
        sizequotedHash= SHA1DIGESTBYTESIZE;
        break;

      case QUOTETYPESHA256FILEHASHRSA1024:
        // Construct quote body
        sprintf(quotedInfo, g_quotedkeyInfoTemplate, QUOTEMETHODSHA256FILEHASHRSA1024, 
                m_serializedpublicKey);
        szCanonicalQuotedBody= canonicalizeXML(quotedInfo);
        if(szCanonicalQuotedBody==NULL) {
            fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't canonicalize quoted info\n");
            return false;
        }
        // hash it
        oHash.Init();
        oHash.Update((byte*) szCanonicalQuotedBody, 
                     strlen(szCanonicalQuotedBody));
        oHash.Final();
        oHash.GetDigest(rgHash);
        sizequotedHash= SHA256DIGESTBYTESIZE;
        break;

      case QUOTETYPESHA256FILEHASHRSA2048:
        // Construct quote body
        sprintf(quotedInfo, g_quotedkeyInfoTemplate, 
                QUOTEMETHODSHA256FILEHASHRSA1024, m_serializedpublicKey);
        szCanonicalQuotedBody= canonicalizeXML(quotedInfo);
        if(szCanonicalQuotedBody==NULL) {
            fprintf(g_logFile, 
                "GenerateQuoteAndCertifyKey: Can't canonicalize quoted info\n");
            return false;
        }
        // hash it
        oHash.Init();
        oHash.Update((byte*) szCanonicalQuotedBody, 
                     strlen(szCanonicalQuotedBody));
        oHash.Final();
        oHash.GetDigest(rgHash);
        sizequotedHash= SHA256DIGESTBYTESIZE;
        break;
    }


    // Do attest
    if(!m_myHost->Attest(sizequotedHash, rgHash, &sizequoteValue, quoteValue)) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't Attest Key\n");
        return false;
    }

	sprintf(locality, "%d", m_myHost->locality());

    // Get the certificate
    if(!m_myHost->GetAttestCertificate(&sizeHostCert, &typeHostCert, (byte**)&szHostCert)) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't get Host cert\n");
        return false;
    }


    // Get evidence list
    if(!m_myHost->GetAncestorCertificates(&sizeEvidence, (byte**)&szEvidence)) 
        szEvidence= NULL;

    m_ancestorEvidence= constructEvidenceList(szHostCert, szEvidence);
    if(m_ancestorEvidence==NULL) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't construct new cert evidence\n");
        return false;
    }
 
    m_ancestorEvidenceValid= true;
    m_ancestorEvidenceSize= strlen(m_ancestorEvidence)+1;

#if 0 //v: commented to remove policy key signing requirement of AIk
    if(!hostCert.init(szHostCert)) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't init host key\n");
        return false;
    }
    if(!hostCert.parsePrincipalCertElements()) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't parse host key\n");
        return false;
    }
    RSAKey* hostKey= (RSAKey*)  hostCert.getSubjectKeyInfo();
    if(hostKey==NULL) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't get host subject key\n");
        return false;
    }
    szHostKey= hostKey->SerializePublictoString();
    if(szHostKey==NULL) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't serialize host subject key\n");
        return false;
    }

#else
	RSAKey aikpub;
	
	if (! aikpub.ParsefromString(szHostCert) ){
        fprintf(g_logFile, "%s: Can't init host key\n", __FUNCTION__);
        return false;
    }
    
    if (! aikpub.getDataFromDoc() ) {
		fprintf(g_logFile, "%s: Can't init host key\n", __FUNCTION__);
        return false;
	}
	
	szHostKey= aikpub.SerializePublictoString();
#endif

    // Format quote
    szQuote= encodeXMLQuote(0, NULL, sizeCodeDigest, codeDigest, locality, szCanonicalQuotedBody, 
                            szHostKey, sizequoteValue, quoteValue);
    if(szQuote==NULL) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't encode quote\n");
        return false;
    }

	// Serialize private key
    m_szserializedPrivateKey= pKey->SerializetoString();
    if(m_szserializedPrivateKey==NULL) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't serialize private key\n");
        return false;
    }
    
    // Certify it
    if(!clientCertNegoMessage1(msg_buf,  maxSize, "key1", szQuote, m_ancestorEvidence))
    {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: key nego failed\n");
        return false;
    }

	return true;
}


bool taoInit::handleCertResponse(const char* srv_response){
	char*               szStatus= NULL;
    char*               szErrorCode= NULL;

   if(!getDatafromServerCertMessage1(srv_response, &szStatus, &szErrorCode, &m_myCertificate)){
	   fprintf(g_logFile, "server response invalid\n");
		return false; //  "server response invalid\n";
	}

	// everything OK?
	if(!validateResponse(szStatus, szErrorCode, m_myCertificate)) {
		fprintf(g_logFile, "server response invalid\n");
		return false; //throw  "server response invalid\n";
	}
            
    m_myCertificateValid= true;
    m_myCertificateType= EVIDENCECERT;
    m_myCertificateSize= strlen(m_myCertificate)+1;

    return true;
}


#if 0
bool taoInit::generatequoteandcertifyKey(u32 keyType, const char* szKeyName, 
                                const char* szSubjectName, const char* szSubjectId)
{
    // this is the host certificate
    int             sizeHostCert= 0;
    u32             typeHostCert;
    char*           szHostCert= NULL;
    char*           szHostKey= NULL;
    PrincipalCert   hostCert;

    // this is host key evidence
    int             sizeEvidence= 0;
    char*           szEvidence= NULL;                   

    // this is the Quote XML
    char            quotedInfo[4096];                   

    // this is the canonicalize Quote XML
    char*           szCanonicalQuotedBody= NULL;        
    Sha256          oHash;

    // this is the hash of the Quote XML
    int             sizequotedHash= SHA256DIGESTBYTESIZE;
    byte            rgHash[SHA256DIGESTBYTESIZE];       

    // this is the TPM signed quote value
    int             sizequoteValue= 512;
    byte            quoteValue[512];                    

    // this is my measurement
    u32             codeDigestType= 0;
    int             sizeCodeDigest= SHA256DIGESTBYTESIZE;
    byte            codeDigest[SHA256DIGESTBYTESIZE];   
    Sha1            oSha1Hash;

    // this is the final formatted Quote
    u32             quoteType=0;
    char*           szQuote= NULL;                      


    quotedInfo[0]= 0;
    // quote key valid?
    if(m_myHost==NULL || !m_myHost->m_hostValid) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: host invalid\n");
        return false;
    }

    // compute quote type from host type and key type
    switch(m_myHost->m_hostType) {
      default:
      case PLATFORMTYPENONE:
      case PLATFORMTYPEHYPERVISOR:
        return false;
      case PLATFORMTYPEHW:
        quoteType= QUOTETYPETPM12RSA2048;
        break;
      case PLATFORMTYPELINUX:
        if(keyType==KEYTYPERSA1024INTERNALSTRUCT)
            quoteType= QUOTETYPESHA256FILEHASHRSA1024;
        else if(keyType==KEYTYPERSA2048INTERNALSTRUCT)
            quoteType= QUOTETYPETPM12RSA2048;
        else
            return false;
        break;
      case PLATFORMTYPELINUXAPP:
        if(keyType==KEYTYPERSA1024INTERNALSTRUCT)
            quoteType= QUOTETYPESHA256FILEHASHRSA1024;
        else if(keyType==KEYTYPERSA2048INTERNALSTRUCT)
            quoteType= QUOTETYPETPM12RSA2048;
        else
            return false;
        break;
    }

    // generate key pair
    if(!genprivateKeyPair(keyType, szKeyName)) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't generate keypair\n");
        return false;
    }
    RSAKey* pKey= (RSAKey*)m_privateKey;

    // serialize public key
    m_serializedpublicKey= pKey->SerializePublictoString();
    m_publicKeyBlockSize= pKey->m_iByteSizeM;
    if(m_serializedpublicKey==NULL) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: can't serialize public key\n");
        return false;
    }
    m_serializedpublicKeySize= strlen(m_serializedpublicKey)+1;

    // get code digest
    if(!m_myHost->GetHostedMeasurement(&sizeCodeDigest, &codeDigestType, codeDigest)) {
        fprintf(g_logFile, "generatequoteandcertifyKey: Can't get code digest\n");
        return false;
    }
    m_myMeasurementType= codeDigestType;
    if(sizeCodeDigest>m_myMeasurementSize) {
        fprintf(g_logFile, "generatequoteandcertifyKey: code digest too big\n");
        return false;
    }
    
    m_myMeasurementSize= sizeCodeDigest;
    memcpy(m_myMeasurement, codeDigest, m_myMeasurementSize);
    m_myMeasurementValid= true;


    switch(quoteType) {

      default:
      case QUOTETYPENONE:
      case QUOTETYPETPM12RSA1024:
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: bad quote type\n");
        return false;

      case QUOTETYPETPM12RSA2048:
        // Construct quote body
        sprintf(quotedInfo, g_quotedkeyInfoTemplate, QUOTEMETHODTPM12RSA2048, 
                m_serializedpublicKey);
        szCanonicalQuotedBody= canonicalizeXML(quotedInfo);
        if(szCanonicalQuotedBody==NULL) {
            fprintf(g_logFile, 
                "GenerateQuoteAndCertifyKey: Can't canonicalize quoted info\n");
            return false;
        }
        // hash it
        oSha1Hash.Init();
        oSha1Hash.Update((byte*) szCanonicalQuotedBody, strlen(szCanonicalQuotedBody));
        oSha1Hash.Final();
        oSha1Hash.getDigest(rgHash);
        sizequotedHash= SHA1DIGESTBYTESIZE;
        break;

      case QUOTETYPESHA256FILEHASHRSA1024:
        // Construct quote body
        sprintf(quotedInfo, g_quotedkeyInfoTemplate, QUOTEMETHODSHA256FILEHASHRSA1024, 
                m_serializedpublicKey);
        szCanonicalQuotedBody= canonicalizeXML(quotedInfo);
        if(szCanonicalQuotedBody==NULL) {
            fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't canonicalize quoted info\n");
            return false;
        }
        // hash it
        oHash.Init();
        oHash.Update((byte*) szCanonicalQuotedBody, 
                     strlen(szCanonicalQuotedBody));
        oHash.Final();
        oHash.GetDigest(rgHash);
        sizequotedHash= SHA256DIGESTBYTESIZE;
        break;

      case QUOTETYPESHA256FILEHASHRSA2048:
        // Construct quote body
        sprintf(quotedInfo, g_quotedkeyInfoTemplate, 
                QUOTEMETHODSHA256FILEHASHRSA1024, m_serializedpublicKey);
        szCanonicalQuotedBody= canonicalizeXML(quotedInfo);
        if(szCanonicalQuotedBody==NULL) {
            fprintf(g_logFile, 
                "GenerateQuoteAndCertifyKey: Can't canonicalize quoted info\n");
            return false;
        }
        // hash it
        oHash.Init();
        oHash.Update((byte*) szCanonicalQuotedBody, 
                     strlen(szCanonicalQuotedBody));
        oHash.Final();
        oHash.GetDigest(rgHash);
        sizequotedHash= SHA256DIGESTBYTESIZE;
        break;
    }


    // Do attest
    if(!m_myHost->Attest(sizequotedHash, rgHash, &sizequoteValue, quoteValue)) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't Attest Key\n");
        return false;
    }


    // Get the certificate
    if(!m_myHost->GetAttestCertificate(&sizeHostCert, &typeHostCert, (byte**)&szHostCert)) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't get Host cert\n");
        return false;
    }


    // Get evidence list
    if(!m_myHost->GetAncestorCertificates(&sizeEvidence, (byte**)&szEvidence)) 
        szEvidence= NULL;

    m_ancestorEvidence= constructEvidenceList(szHostCert, szEvidence);
    if(m_ancestorEvidence==NULL) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't construct new cert evidence\n");
        return false;
    }
 
    m_ancestorEvidenceValid= true;
    m_ancestorEvidenceSize= strlen(m_ancestorEvidence)+1;


    if(!hostCert.init(szHostCert)) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't init host key\n");
        return false;
    }
    if(!hostCert.parsePrincipalCertElements()) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't parse host key\n");
        return false;
    }
    RSAKey* hostKey= (RSAKey*)  hostCert.getSubjectKeyInfo();
    if(hostKey==NULL) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't get host subject key\n");
        return false;
    }
    szHostKey= hostKey->SerializePublictoString();
    if(szHostKey==NULL) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't serialize host subject key\n");
        return false;
    }


    // Format quote
    szQuote= encodeXMLQuote(0, NULL, sizeCodeDigest, codeDigest, szCanonicalQuotedBody, 
                            szHostKey, sizequoteValue, quoteValue);
    if(szQuote==NULL) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't encode quote\n");
        return false;
    }


    // Certify it
    if(!KeyNego(szQuote, m_ancestorEvidence, (char**)&m_myCertificate)) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: key nego failed\n");
        return false;
    }
 
  
    m_myCertificateValid= true;
    m_myCertificateType= EVIDENCECERT;
    m_myCertificateSize= strlen(m_myCertificate)+1;


    // Serialize private key
    m_szserializedPrivateKey= pKey->SerializetoString();
    if(m_szserializedPrivateKey==NULL) {
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: Can't serialize private key\n");
        return false;
    }

    // Fix: clean up szHostCert and szEvidence
    return true;
}

#endif //generatequotetocertifyKey

bool taoInit::initKeys(u32 symType, u32 pubkeyType, const char* szKeyName, 
                       const char* szSubjectName, const char* szSubjectId)
{
    bool        fRet= false;


    fRet= gensymKey(symType);
    if(!fRet)
        return false;
#if 0
    fRet= generatequoteandcertifyKey(pubkeyType, szKeyName, 
                                     szSubjectName, szSubjectId);
    if(!fRet)
        goto cleanup;
        
#else
	int status = 0;

    printf("Waiting for RP certification by server\n");
	while ((TAOINIT_DONE != status) && (TAOINIT_FAIL != status)){
		status = do_rp_certification(pubkeyType, szKeyName, szSubjectName, szSubjectId);
	}
	fRet = (TAOINIT_DONE == status)? true : false;
#endif 


cleanup:
    
    return fRet;
}


bool taoInit::gensymKey(u32 symType)
{
    if(symType!=KEYTYPEAES128PAIREDENCRYPTINTEGRITY)
        return false;
    m_symKeyValid= true;
    m_symKeySize= 32;
    m_symKey= (byte*) malloc(m_symKeySize);
    if(m_symKey==NULL)
        return false;
    if(!getCryptoRandom(m_symKeySize*NBITSINBYTE, m_symKey))
        return false;
    return true;
}


bool taoInit::genprivateKeyPair(u32 keyType, const char* szKeyName)
{
    int         ikeySize= 0;
    RSAKey*     pKey= NULL;

#ifdef TEST
    fprintf(g_logFile, "genprivateKeyPair(%d)\n", (int) keyType);
#endif
    if(keyType==KEYTYPERSA1024INTERNALSTRUCT) {
        ikeySize= 1024;
    }
    else if(keyType==KEYTYPERSA2048INTERNALSTRUCT) {
       ikeySize= 2048;
    }
    else
        return false;

#ifdef NEWANDREORGANIZED
    pKey= RSAGenerateKeyPair(ikeySize);
#else
    pKey= generateRSAKeypair(ikeySize);
#endif
    if(pKey==NULL) {
        fprintf(g_logFile, "taoEnvironment::genprivateKeyPair: Can't generate RSA key pair\n");
        return false;
    }
    if(szKeyName!=NULL && strlen(szKeyName)<(KEYNAMEBUFSIZE-1)) {
        pKey->m_ikeyNameSize= strlen(szKeyName);
        memcpy(pKey->m_rgkeyName, szKeyName, pKey->m_ikeyNameSize+1);
    }

    m_privateKeyType= keyType;
    m_privateKeySize= sizeof(RSAKey);
    m_privateKey= (byte*)pKey;
    if(m_privateKey==NULL)
        return false;

    m_privateKeyValid= true;
    return true;
}


// --------------------------------------------------------------------------

int taoInit::do_rp_certification(u32 keyType, const char* szKeyName, 
                                const char* szSubjectName, const char* szSubjectId)
{
    int                 procid;
    int                 origprocid;
    u32                 uReq;
    u32                 uStatus;


    int                 inparamsize;
    byte                inparams[PARAMSIZE];

    int                 outparamsize;
    byte                outparams[PARAMSIZE];

    int                 size;
//    byte                rgBuf[PARAMSIZE];


	bool 				status;
	int 				retval = TAOINIT_FAIL;
    // get request
    inparamsize= PARAMSIZE;
    
	memset(inparams, 0, inparamsize);
	
    if(!g_reqChannel.gettcBuf(&procid, &uReq, &uStatus, &origprocid, &inparamsize, inparams)) {
        fprintf(stdout, "serviceRequest: gettcBuf failed\n");
        return retval;
    }
    
    if(uStatus==TCIOFAILED) {
        g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
        return retval;
    }

	fprintf(stdout, "config file size is %d\n", inparamsize);
	
    switch(uReq) {
		
		
		default:
			outparamsize = sprintf ((char*)outparams, "RPCore is not certified yet."); 
			g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, outparamsize, outparams);
			
			return retval;
			
	     case RP2VM_GETRPHOSTQUOTE:
				
				if(!decodeVM2RP_GETRPHOSTQUOTE(&outparamsize, outparams, inparams)) {
					fprintf(g_logFile, "serviceRequest: decodeTCSERVICEGETRPHOSTQUOTEFROMAPP failed\n");
					g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return retval;
				}
				
				inparamsize= PARAMSIZE;
	
                if (! generateCertRequest ( KEYTYPERSA1024INTERNALSTRUCT, szKeyName, szSubjectName, szSubjectId, (char*)inparams, inparamsize)) {
				}
                
                    
				size = strlen((char*)inparams)+1;
				
		        outparamsize= encodeRP2VM_GETRPHOSTQUOTE(size,(byte*)inparams, PARAMSIZE, outparams);
				if(outparamsize<0) {
					fprintf(g_logFile, "serviceRequest: encodeRP2VM_GETRPHOSTQUOTE buf too small\n");
					g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return retval;
				}
				
				retval = TAOINIT_INPROGRESS;
				break;
				
		case RP2VM_RPHOSTQUOTERESPONSE:
				
				if(!decodeVM2RP_RPHOSTQUOTERESPONSE(&outparamsize, outparams, inparams)) {
					fprintf(g_logFile, "serviceRequest: decodeVM2RP_RPHOSTQUOTERESPONSE failed\n");
					g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return retval;
				}
	
				status = handleCertResponse((char*)outparams);
				
				size = (status)? sizeof(status): 0;
				
				memset(outparams, 0, PARAMSIZE);
		        outparamsize= encodeRP2VM_RPHOSTQUOTERESPONSE(size,(byte*) &status, PARAMSIZE, outparams);
				if(outparamsize<0) {
					fprintf(g_logFile, "serviceRequest: encodeRP2VM_RPHOSTQUOTERESPONSE buf too small\n");
					g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return retval;
				}
				
				retval = TAOINIT_DONE;
				break;
    }
	
	if(!g_reqChannel.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
		fprintf(stdout, "serviceRequest: sendtcBuf (startapp) failed\n");
		retval = TAOINIT_FAIL;
	}
	
	return retval;
	
}
