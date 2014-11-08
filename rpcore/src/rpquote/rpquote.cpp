
//  File: rptrust.cpp
//    Vinay Phegade 
//
//  Description: Reliance Point Trust establishment services 
//
//  Copyright (c) 2011, Intel Corporation. 
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "rptrust.h"
#include "jlmUtility.h"
#include "rpapilocal.h"
#include "channelcoding.h"
#include <ctype.h>
#include "cert.h"
#include "quote.h"
#include "cryptoHelper.h"
#include "validateEvidence.h"
#include "rptrust.h"
#include "tcpchan.h"
#include "pyifc.h"

const char*   g_szRpNonceTemplate= "<Nonce> %s </Nonce>\n";
const char*   g_szRpQuoteTemplate=
"<Quote format='xml'>\n"\
"    %s\n" \
"    <HostedCodeDigest alg='%s'> %s </HostedCodeDigest>\n" \
"%s\n" \
"        <QuoteValue>\n" \
"%s\n" \
"        </QuoteValue>\n" \
"</Quote> \n<HostCert>%s</HostCert>\n%s\n";

const char* g_RpQuotedkeyInfoTemplate=
"<QuotedInfo>\n" \
"    <ds:CanonicalizationMethod Algorithm=\"http://www.rpcore.com/2011/Xml/canonical248ization/tinyxmlcanonical#\"/>  " \
"    <ds:QuoteMethod Algorithm=\"%s\"/>\n" \
"<extdata>%s</extdata>\n" \
"</QuotedInfo>\n";

#define TAG_QUOTE_END      "</Quote>"
#define TAG_HOSTCERT_START "<HostCert>"
#define TAG_HOSTCERT_END   "</HostCert>"
#define TAG_EVIDENCE_START "<EvidenceList"
#define TAG_QINFO_START     "<QuotedInfo>"
#define TAG_QINFO_END 		"</QuotedInfo>"

#define PARAMSIZE 8192
#define QUOTEMETHODSHA256FILEHASHRSA2048    "Quote-Sha256FileHash-RSA2048"
#define QUOTEMETHODSHA256FILEHASHRSA1048    "Quote-Sha256FileHash-RSA1048"

#define QUOTETYPESHA256FILEHASHRSA1024  3
#define QUOTETYPESHA256FILEHASHRSA2048  4



static TiXmlNode* findNode(TiXmlNode* pNode, const char* szElementName)
{
    TiXmlNode* pNode1;

    while(pNode) {
		
        if(pNode->Type()==TiXmlNode::TINYXML_ELEMENT) {
            if(strcmp(((TiXmlElement*)pNode)->Value(),szElementName)==0) {
                return pNode;
            }
        }
        
        pNode1= findNode(pNode->FirstChild(),szElementName);
        if(pNode1!=NULL)
           return pNode1; 
           
        pNode= pNode->NextSibling();
    }

    return NULL;
}

static const char* findAttr(TiXmlNode* pRoot, const char* szElementName, char* szAttrName)
{
	TiXmlNode* 	pNode = NULL;

	pNode  = findNode(pRoot, szElementName);
	if (!pNode )
		return NULL;
		
	const char* szA= ((TiXmlElement*) pNode)->Attribute (szAttrName);
    if(szA==NULL) {
         return NULL;
    }

    return strdup(szA);;
}

static const char* findValueOf(TiXmlNode* pRoot, const char* szElementName) {
	
	TiXmlNode* 	pchild = NULL;
	TiXmlNode* 	pnode = NULL;
	
		pnode  = findNode(pRoot, szElementName);
		if (!pnode )
			return NULL;
			
		pchild = pnode->FirstChild();
		if (!pchild)
			return NULL;
	
		return pchild->Value();
}

