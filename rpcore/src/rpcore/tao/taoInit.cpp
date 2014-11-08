
//  File: taoInit.cpp
//      Vinay Phegade, John Manferdelli
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
#include "RP_RSAKey-helper.h"
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

//#define CSR_REQ 1
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

#ifdef CSR_REQ
    m_pem_privateKeyValid= false;
    m_pem_privateKeyType= 0;
    m_pem_privateKeySize= 0;
    m_pem_privateKey= NULL;
#endif

    m_myCertificateValid= false;
    m_myCertificateType= 0;
    m_myCertificateSize= 0;
    m_myCertificate= NULL;

    m_myMeasurementValid= false;
    m_myMeasurementType= 0;
    m_myMeasurementSize= 32;
    //m_myMeasurementSize= 20;

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
"	<nonce>%s</nonce>\n"
"    <ds:CanonicalizationMethod Algorithm=\"http://www.rpcore.com/2011/Xml/canonicalization/tinyxmlcanonical#\"/>  " \
"    <ds:QuoteMethod Algorithm=\"%s\"/>\n" 
"%s\n" \
"</QuotedInfo>\n";


/*
const char* g_quotedkeyInfoTemplate=
"<QuotedInfo>\n" \
"    <ds:CanonicalizationMethod Algorithm=\"http://www.rpcore.com/2011/Xml/canonicalization/tinyxmlcanonical#\"/>  " \
"    <ds:QuoteMethod Algorithm=\"%s\"/>\n" \
"%s\n" \
"</QuotedInfo>\n";
*/


const char* g_EvidenceListTemplate=
"<EvidenceList count='%d'>\n" \
"%s\n" \
"</EvidenceList>\n";

//constructPcrSelectionQuoteString: format the indices of PCRs as "%d,%d,...,%d",
//assumption: pcrSelectionBuf has enough space of hold the string (>3*24)
int g_maxPcrSelectionBufSize = 128;
bool constructPcrSelectionQuoteString(char * pcrSelectionBuf, int* pcrSelection, int numPCRs)
{	
	if(pcrSelectionBuf == NULL)
		return false;
	
	char *start = pcrSelectionBuf;
	for(int i=0; i<numPCRs; i++){
		if(i==0)
			sprintf(start, "%d", pcrSelection[i]);
		else
			sprintf(start, ",%d", pcrSelection[i]);
			
			//fprintf(stdout, "formatted pcrSelect string: %s; \n", pcrSelectionBuf);
			
		int len = strlen(start);
		start += len;
	}
	
	fprintf(stdout, "formatted pcrSelect string: %s; \n", pcrSelectionBuf);
	return true;
}

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

    #ifdef TEST
    fprintf(g_logFile, "szEvidence: \n %s \n end of szEvidence\n", szEvidence);
    #endif

    // Fix: later include evidence support
    sprintf(szEvidenceList, g_EvidenceListTemplate, 1, szEvidence);
    
    #ifdef TEST
    fprintf(g_logFile, "szEvidenceList before canonicalizeXML: \n %s \n end of szEvidenceList\n", szEvidenceList);
    #endif
    
    szReturn= canonicalizeXML(szEvidenceList);
    if(szEvidenceList!=NULL)
        free(szEvidenceList);
    return szReturn;
}

