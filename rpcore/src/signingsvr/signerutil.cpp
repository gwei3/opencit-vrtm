//
//  File: signerutil.cpp
//	Author: Vinay Phegade
//  Description: cryptoUtility
//
//  Copyright (c) 2011, Intel Corporation. All rights reserved.
//  Incorporates contributions  (c) John Manferdelli.  All rights reserved.
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
#include <ctype.h>
#include "jlmUtility.h"
#include "cryptoHelper.h"
#include "bignum.h"
#include "rpapi.h"
#include "mpFunctions.h"


const char* g_aikTemplate=
"<ds:SignedInfo>\n" \
"    <ds:CanonicalizationMethod Algorithm=\"http://www.rp.com/2011/Xml/canonicalization/tinyxmlcanonical#\" />\n" \
"    <ds:SignatureMethod Algorithm=\"http://www.rp.com/2011/Xml/algorithms/rsa1024-sha256-pkcspad#\" />\n" \
"    <Certificate Id='%s' version='1'>\n" \
"        <SerialNumber>20110930001</SerialNumber>\n" \
"        <PrincipalType>Hardware</PrincipalType>\n" \
"        <IssuerName>manferdelli.com</IssuerName>\n" \
"        <IssuerID>manferdelli.com</IssuerID>\n" \
"        <ValidityPeriod>\n" \
"            <NotBefore>2011-01-01Z00:00.00</NotBefore>\n" \
"            <NotAfter>2021-01-01Z00:00.00</NotAfter>\n" \
"        </ValidityPeriod>\n" \
"        <SubjectName>//www.rp.com/Keys/attest/0001</SubjectName>\n" \
"        <SubjectKey>\n" \
"<ds:KeyInfo xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" " \
"            KeyName=\"%s\">\n" \
"    <KeyType>RSAKeyType</KeyType>\n" \
"    <ds:KeyValue>\n" \
"        <ds:RSAKeyValue size='%d'>\n" \
"            <ds:M>%s</ds:M>\n" \
"            <ds:E>AAAAAAABAAE=</ds:E>\n" \
"        </ds:RSAKeyValue>\n" \
"    </ds:KeyValue>\n" \
"</ds:KeyInfo>\n" \
"        </SubjectKey>\n" \
"        <SubjectKeyID>%s</SubjectKeyID>\n" \
"        <RevocationPolicy>Local-check-only</RevocationPolicy>\n" \
"    </Certificate>\n" \
"</ds:SignedInfo>\n" ;


const char* g_aikXmlTemplate=
"<ds:KeyInfo xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" " \
"            KeyName=\"%s\">\n" \
"    <KeyType>RSAKeyType</KeyType>\n" \
"    <ds:KeyValue>\n" \
"        <ds:RSAKeyValue size='%d'>\n" \
"            <ds:M>%s</ds:M>\n" \
"            <ds:E>AAAAAAABAAE=</ds:E>\n" \
"        </ds:RSAKeyValue>\n" \
"    </ds:KeyValue>\n" \
"</ds:KeyInfo>\n";

inline byte fromHextoVal(char a, char b)
{
    byte x= 0;

    if(a>='a' && a<='f')
        x= (((byte) (a-'a')+10)&0xf)<<4;
    else if(a>='A' && a<='F')
        x= (((byte) (a-'A')+10)&0xf)<<4;
    else
        x= (((byte) (a-'0'))&0xf)<<4;

    if(b>='a' && b<='f')
        x|= ((byte) (b-'a')+10)&0xf;
    else if(b>='A' && b<='F')
        x|= ((byte) (b-'A')+10)&0xf;
    else
        x|= ((byte) (b-'0'))&0xf;

    return x;
}