int VerifyHostedComponentQuote(char* szQuoteData, char* szRoot ){
	
	TiXmlDocument   	doc;	
    PrincipalCert   oCert;
    PrincipalCert   rCert;
    RSAKey* pKeyInfo= NULL;
	
    RSAKey rootKey;

	
	const char* value;
	int rc = -1;
	Sha256      oHash;
	int size_nonce=1024;
	byte nonce[1024];
	
  // this is the hash of the Quote XML
    int         sizequotedHash= SHA256DIGESTBYTESIZE;
    byte        rgHash[SHA256DIGESTBYTESIZE];       

    // this is the rpcore signed quote value
    int         sizequoteValue= 512;
    byte        quoteValue[512];                    

    // this is my measurement
    u32         codeDigestType= 0;
    int         sizeCodeDigest= SHA256DIGESTBYTESIZE;
    byte        codeDigest[SHA256DIGESTBYTESIZE];   	
	
	byte        rgQuotedHash[SHA256DIGESTBYTESIZE];
		
	char*   szXMLQuote = NULL;    
    char*   szXMLQuoteEnd = NULL;
	char*   szCert= NULL;   
    char*   szCertEnd= NULL;  
    char*   szEvidence = NULL;
    char* 	szLocality = NULL;
    char 	c1, c2;
    char* 	szQinfo = NULL, *szQinfoEnd = NULL;
    int sizeQinfo = 0;
    
    int hashType = SHA256HASH;
    
   if(szQuoteData==NULL)
        return -1;
    
    
    	//parse quote data componenents
	szEvidence = strstr(szQuoteData, TAG_EVIDENCE_START);	
	
	szCert = strstr(szQuoteData, TAG_HOSTCERT_START);
	if (! szCert )
		return -1;
	
	szCert += strlen(TAG_HOSTCERT_START);
	
	//find end of the certificate
	szCertEnd = strstr(szQuoteData, TAG_HOSTCERT_END);
	if (! szCertEnd )
		return -1;
		
	//WARNING: modifying callers buffer
	c1 = *szCertEnd;
	*szCertEnd = '\0';
	
	szXMLQuote = szQuoteData;
	
	szXMLQuoteEnd = strstr(szQuoteData, TAG_QUOTE_END);
	if (!szXMLQuoteEnd )
		return -1;
	
	szXMLQuoteEnd = szXMLQuoteEnd + strlen(TAG_QUOTE_END) + 1;
	
	c2 = *szXMLQuoteEnd;
	*szXMLQuoteEnd = '\0';
	
	szQinfo = strstr(szXMLQuote, TAG_QINFO_START);
	szQinfoEnd = strstr(szXMLQuote, TAG_QINFO_END);
	if (!szQinfoEnd )
		return -1;
	
	szQinfoEnd = szQinfoEnd + strlen(TAG_QINFO_END)+1;
	
	sizeQinfo = szQinfoEnd - szQinfo;
	
    if(!doc.Parse(szXMLQuote)) {
        return -1;
    }   
    
	TiXmlElement*   pRoot= doc.RootElement();
	
	if ( pRoot ) {
			
		const char* sznonce = findValueOf(pRoot, "Nonce");
		const char* szdigest = findValueOf(pRoot, "HostedCodeDigest");
		
		const char* szdigest_alg =  findAttr(pRoot, "HostedCodeDigest", "alg");
			
		const char* szqval = findValueOf(pRoot, "QuoteValue"); 
		
		//calculate input hash from the data in quote
		 if(!fromBase64(strlen(sznonce), sznonce, &size_nonce, nonce)) {     
			return -1;
		}
		
		//calculate input hash from the data in quote
		if(!fromBase64(strlen(szdigest), szdigest, &sizeCodeDigest, codeDigest)) {     
			return -1;
		}
		//calculate input hash from the data in quote
		if(!fromBase64(strlen(szqval), szqval, &sizequoteValue, quoteValue)) {     
			return -1;
		}
		
		// hash it - todo replace it with rpcrypto api
		oHash.Init();   
		oHash.Update((byte*)nonce, size_nonce);
		oHash.Update((byte*) szQinfo, sizeQinfo);
		oHash.Final();
		oHash.GetDigest(rgHash);
		sizequotedHash= SHA256DIGESTBYTESIZE;
		
		
		   // Compute quote
		if(!sha256quoteHash(0, NULL, sizequotedHash, rgHash, sizeCodeDigest, 
											codeDigest, rgQuotedHash)) {
            return -1;
        }

		if(!oCert.init(szCert)) {
			return -1;
		}
	  
		if(!oCert.parsePrincipalCertElements()) {

			return -1;
		}

		// check quote
		pKeyInfo = (RSAKey*) oCert.getSubjectKeyInfo();
		if(pKeyInfo==NULL) {
			return -1;
		}
		
		return RSAVerify(*(RSAKey*)pKeyInfo, hashType, rgQuotedHash,
                               quoteValue);
	}
	rc = 0;
	
	return 0;
}




