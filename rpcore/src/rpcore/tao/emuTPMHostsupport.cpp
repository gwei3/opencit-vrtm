//  File: TPMHostsupport.cpp
//      John Manferdelli
//  Description:  TPM interface for trusted services
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


// -------------------------------------------------------------------------


#ifdef TPMSUPPORT


#include "jlmTypes.h"
#include "logging.h"
#include "jlmcrypto.h"
#include "jlmUtility.h"
#include "modesandpadding.h"
#include "sha256.h"
#include "tao.h"
#include "bignum.h"
#include "mpFunctions.h"
#ifndef NEWANDREORGANIZED
#include "rsaHelper.h"
#else
#include "cryptoHelper.h"
#endif
#include "trustedKeyNego.h"
#include "hashprep.h"
#include "vTCIDirect.h"
//#include "tcIO.h"

#include <string.h>
#include <time.h>
#include "TPMHostsupport.h"

// -------------------------------------------------------------------------




RSAKey g_rsa;
symKey g_aes;

byte   g_pcrs[] = "ad0e10f1052fd6e4a3bb6fab9f7e63f0988ef9ed";
byte   g_measurement[SHA1DIGESTBYTESIZE] = {0};


int g_publicKeyBlockSize= 256;


bool initTPM(const char* aikblobfile, const char* szTPMPassword)
{
	bool pValid;
	int size = 1024;
	byte szxml[4096];
	byte* pdata = &szxml[0];
	taoFiles xmlkey;
	
	fprintf(stdout, "Called %s\n", __FUNCTION__);

	xmlkey.getBlobData("/tmp/rptmp/config/HWRoot/p2048tpm.xml", &pValid, &size, &pdata);
	
	g_rsa.m_pDoc= new TiXmlDocument();
	
    if(g_rsa.m_pDoc==NULL) {
         fprintf(stdout, "Cant init %s key document\n", "");
         return false;
    }

    if(!g_rsa.ParsefromString((char*)pdata)) {
         fprintf(stdout, "Cant parse %s key\n%s\n", "", (char*)pdata);
         return false;
    }

    if(!g_rsa.getDataFromDoc()) {
         fprintf(stdout, "Cant get data from %s key document\n", "");
         return false;
    }
	
	memset(g_aes.m_rgbKey, 0x1, 32);
	
	g_aes.m_iByteSizeKey = 32;
	g_aes.m_iByteSizeIV = 0;
	
    return true;
}


bool deinitTPM()
{
    return true;

}


bool getAttestCertificateTPM(int size, byte* pKey)
{
    return false;
}


bool getEntropyTPM(int size, byte* pKey)
{
    return false;
}

bool getPcrSelectionTPM(int *rgiPCRs, int *pcrNum)
{
	return false;
}

bool getMeasurementTPM(int* pSize, byte* pHash)
// return TPM1.2 composite hash
{
    u32     size= SHA1DIGESTBYTESIZE*24; 
    //byte    pcrs[SHA1DIGESTBYTESIZE*24];
    byte    pcr17Mask[3]= {0,0,0x02};

#ifndef QUOTE2_DEFINED
    // reconstruct PCR composite and composite hash
    if(!computeTPM12compositepcrDigest(pcr17Mask, g_pcrs, pHash)) {
        fprintf(g_logFile, "getMeasurementTPM: can't compute composite digest\n");
        return false;
    }
#else
  
#endif

	memcpy(g_measurement, pHash, SHA1DIGESTBYTESIZE);
    *pSize= SHA1DIGESTBYTESIZE;
    return true;
}


bool sealwithTPM(int sizetoSeal, byte* toSeal, int* psizeSealed, byte* sealed)
{
#ifdef TEST
    fprintf(g_logFile, "sealwithTPM\n");
#endif
#if 0
    return g_oTpm.sealData(inSize, inData, (unsigned*) poutSize, outData);
#endif

   byte    tmpout[4096];
   int hostedMeasurementSize = SHA1DIGESTBYTESIZE;
   byte *hostedMeasurement = &g_measurement[0];

    if((2*sizeof(int)+sizetoSeal+hostedMeasurementSize)>4096) {
        fprintf(g_logFile, "taoEnvironment::Seal, buffer too small\n");
        return false;
    }

    // tmpout is hashsize||hash||sealdatasize||sealeddata
    int     n= 0;
    memcpy(&tmpout[n], &hostedMeasurementSize, sizeof(int));
    n+= sizeof(int);
    memcpy(&tmpout[n], hostedMeasurement, hostedMeasurementSize);
    n+= hostedMeasurementSize;
    memcpy(&tmpout[n], &sizetoSeal, sizeof(int));
    n+= sizeof(int);
    memcpy(&tmpout[n], toSeal, sizetoSeal);
    n+= sizetoSeal;

#ifdef TEST1
    fprintf(g_logFile, "taoEnvironment::Seal, about to encryptBlob, n=%d\n",
            n);
    PrintBytes("To Seal: ", tmpout, n);
    fflush(g_logFile);
#endif
    if(!AES128CBCHMACSHA256SYMPADEncryptBlob(n, tmpout, psizeSealed, sealed,
                                    g_aes.m_rgbKey, &g_aes.m_rgbKey[AES128BYTEBLOCKSIZE])) {
        fprintf(g_logFile, "taoEnvironment::seal: AES128CBCHMACSHA256SYMPADEncryptBlob failed\n");
        return false;
    }
	return true;
}