int MyConvertFromHexString(const char* szIn, int iSizeOut, byte* rgbBuf)
{
    char    a, b;
    int     j= 0;

    while(*szIn!=0) {
        if(*szIn=='\n' || *szIn==' ') {
            szIn++;
            continue;
        }
        a= *(szIn++);
        b= *(szIn++);
        if(a==0 || b==0)
            break;
        rgbBuf[j++]= fromHextoVal(a, b);
    }
    return j;
}
bool SignHexModulus(const char* szKeyFile, const char* szInFile, const char* szOutFile)
{
    bool        fRet= true;
    TiXmlNode*  pNode= NULL;
    RSAKey*     pRSAKey= NULL;
    char        rgBase64[1024];
    int         size= 1024;
    char*       szToHash= NULL;
    char        szSignedInfo[4096];

    
    char* modString= readandstoreString(szInFile); 
    if(modString==NULL) {
        fprintf(stdout /*g_logFile*/, "Couldn't open modulusfile %s\n", szInFile);
        return false;
    }

    byte    modHex[1024];
    int     modSize=  MyConvertFromHexString(modString, 1024, modHex);
 
    if(szKeyFile==NULL) {
        fprintf(stdout /*g_logFile*/, "No Key file\n");
        return false;
    }
    if(szInFile==NULL) {
        fprintf(stdout /*g_logFile*/, "No Input file\n");
        return false;
    }
    if(szOutFile==NULL) {
        fprintf(stdout /*g_logFile*/, "No Output file\n");
        return false;
    }

    try {

        pRSAKey= (RSAKey*)ReadKeyfromFile_i(szKeyFile);
        if(pRSAKey==NULL)
            throw "Cant open Keyfile\n";
        if(((KeyInfo*)pRSAKey)->m_ukeyType!=RSAKEYTYPE) {
            delete (KeyInfo*) pRSAKey;
            pRSAKey= NULL;
            throw "Wrong key type for signing\n";
        }

 //       fprintf(stdout /*g_logFile*/, "\n");
 //       pRSAKey->printMe();
 //       fprintf(stdout /*g_logFile*/, "\n");

        // construct key XML from modulus
        const char*   szCertid= "www.rp.com/certs/000122";
        const char*   szKeyName= "Gauss-AIK-CERT";
        byte    revmodHex[1024];

	//The cryptUtil/rpapicryputil has not been changed for signinging
        // a hex mode.

        //revmemcpy(revmodHex, modHex, modSize);
        memcpy(revmodHex, modHex, modSize);
        if(!toBase64(modSize, revmodHex, &size, rgBase64))
            throw "Cant base64 encode modulus value\n";

        const char*   szKeyId= "/Gauss/AIK";
        int     iNumBits= ((size*6)/1024)*1024;
        
        sprintf(szSignedInfo, g_aikTemplate, szCertid, 
                szKeyName, iNumBits, rgBase64, szKeyId);
  

        // read input file
        TiXmlDocument toSignDoc;
        if(!toSignDoc.Parse(szSignedInfo))
            throw "Can't parse signed info\n";
        
        pNode= Search(toSignDoc.RootElement(), "ds:SignedInfo");
        if(pNode==NULL) {
            fprintf(stdout /*g_logFile*/, "Can't find SignedInfo\n");
            throw "Can't find SignedInfo\n";
        }

        // Canonicalize
        szToHash= canonicalize(pNode);
        if(szToHash==NULL) 
            throw "Can't canonicalize\n";

        char* szSignature= constructXMLRSASha256SignaturefromSignedInfoandKey(*pRSAKey,
                                                szKeyId, szToHash);
        if(szSignature==NULL) {
            fprintf(stdout /*g_logFile*/, "Can't construct signature\n");
            throw "Can't construct signature\n";
        }
        if(!saveBlobtoFile(szOutFile, (byte*)szSignature, strlen(szSignature)+1))
            throw "Can't save data to file\n" ;

#ifdef TEST
        fprintf(stdout, "Signed TPM key is in %s\n", szOutFile);
#endif
    }
    catch(const char* szError) {
        fRet= false;
        fprintf(stdout, "Sign error: %s\n", szError);
    }

    if(szToHash!=NULL) {
        free(szToHash);
        szToHash= NULL;
    }
    if(pRSAKey!=NULL) {
        delete pRSAKey;
        pRSAKey= NULL;
    }

    return fRet;
}