bool taoInit::generateCSR(u32 keyType, const char* szKeyName,
                                const char* szSubjectName, const char* szSubjectId, char * nonceStr,
                                char* msg_buf, int maxSize)
{
    int ikeySize;
    if(keyType==KEYTYPERSA1024INTERNALSTRUCT) {
        ikeySize= 1024;
    }
    else if(keyType==KEYTYPERSA2048INTERNALSTRUCT) {
       ikeySize= 2048;
    }
    else
        return false;
        
    // this is my measurement
    u32             codeDigestType= 0;
    int             sizeCodeDigest= SHA256DIGESTBYTESIZE;
    byte            codeDigest[SHA256DIGESTBYTESIZE];  

    RSA * opensslRsaKey=RSA_new();
    RSAKey*  pKey= new RSAKey();
    if( pKey == NULL ) {
	RSA_free(opensslRsaKey);
        fprintf(g_logFile, "taoEnvironment:: Can't generate RSA key pair\n");
        return false;
    }
    
    RSAGenerateKeyPair_OpenSSL(ikeySize, opensslRsaKey, pKey);
    char *pemkey = opensslRSAtoPEM(opensslRsaKey);
    if( pemkey == NULL ) {
	RSA_free(opensslRsaKey);
	free(pKey);
        fprintf(g_logFile, "taoEnvironment:: opensslRSAtoPEM failed\n");
        return false;
    }

    char *pem;
    //pem = (char *)malloc(1500);
    int len;	
    pem = create_csr(opensslRsaKey);
    if(pem == NULL){
	fprintf(g_logFile, "taoEnvironment:: opensslRSAtoPEM failed\n");
	RSA_free(opensslRsaKey);
        free(pKey);
	return false;
    }
    printf("\n Pem key: %s", pem);
 
    if(szKeyName != NULL && strlen(szKeyName) < (KEYNAMEBUFSIZE-1)) {
        pKey->m_ikeyNameSize= strlen(szKeyName);
	memset(pKey->m_rgkeyName, 0, pKey->m_ikeyNameSize);
        memcpy(pKey->m_rgkeyName, szKeyName, pKey->m_ikeyNameSize+1);
    }
#ifdef CSR_REQ 
    m_pem_privateKeyType= keyType;
    m_pem_privateKeySize= strlen(pemkey);
    m_pem_privateKey= (byte*)pemkey;
    if(m_pem_privateKey==NULL){
	RSA_free(opensslRsaKey);
        free(pKey);
        return false;
    }
     m_pem_privateKeyValid= true;
#endif

    m_privateKeyType= keyType;
    m_privateKeySize = sizeof(RSAKey);
    m_privateKey = (byte*)pKey;
    if(m_privateKey == NULL){
	RSA_free(opensslRsaKey);
        free(pKey);
        return false;
    }
    m_privateKeyValid= true;

	//HL: copied following code from generateCertRequest
    // serialize public key
    m_serializedpublicKey= pKey->SerializePublictoString();
    m_publicKeyBlockSize = pKey->m_iByteSizeM;
    if(m_serializedpublicKey == NULL) {
	RSA_free(opensslRsaKey);
        free(pKey);
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: can't serialize public key\n");
        return false;
    }
    m_serializedpublicKeySize = strlen(m_serializedpublicKey)+1;

    // get code digest
    if(!m_myHost->GetHostedMeasurement(&sizeCodeDigest, &codeDigestType, codeDigest)) {
	RSA_free(opensslRsaKey);
        free(pKey);
        fprintf(g_logFile, "generatequoteandcertifyKey: Can't get code digest\n");
        return false;
    }
    m_myMeasurementType = codeDigestType;
    if(sizeCodeDigest > m_myMeasurementSize) {
	RSA_free(opensslRsaKey);
        free(pKey);
        fprintf(g_logFile, "generatequoteandcertifyKey: code digest too big\n");
        return false;
    }
    
    m_myMeasurementSize = sizeCodeDigest;
    memset(m_myMeasurement, 0, m_myMeasurementSize);
    memcpy(m_myMeasurement, codeDigest, m_myMeasurementSize);
    m_myMeasurementValid= true;

	#ifdef TEST
		fprintf(g_logFile, "m_serializedpublicKey = %s, codeDigest size = %d\n", m_serializedpublicKey, sizeCodeDigest);
		PrintBytes("codeDigest = ", codeDigest, sizeCodeDigest);
				fprintf(g_logFile, "nonceStr: %s\n", nonceStr);
	#endif
	//HL: end of code copied from generateCertRequest

   //read csr in msg_buf
   /* FILE *fr;

    char *message;
    fr = fopen("/tmp/x509.csr", "r");
    if(fr == NULL){
	fprintf(g_logFile,"Error in opening file /tmp/x509.csr \n");
	return false;
    }*/
    /*create variable of stat*/
    //struct stat stp = { 0 };
    /*These functions return information about a file. No permissions are required on the file itself*/
    //stat("/tmp/x509.csr", &stp);
    /*determine the size of data which is in file*/
    //int filesize = stp.st_size;
    /*allocates the address to the message pointer and allocates memory*/
    /*message = (char *) malloc(sizeof(char) * filesize);
    if (fread(message, 1, filesize - 1, fr) == -1) {
	fprintf(g_logFile, "error in reading /tmp/x509.csr");*/
        //printf("\nerror in reading\n");
	/**close the read file*/
	//fclose(fr);
	/*free input string*/
	//free(message);
	//return false;
  // }
   //printf("\n\tEntered Message for Encode is = %s\n", message);
   memset(msg_buf,0,strlen(pem));
   memcpy(msg_buf, pem, strlen(pem));
   maxSize=strlen(pem);
   
   return true;
}