bool unsealwithTPM(int sizeSealed, byte* sealed, int *psizeunsealed, byte* unsealed)
{
#if 0
    return g_oTpm.unsealData(inSize, inData, (unsigned*) poutSize, outData);
#endif

 int     n= 0;
    int     outsize= 4096;
    byte    tmpout[4096];
    int     hashsize= 20;
   int hostedMeasurementSize = SHA1DIGESTBYTESIZE;
   byte *hostedMeasurement = &g_measurement[0];

    if(!AES128CBCHMACSHA256SYMPADDecryptBlob(sizeSealed, sealed, &outsize, tmpout, 
                        g_aes.m_rgbKey, &g_aes.m_rgbKey[AES128BYTEBLOCKSIZE])) {
        fprintf(g_logFile, 
           "taoEnvironment::unseal: AES128CBCHMACSHA256SYMPADDecryptBlob failed\n");
        return false;
    }

    // tmpout is hashsize||hash||sealdatasize||sealeddata
    memcpy(&hashsize, &tmpout[n], sizeof(int));
    n+= sizeof(int);

    if(hashsize!=hostedMeasurementSize)
        return false;
     
    // Fix: the following line was the old value, 
    //  but I think it should be memcpy and is just a typo
    //memcmp(&tmpout[n], hostedMeasurement, hashsize);
    memcpy(&tmpout[n], hostedMeasurement, hashsize);
    n+= hashsize;

    memcpy(psizeunsealed, &tmpout[n], sizeof(int));
    n+= sizeof(int);
    memcpy(unsealed, &tmpout[n], *psizeunsealed);
    return true;
}


bool quotewithTPM(int sizetoAttest, byte* toAttest, int* psizeAttested, byte* attest, byte* plocality)
{
    byte    newout[1024];
    
   int hostedMeasurementSize = SHA1DIGESTBYTESIZE;
   byte *hostedMeasurement = &g_measurement[0];

    if(*psizeAttested < g_publicKeyBlockSize)
        return false;

    byte        rgQuotedHash[SHA256DIGESTBYTESIZE];
    byte        rgToSign[512];
	
	    
	byte    locality= 0; 
	u32     sizeversion= 0;
	byte*   versionInfo= NULL;
	byte    pcr17Mask[3]= {0,0,0x02};

   // getMeasurementTPM(&sizehashCode, hashCode);
    
		// construct PCR composite and composite hash
	if(!tpm12quote2Hash(0, NULL, pcr17Mask, locality,
						sizetoAttest, toAttest, SHA1DIGESTBYTESIZE, g_measurement, 
						false, sizeversion, versionInfo, 
						rgQuotedHash)) {
		fprintf(g_logFile, "checkXMLQuote: Cant compute TPM12 hash\n");
		return false;
	}
    
    fprintf(g_logFile, "checkXMLQuote hashtype: 2\n");
    PrintBytes("Code digest: ", g_measurement, SHA1DIGESTBYTESIZE);
    PrintBytes("Body hash: ", toAttest, sizetoAttest);
    PrintBytes("final hash: ", rgQuotedHash, SHA1DIGESTBYTESIZE);
    //PrintBytes("quotevalue: ", quoteValue, outLen);
    fflush(g_logFile);  
    
    
    // pad
    if(!emsapkcspad(SHA1HASH, rgQuotedHash, g_publicKeyBlockSize, rgToSign)) {
        fprintf(g_logFile, "taoEnvironment::Attest: emsapkcspad returned false\n");
        return false;
    }
    // sign
    
#if 1 //ONERSA
	RSAKey* pRSA= (RSAKey*) &g_rsa;
	if (! RSADecrypt_i(*pRSA, g_publicKeyBlockSize, rgToSign, psizeAttested, attest)){
		fprintf(g_logFile, "taoEnvironment::Attest: mpRSAENC returned false\n");
        return false;
	}

#else
    RSAKey* pRSA= (RSAKey*) &g_rsa;
    bnum    bnMsg(g_publicKeyBlockSize/sizeof(u64));
    bnum    bnOut(g_publicKeyBlockSize/sizeof(u64));
    memset(bnMsg.m_pValue, 0, g_publicKeyBlockSize);
    memset(bnOut.m_pValue, 0, g_publicKeyBlockSize);
    revmemcpy((byte*)bnMsg.m_pValue, rgToSign, g_publicKeyBlockSize);
#ifdef TEST1
    fprintf(g_logFile, "taoEnvironment::Attest about to RSAENC\n");
    fflush(g_logFile);
#endif
   if(!mpRSAENC(bnMsg, *(pRSA->m_pbnD), *(pRSA->m_pbnM), bnOut)) {
        fprintf(g_logFile, "taoEnvironment::Attest: mpRSAENC returned false\n");
        return false;
    }

#ifdef TEST1
    fprintf(g_logFile, "taoEnvironment::Attest succeeded, m_publicKeyBlockSize: %d\n",
            m_publicKeyBlockSize);
    fflush(g_logFile);
#endif
    memcpy(attest, bnOut.m_pValue, g_publicKeyBlockSize);
    
#endif
	*plocality = locality;
    *psizeAttested= g_publicKeyBlockSize;
    return true;
}


// -------------------------------------------------------------------------


#endif


// -------------------------------------------------------------------------