bool GenXmlKey( const char* szInFile, const char* szOutFile)
{
    bool        fRet= true;
    TiXmlNode*  pNode= NULL;
    RSAKey*     pRSAKey= NULL;
    char        rgBase64[1024];
    int         size= 1024;
    char*       szToHash= NULL;
    char        szSignedInfo[4096];

    
    char* modString= readandstoreString(szInFile); 
    if(modString==NULL) {
        fprintf(stdout /*g_logFile*/, "Couldn't open modulusfile %s\n", szInFile);
        return false;
    }

    byte    modHex[1024];
    int     modSize=  MyConvertFromHexString(modString, 1024, modHex);
 
 
    if(szInFile==NULL) {
        fprintf(stdout /*g_logFile*/, "No Input file\n");
        return false;
    }
    if(szOutFile==NULL) {
        fprintf(stdout /*g_logFile*/, "No Output file\n");
        return false;
    }

    try {

 
        // construct key XML from modulus
        const char*   szCertid= "www.rp.com/certs/000122";
        const char*   szKeyName= "Gauss-AIK-CERT";
        byte    revmodHex[1024];

	//The cryptUtil/rpapicryputil has not been changed for signinging
        // a hex mode.

        //revmemcpy(revmodHex, modHex, modSize);
        memcpy(revmodHex, modHex, modSize);
        if(!toBase64(modSize, revmodHex, &size, rgBase64))
            throw "Cant base64 encode modulus value\n";

        const char*   szKeyId= "/Gauss/AIK";
        int     iNumBits= ((size*6)/1024)*1024;

       sprintf(szSignedInfo, g_aikXmlTemplate, 
                szKeyName, iNumBits, rgBase64);

 
 
        if(!saveBlobtoFile(szOutFile, (byte*)szSignedInfo, strlen(szSignedInfo)))
            throw "Can't save data to file\n" ;

#ifdef TEST
        fprintf(stdout, "Signed TPM key is in %s\n", szOutFile);
#endif
    }
    catch(const char* szError) {
        fRet= false;
        fprintf(stdout, "Sign error: %s\n", szError);
    }

    if(szToHash!=NULL) {
        free(szToHash);
        szToHash= NULL;
    }
  
    return fRet;
}


const char*   szSigHeader= 
          "<ds:Signature xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" Id='uniqueid'>\n";
const char*   szSigValueBegin= "    <ds:SignatureValue>    \n";
const char*   szSigValueEnd= "\n    </ds:SignatureValue>\n";
const char*   szSigTrailer= "</ds:Signature>\n";


bool Sign(const char* szKeyFile, const char* szAlgorithm, const char* szInFile, const char* szOutFile)
{
    char*   szBuf[8192];
    int     bufLen= 8192;
    RSAKey* pKey= (RSAKey*)ReadKeyfromFile_i(szKeyFile);

    if(pKey==NULL) {
        fprintf(stdout /*g_logFile*/, "Sign: Can't get key from keyfile %s\n", szKeyFile);
        return false;
    }

    if(!getBlobfromFile(szInFile, (byte*)szBuf, &bufLen)) {
        fprintf(stdout /*g_logFile*/, "Sign: Can't read signedInfo from %s\n", szInFile);
        return false;
    }
    szBuf[bufLen]= 0;
    char* szSignedInfo= XMLCanonicalizedString((const char*) szBuf);
    if(szSignedInfo==NULL) {
        fprintf(stdout /*g_logFile*/, "Sign: Cant canonicalize signedInfo\n");
        return false;
    }

    char* szSig= constructXMLRSASha256SignaturefromSignedInfoandKey(*pKey,
                                                "newKey", szSignedInfo);
    if(szSig==NULL) {
        fprintf(stdout /*g_logFile*/, "Sign: Cant construct signature\n");
        return false;
    }

    if(!saveBlobtoFile(szOutFile, (byte*)szSig, strlen(szSig)+1)) {
        fprintf(stdout /*g_logFile*/, "Sign: Cant save %s\n", szOutFile);
        return false;
    }
    return true;
}