int VerifyHostQuote(char* szQuoteData, char* szRoot)
{
    char*   szAlg= NULL;
    char*   szNonce= NULL;
    char*   szDigest= NULL;
    char*   szQuotedInfo= NULL;
    char*   szQuoteValue= NULL;

    char*   szXMLQuote = NULL;    
    char*   szXMLQuoteEnd = NULL;
	   
    char*   szCert= NULL;   
    char*   szCertEnd= NULL;  
    char*   szEvidence = NULL;
    char* 	szLocality = NULL;
    char c1, c2;
    

    char*   szquoteKeyInfo= NULL;
    char*   szquotedKeyInfo= NULL;

    PrincipalCert   oCert;
    PrincipalCert   rCert;
    
    evidenceList oEvid;
    Quote    oQuote;
 
    RSAKey* pAIKKey= NULL;
	
    RSAKey rootKey;

    bool    fRet= true;

#ifdef TEST 
	printf("Entered Internal VerifyQuote\n");
#endif
	//parse quote data componenents
	szEvidence = strstr(szQuoteData, TAG_EVIDENCE_START);	
	
	szCert = strstr(szQuoteData, TAG_HOSTCERT_START);
	if (! szCert )
		return false;
	
	szCert += strlen(TAG_HOSTCERT_START);
	
	//find end of the certificate
	szCertEnd = strstr(szQuoteData, TAG_HOSTCERT_END);
	if (! szCertEnd )
		return false;
		
	//WARNING: modifying callers buffer
	c1 = *szCertEnd;
	*szCertEnd = '\0';
	
	szXMLQuote = szQuoteData;
	
	szXMLQuoteEnd = strstr(szQuoteData, TAG_QUOTE_END);
	if (!szXMLQuoteEnd )
		return false;
	
	szXMLQuoteEnd = szXMLQuoteEnd + strlen(TAG_QUOTE_END) + 1;
	
	c2 = *szXMLQuoteEnd;
	*szXMLQuoteEnd = '\0';


#ifdef TEST
	fprintf(stdout, "Root key: "); 
	//rootKey.printMe();
	fprintf(stdout, "Root key end  ------------------------------------------"); 
#endif
	
    if(!oCert.init(szCert)) {
#ifdef TEST 
        fprintf(stdout, "%s: Can't parse cert\n", __FUNCTION__);
#endif
        return false;
    }
  
    if(!oCert.parsePrincipalCertElements()) {
#ifdef TEST 
        fprintf(stdout, "%s: Can't get principal cert elements\n", __FUNCTION__);
#endif
        return false;
    }

    // check quote
    pAIKKey = (RSAKey*) oCert.getSubjectKeyInfo();
    if(pAIKKey==NULL) {
#ifdef TEST 
        fprintf(stdout, "%s: Cant get quote keyfromkeyInfo\n", __FUNCTION__);
#endif
        return false;
    }


    if(!oQuote.init(szXMLQuote)) {
#ifdef TEST 
        fprintf(stdout, "%s: Can't parse quote\n", __FUNCTION__);
#endif
        return false;
    }

    // decode request
    szAlg= oQuote.getQuoteAlgorithm();
    szQuotedInfo= oQuote.getCanonicalQuoteInfo();
    szQuoteValue= oQuote.getQuoteValue();
    szNonce= oQuote.getnonceValue();
    szDigest= oQuote.getcodeDigest();
    szLocality = oQuote.getQuoteLocality();

    if(!checkXMLQuote(szAlg, szQuotedInfo, szNonce,
                      szDigest, pAIKKey, szQuoteValue, szLocality)) {
#ifdef TEST 
        fprintf(stdout, "%s: cant verify quote\n", __FUNCTION__);
#endif
        fflush(stdout);
        fRet= false;
        goto cleanup;
    }
 
#ifdef TEST 
    fprintf(stdout, "%s: Quote verified with attached cert, checking evidence chain\n", __FUNCTION__);
#endif
 
    if(szEvidence!=NULL) {
		
        TiXmlDocument   doc;
        
        if(!doc.Parse(szEvidence)) {
#ifdef TEST 
            fprintf(stdout, "%s: can't parse evidence list\n", __FUNCTION__);
#endif
            fRet= false;
            goto cleanup;
        }
       
        if(!oEvid.parseEvidenceList(doc.RootElement())) {
#ifdef TEST 
            fprintf(stdout, "%s: can't parse evidence list\n", __FUNCTION__);
#endif
            fRet= false;
            goto cleanup;
        }
        
		if (! rootKey.ParsefromString(szRoot) )
			return false;
		
		if (! rootKey.getDataFromDoc())
			return false;
		        
        if(!oEvid.validateEvidenceList(&rootKey)) {
#ifdef TEST 
            fprintf(stdout, "%s: can't validate evidence list\n", __FUNCTION__);
#endif
            fRet= false;
            goto cleanup;
        }
    }

	fRet = true;

cleanup:
	return fRet;
}