bool taoInit::generateCertRequest(u32 keyType, const char* szKeyName, 
                                const char* szSubjectName, const char* szSubjectId, char * nonceStr,
                                char* msg_buf, int maxSize)
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
    
    //some fields needed in the quote
    char			locality[8] = {0};
    char			pcrSelection[128]={0};
	int 			pcrSelected[MAXPCRS], numPcr;
	
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
    memset(m_myMeasurement, 0, m_myMeasurementSize);
    memcpy(m_myMeasurement, codeDigest, m_myMeasurementSize);
    m_myMeasurementValid= true;

	#ifdef TEST
		fprintf(g_logFile, "m_serializedpublicKey = %s, codeDigest size = %d\n", m_serializedpublicKey, sizeCodeDigest);
		PrintBytes("codeDigest = ", codeDigest, sizeCodeDigest);
				fprintf(g_logFile, "nonceStr: %s\n", nonceStr);
	#endif
	

    switch(quoteType) {

      default:
      case QUOTETYPENONE:
      case QUOTETYPETPM12RSA1024:
        fprintf(g_logFile, "GenerateQuoteAndCertifyKey: bad quote type\n");
        return false;

      case QUOTETYPETPM12RSA2048:
        // Construct quote body
        sprintf(quotedInfo, g_quotedkeyInfoTemplate, nonceStr, QUOTEMETHODTPM12RSA2048, 
                m_serializedpublicKey);
        //sprintf(quotedInfo, g_quotedkeyInfoTemplate, QUOTEMETHODTPM12RSA2048, 
        //        m_serializedpublicKey);
        szCanonicalQuotedBody= canonicalizeXML(quotedInfo);
        
        #ifdef TEST
		fprintf(stdout, "quotedInfo = %s\n", szCanonicalQuotedBody);
		
		#endif
        
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
        sprintf(quotedInfo, g_quotedkeyInfoTemplate, nonceStr, QUOTEMETHODSHA256FILEHASHRSA1024, 
                m_serializedpublicKey);
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
        sprintf(quotedInfo, g_quotedkeyInfoTemplate, nonceStr,
                QUOTEMETHODSHA256FILEHASHRSA1024,  m_serializedpublicKey);
        //sprintf(quotedInfo, g_quotedkeyInfoTemplate, 
        //        QUOTEMETHODSHA256FILEHASHRSA1024,  m_serializedpublicKey);
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
    
    #ifdef TEST
    fprintf(g_logFile, "quoteValue %s\n size of quoteValue %d", quoteValue, sizequoteValue);
    #endif

	//get the locality for TPM access
	sprintf(locality, "%d", m_myHost->locality());
	//get the PCR selection defined for the platform this program runs on
	
	if(m_myHost->getPcrSelection(pcrSelected, &numPcr))
	{
		constructPcrSelectionQuoteString(pcrSelection, pcrSelected, numPcr); 
	}else
		sprintf(pcrSelection, "");//if not TPM support (in case of nontpmprcore), leave it empty

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
    szQuote= encodeXMLQuote(0, NULL, sizeCodeDigest, codeDigest, locality, pcrSelection,
							szCanonicalQuotedBody, 
                            szHostKey, sizequoteValue, quoteValue);
                            
	#ifdef TEST
	fprintf(g_logFile, "encoded quote --\n%s\n", szQuote);
	#endif
						
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

void verify_cert(X509* cert, char *ca_bundlestr)
{
  if (ca_bundlestr && ca_bundlestr[0] == '\0') {
	fprintf(g_logFile,"ca_bundlestr is empty\n");
      //printf("ca_bundlestr is empty\n");
  }

  BIO              *certbio = NULL;
  BIO               *outbio = NULL;
  X509          *error_cert = NULL;
  X509_NAME    *certsubject = NULL;
  X509_STORE         *store = NULL;
  X509_STORE_CTX  *vrfy_ctx = NULL;
  int ret;

  /* ---------------------------------------------------------- *
   * These function calls initialize openssl for correct work.  *
   * ---------------------------------------------------------- */
  OpenSSL_add_all_algorithms();
  ERR_load_BIO_strings();
  ERR_load_crypto_strings();

  /* ---------------------------------------------------------- *
   * Create the Input/Output BIO's.                             *
   * ---------------------------------------------------------- */
  certbio = BIO_new(BIO_s_file());
  outbio  = BIO_new_fp(g_logFile, BIO_NOCLOSE);

  /* ---------------------------------------------------------- *
   * Initialize the global certificate validation store object. *
   * ---------------------------------------------------------- */
  if (!(store=X509_STORE_new()))
     BIO_printf(outbio, "Error creating X509_STORE_CTX object\n");

  /* ---------------------------------------------------------- *
   * Create the context structure for the validation operation. *
   * ---------------------------------------------------------- */
  vrfy_ctx = X509_STORE_CTX_new();

  /* ---------------------------------------------------------- *
   * Load the certificate and cacert chain from file (PEM).     *
   * ---------------------------------------------------------- */

  ret = X509_STORE_load_locations(store, ca_bundlestr, NULL);
 if (ret != 1)
    BIO_printf(outbio, "Error loading CA cert or chain file\n");

  /* ---------------------------------------------------------- *
   * Initialize the ctx structure for a verification operation: *
   * Set the trusted cert store, the unvalidated cert, and any  *
   * potential certs that could be needed (here we set it NULL) *
   * ---------------------------------------------------------- */
 X509_STORE_CTX_init(vrfy_ctx, store, cert, NULL);

  /* ---------------------------------------------------------- *
   * Check the complete cert chain can be build and validated.  *
   * Returns 1 on success, 0 on verification failures, and -1   *
   * for trouble with the ctx object (i.e. missing certificate) *
   * ---------------------------------------------------------- */
  ret = X509_verify_cert(vrfy_ctx);
  BIO_printf(outbio, "Verification return code: %d\n", ret);

  if(ret == 0 || ret == 1)
  BIO_printf(outbio, "Verification result text: %s\n",
             X509_verify_cert_error_string(vrfy_ctx->error));

  /* ---------------------------------------------------------- *
   * The error handling below shows how to get failure details  *
   * from the offending certificate.                            *
   * ---------------------------------------------------------- */
  if(ret == 0) {
    /*  get the offending certificate causing the failure */
    error_cert  = X509_STORE_CTX_get_current_cert(vrfy_ctx);
    certsubject = X509_NAME_new();
    certsubject = X509_get_subject_name(error_cert);
    BIO_printf(outbio, "Verification failed cert:\n");
    X509_NAME_print_ex(outbio, certsubject, 0, XN_FLAG_MULTILINE);
    BIO_printf(outbio, "\n");
  }

  /* ---------------------------------------------------------- *
   * Free up all structures                                     *
   * ---------------------------------------------------------- */
  X509_STORE_CTX_free(vrfy_ctx);
  X509_STORE_free(store);
  BIO_free_all(certbio);
  BIO_free_all(outbio);

}
bool parsex509CertResponse(const char* response, int outparamsize){
    //printf("Inside parsex509CertResponse \n");
    //fwrite(response, 1, outparamsize, stdout);
    BIO *bio_mem = BIO_new(BIO_s_mem());
    //BIO_puts(bio_mem, response);
    BIO_write(bio_mem, response, outparamsize);
    X509 * x509 = PEM_read_bio_X509(bio_mem, NULL, NULL, NULL);
    X509 * x = x509;
    //char *cert_chain="/tmp/cert_chain";

    struct sample_parameters parms1;

    //printf ("Initializing parameters to default values...\n");
    init_parameters (&parms1);

    //printf ("Reading config file...\n");
    parse_config (&parms1);

    //printf ("Final values:\n");
    //printf (" countryName : %s, stateOrProvinceName : %s, localityName : %s, organizationName : %s, organizationalUnitName : %s, commonName : %s, ca_cert_chain_path: %s \n ",
    //parms1.countryName, parms1.stateOrProvinceName, parms1.localityName, parms1.organizationName, parms1.organizationalUnitName, parms1.commonName, parms1.ca_cert_chain_path);

    char subject_from_config[2000];
//    /C=US/ST=NY/L=Albany/O=example.com/OU=Development/CN=Internal Project
    strcpy(subject_from_config, "/C=");
    strcat(subject_from_config, parms1.countryName);
    strcat(subject_from_config, "/ST=");
    strcat(subject_from_config, parms1.stateOrProvinceName);
    strcat(subject_from_config, "/L=");
    strcat(subject_from_config, parms1.localityName);
    strcat(subject_from_config, "/O=");
    strcat(subject_from_config, parms1.organizationName);
    strcat(subject_from_config, "/OU=");
    strcat(subject_from_config, parms1.organizationalUnitName);
    strcat(subject_from_config, "/CN=");
    strcat(subject_from_config, parms1.commonName);
    //printf("Generated Subject : %s",subject_from_config);   
    if(!strcmp(parms1.ca_cert_chain_path,"")==0){
	    verify_cert(x, parms1.ca_cert_chain_path);
    }
    BIO *bio_out = BIO_new_fp(g_logFile, BIO_NOCLOSE);

    //PEM_write_bio_X509(bio_out, x509);//STD OUT the PEM
    //X509_print(bio_out, x509);//STD OUT the details
    //X509_print_ex(bio_out, x509, XN_FLAG_COMPAT, X509_FLAG_COMPAT);//STD OUT the details

    //Version
    long l = X509_get_version(x509);
    BIO_printf(bio_out, "Version: %ld\n", l+1);

    //Serial Number
    int i=0;
    ASN1_INTEGER *bs = X509_get_serialNumber(x);
    BIO_printf(bio_out,"Serial: ");
    for(i=0; i<bs->length; i++)
    {
        BIO_printf(bio_out,"%02x",bs->data[i] );
    }
    BIO_printf(bio_out,"\n");

    //Signature Algorithm
    X509_signature_print(bio_out, x->sig_alg, NULL);

    //Issuer
    BIO_printf(bio_out,"Issuer: ");
    X509_NAME_print(bio_out,X509_get_issuer_name(x),0);
    BIO_printf(bio_out,"\n");

    //Validity Dates
    BIO_printf(bio_out,"Valid From: ");
    ASN1_TIME_print(bio_out,X509_get_notBefore(x));
    BIO_printf(bio_out,"\n");

    BIO_printf(bio_out,"Valid Until: ");
    ASN1_TIME_print(bio_out,X509_get_notAfter(x));
    BIO_printf(bio_out,"\n");

    //Subject
    BIO_printf(bio_out,"Subject: ");
    X509_NAME_print(bio_out,X509_get_subject_name(x),0);
    BIO_printf(bio_out,"\n");
    char buf[256];
    X509_NAME_oneline(X509_get_subject_name(x), buf, 256);
    //printf("\n Subject: %s", buf);
    //compare generated subject with config subject
    if(strcmp(buf, subject_from_config)==0)
	fprintf(g_logFile,"Subject Matches");
	//printf("Subject Matches");
    else{
	fprintf(g_logFile, "Subject from cert doesnot match with subject of CSR");
	BIO_free(bio_out);
    	BIO_free(bio_mem);
	return false;
    }	
    

    //Public Key
    EVP_PKEY *pkey=X509_get_pubkey(x);
    EVP_PKEY_print_public(bio_out, pkey, 0, NULL);
    EVP_PKEY_free(pkey);

    //Extensions
    X509_CINF *ci=x->cert_info;
    X509V3_extensions_print(bio_out, "X509v3 extensions", ci->extensions, X509_FLAG_COMPAT, 0);

    //Signature
    X509_signature_print(bio_out, x->sig_alg, x->signature);


    BIO_free(bio_out);
    BIO_free(bio_mem);
    X509_free(x509);
    return true;
}
//Handle x509 cert response
bool taoInit::handlex509CertResponse(const char* srv_response, int outparamsize){
    if(!parsex509CertResponse(srv_response, outparamsize)){
	fprintf(g_logFile, "server response invalid\n");	
	return false;
    }
    m_myCertificate = (char *)malloc(outparamsize);
    memset(m_myCertificate,0,outparamsize);
    memcpy(m_myCertificate, srv_response, outparamsize);
    m_myCertificateValid= true;
    m_myMeasurementValid= true;
    m_myCertificateType= EVIDENCECERT;
    m_myCertificateSize= outparamsize;
    return true;
}

//To be replaced by our won logic
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
    memset(m_myMeasurement, 0, m_myMeasurementSize);
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
    RSAKey*  pKey= new RSAKey();
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
	//this is the default method now, as NEWANDREORGANIZED is defined in Makefile (tcsd-rpcoresvc-g.mak/nontpmrpcorsvc-g.mak)
    pKey= RSAGenerateKeyPair(ikeySize);
#else
    pKey= generateRSAKeypair(ikeySize);
#endif

    if(pKey==NULL) {
        fprintf(g_logFile, "taoEnvironment::genprivateKeyPair: Can't generate RSA key pair\n");
        return false;
    }
    if(szKeyName!=NULL && strlen(szKeyName) < (KEYNAMEBUFSIZE-1)) {
        pKey->m_ikeyNameSize= strlen(szKeyName);
	memset(pKey->m_rgkeyName, 0, pKey->m_ikeyNameSize);
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

#ifdef TEST
	//fprintf(stdout, "config file size is %d\n", inparamsize);
	fprintf(g_logFile, "inparams: %s, size: %d\n", (char*)inparams, inparamsize);
#endif
	
	char * nonceStr=NULL;
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
				
				//HL: make sure outparams ends with null 
				outparams[outparamsize]='\0';
				//get the nonce string, pass it to generateCertRequest
				nonceStr = (char*)outparams;
				
				#ifdef TEST
				fprintf(g_logFile, "decodeVM2RP_GETRPHOSTQUOTE: outparams %s, \n outparams size: %d\n", (char*)outparams, outparamsize);
				#endif
				//clear the buf "inparams" and reuse it as buffer for the quote request message
				inparamsize= PARAMSIZE;
				memset(inparams, 0, inparamsize);
			#ifdef CSR_REQ		
			
				if(!generateCSR(KEYTYPERSA1024INTERNALSTRUCT, szKeyName, szSubjectName, szSubjectId, nonceStr, (char*)inparams, inparamsize)){					}
				size = strlen((char*)inparams);

	                        outparamsize= encodeRP2VM_GETRPHOSTQUOTE(size,(byte*)inparams, PARAMSIZE, outparams);
                                if(outparamsize<0) {
                                        fprintf(g_logFile, "RP2VM_GETRPHOSTQUOTE: encodeRP2VM_GETRPHOSTQUOTE buf too small\n");
                                        g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
                                        return retval;
                                }

                                retval = TAOINIT_INPROGRESS;

			#else
        	                if (! generateCertRequest ( KEYTYPERSA1024INTERNALSTRUCT, szKeyName, szSubjectName, szSubjectId, nonceStr, (char*)inparams, inparamsize)) {
				}
                
				size = strlen((char*)inparams)+1;
				
		        	outparamsize= encodeRP2VM_GETRPHOSTQUOTE(size,(byte*)inparams, PARAMSIZE, outparams);
				if(outparamsize<0) {
					fprintf(g_logFile, "RP2VM_GETRPHOSTQUOTE: encodeRP2VM_GETRPHOSTQUOTE buf too small\n");
					g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return retval;
				}
				
				retval = TAOINIT_INPROGRESS;
			#endif
				break;
				
		case RP2VM_RPHOSTQUOTERESPONSE:
				memset(outparams,0,sizeof(outparams));	
				if(!decodeVM2RP_RPHOSTQUOTERESPONSE(&outparamsize, outparams, inparams)) {
					fprintf(g_logFile, "RP2VM_RPHOSTQUOTERESPONSE: decodeVM2RP_RPHOSTQUOTERESPONSE failed\n");
					g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return retval;
				}
	
				#ifdef TEST
				fprintf(g_logFile, "decodeVM2RP_RPHOSTQUOTERESPONSE -- outparams: %s, size: %d\n", (char*)outparams, outparamsize);
				#endif
			#ifdef CSR_REQ
				//printf("\n length= %d",outparamsize);
				//fwrite(outparams,1,outparamsize,stdout);
				status = handlex509CertResponse((char*)outparams, outparamsize);
			#else	
				status = handleCertResponse((char*)outparams);
			#endif	
				size = (status)? sizeof(status): 0;
				
				memset(outparams, 0, PARAMSIZE);
		        outparamsize= encodeRP2VM_RPHOSTQUOTERESPONSE(size,(byte*) &status, PARAMSIZE, outparams);
				if(outparamsize<0) {
					fprintf(g_logFile, "RP2VM_RPHOSTQUOTERESPONSE: encodeRP2VM_RPHOSTQUOTERESPONSE buf too small\n");
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