bool GenKey(const char* szKeyType, const char* szOutFile)
{
	int nsize = 1024;
   

    if(strcmp(szKeyType, "RSA2048")==0) {
        nsize = 2048;
    }
    
    RSAKey* pKey= RSAGenerateKeyPair(nsize);
    char*   szKeyInfo= pKey->SerializetoString();
    if(!saveBlobtoFile(szOutFile, (byte*)szKeyInfo, strlen(szKeyInfo)))
        return false;
        
    return true;
}

int main(int an, char** av)
{
    const char*   szInFile= NULL;
    const char*   szKeyType= NULL;
    const char*   szOutFile= NULL;
    const char*   szAlgorithm= NULL;
    const char*   szKeyFile= NULL;
    bool          fRet;
    int           i;

      if( an < 4 ) {
            fprintf(stdout, "       signerutil -GenKey RSA1024 output-file\n");
            fprintf(stdout, "       signerutil -Sign keyfile rsa1024-sha256-pkcspad inputfile outputfile\n");
            fprintf(stdout, "       signerutil -SignHexModulus keyfile input-file output-file\n");
            return 0;
       }

    for(i=0; i<an; i++) {
        if ( strcmp(av[i], "-help")==0 )  {
            fprintf(stdout, "       signerutil -GenKey RSA1024 output-file\n");
            fprintf(stdout, "       signerutil -Sign keyfile rsa1024-sha256-pkcspad inputfile outputfile\n");
            fprintf(stdout, "       signerutil -SignHexModulus keyfile input-file output-file\n");
            return 0;
        }

        if(strcmp(av[i], "-GenKey")==0) {
            if(an<(i+3)) {
                fprintf(stdout, "Too few arguments: RSA1024 output-file\n");
                return 1;
            }
            szKeyType= av[i+1];
            szOutFile= av[i+2];
            initCryptoRand();
	    initBigNum();
	    fRet= GenKey(szKeyType, szOutFile);
            closeCryptoRand();

            break;
        }
        
        if(strcmp(av[i], "-Sign")==0) {
            if(an<(i+4)) {
                fprintf(stdout, "Too few arguments: key-file rsa2048-sha256-pkcspad input-file output-file\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szAlgorithm= av[i+2];
            szInFile= av[i+3];
            szOutFile= av[i+4];
            
            initCryptoRand();
			initBigNum();
            fRet= Sign(szKeyFile, szAlgorithm, szInFile, szOutFile);
            closeCryptoRand();
            break;
        }

        if(strcmp(av[i], "-SignHexModulus")==0) {
            if(an<(i+3)) {
                fprintf(stdout, "Too few arguments: key-file input-file output-file\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szInFile= av[i+2];
            szOutFile= av[i+3];
            
            initCryptoRand();
			initBigNum();
			fRet= SignHexModulus(szKeyFile, szInFile, szOutFile);
			closeCryptoRand();
            break;
        }
        
        if(strcmp(av[i], "-GenXmlKey")==0) {
            if(an<(i+3)) {
                fprintf(stdout, "Too few arguments: key-file input-file output-file\n");
                return 1;
            }
   
            szInFile= av[i+1];
            szOutFile= av[i+2];
            
            initCryptoRand();
			initBigNum();
			fRet= GenXmlKey(szInFile, szOutFile);
			closeCryptoRand();
            break;
        }
    }

    return 0;
}