int GenHostedComponentQuote(int sizenonce, byte* nonce, char* publicKeyXML, char** ppQuoteXML)
{
    bool rc = false;
    char buf[1028*16];
    char szN[1028*16];
    int nsize = 512;
	
    // this is host key evidence
    int 	sizeHostCert = 1028*16;
    char 	szHostCert[1028*16];
    int         sizeEvidence= 1028*16;
    char        szEvidence[1028*16];                   

    // this is the Quote XML
    char        quotedInfo[1028*16];                   

    // this is the canonicalize Quote XML
    char*       szCanonicalQuotedBody= NULL;        
    Sha256      oHash;

    // this is the hash of the Quote XML
    int         sizequotedHash= SHA256DIGESTBYTESIZE;
    byte        rgHash[SHA256DIGESTBYTESIZE];       

    // this is the TPM signed quote value
    int         sizequoteValue= 512;
    byte        quoteValue[512];                    

    // this is my measurement
    u32         codeDigestType= 0;
    int         sizeCodeDigest= SHA256DIGESTBYTESIZE;
    byte        codeDigest[SHA256DIGESTBYTESIZE];   


    char*   szquoteValue= NULL;
    //char*   szQuote= NULL;
    char*   szNonce= NULL;
    char*   szCodeDigest= NULL;
	
    char*   szdigestAlg= "SHA256";
    char*   szQuoteMethod = QUOTEMETHODSHA256FILEHASHRSA1024;
	

	
    // this is the final formatted Quote
    u32    quoteType=0;        

    quotedInfo[0]= 0;

    //dummy parameters
    u32 type = 0;
    char* indata ="";
	
    // get code digest from rpcoreservice
    rc = doTrustOp(VM2RP_GETPROGHASH, &codeDigestType, 0, (byte*)indata, &sizeCodeDigest, (byte*)codeDigest);
    if (rc <= 0)
    {
#ifdef TEST
        fprintf(stdout, "%s: Can't get code digest\n", __FUNCTION__);
#endif
        rc = false;
        goto cleanup;
    }
    
	if(sizenonce > 0) {
        if(!toBase64(sizenonce, nonce, &nsize, szN)) {
#ifdef TEST
            fprintf(stdout, "%s: cant transform nonce to base64\n", __FUNCTION__);
#endif
            goto cleanup;
        }
        sprintf(buf, g_szRpNonceTemplate, szN);
        szNonce= strdup(buf);
    }
    else
        szNonce= strdup("");

    nsize= 256;
    if(!toBase64(sizeCodeDigest, codeDigest, &nsize, szN)) {
#ifdef TEST
        fprintf(stdout, "%s: cant transform codeDigest to base64\n", __FUNCTION__);
#endif
        goto cleanup;
    }
    
    szCodeDigest= strdup(szN);
    if(sizeCodeDigest == 20)
        szdigestAlg= "SHA1";
        
    //hardcoded 
    quoteType = QUOTETYPESHA256FILEHASHRSA1024;

	if (quoteType == QUOTETYPESHA256FILEHASHRSA2048 )
		szQuoteMethod = QUOTEMETHODSHA256FILEHASHRSA2048;
		
 // Construct quote body
	sprintf(quotedInfo, g_RpQuotedkeyInfoTemplate, szQuoteMethod, publicKeyXML);
				
    szCanonicalQuotedBody= canonicalizeXML(quotedInfo);
	if(szCanonicalQuotedBody==NULL) {
#ifdef TEST
		fprintf(stdout, "%s: Can't canonicalize quoted info\n", __FUNCTION__);
#endif
		rc = false;
		goto cleanup;
    }
    
   // hash it - todo replace it with rpcrypto api
	oHash.Init();
	oHash.Update((byte*)nonce, sizenonce);
	oHash.Update((byte*) szCanonicalQuotedBody, strlen(szCanonicalQuotedBody));
	oHash.Final();
	oHash.GetDigest(rgHash);
	sizequotedHash= SHA256DIGESTBYTESIZE;
    
    //pb ("quoted hash is \n", rgHash, sizequotedHash);
    
    // Do attest
        
    rc = doTrustOp(VM2RP_ATTESTFOR, &quoteType, sizequotedHash, rgHash, &sizequoteValue, quoteValue);
    if(rc <= 0) {
#ifdef TEST
        fprintf(stdout, "%s: Can't Attest Key\n", __FUNCTION__);
#endif
        rc = false;
        return rc;
    }

    // Get the certificate
    rc = doTrustOp(VM2RP_GETOSCERT, &type, strlen(indata), (byte*)indata, &sizeHostCert, (byte*)szHostCert);
    if( rc <= 0) {
#ifdef TEST
        fprintf(stdout, "%s: Can't get Host cert\n", __FUNCTION__);
        rc = false;
#endif
        return rc;
    }

    // Get evidence list
    rc = doTrustOp(VM2RP_GETOSCREDS, &type, strlen(indata), (byte*)indata, &sizeEvidence, (byte*)szEvidence);
    if(rc <= 0) {
        szEvidence[0]= '\0';
#ifdef TEST
        fprintf(stdout, "%s: No evidence returned\n", __FUNCTION__);
#endif
	}



    nsize= 512;
    if(!toBase64(sizequoteValue, quoteValue, &nsize, szN)) {
#ifdef TEST
        fprintf(stdout, "%s: cant transform quoted value to base64\n", __FUNCTION__);
#endif
        goto cleanup;
    }
    
    szquoteValue= strdup(szN);

    sprintf(buf, g_szRpQuoteTemplate, szNonce, szdigestAlg, szCodeDigest,
						szCanonicalQuotedBody, szquoteValue, szHostCert, szEvidence);
    
    *ppQuoteXML= strdup(buf);
	 
     rc = true;
	 
cleanup:

    if(szCodeDigest!=NULL) {
        free(szCodeDigest);
        szCodeDigest= NULL;
    }
    
    if(szquoteValue!=NULL) {
        free(szquoteValue);
        szquoteValue= NULL;
    }
    
    if(szNonce!=NULL) {
        free(szNonce);
        szNonce= NULL;
    }

    return rc;
}
