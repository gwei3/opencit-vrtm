//
//  File: rpapi.cpp
//        
//  Description: rp crypto api wrappers
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



#include "jlmTypes.h"

#include "rpapilocalext.h"
#include "rpapilocal.h"

#include "tinyxml.h"
#include "bignum.h"
#include "logging.h"

#include "mpFunctions.h"
#include "jlmcrypto.h"
//#include "hashprep.h"
#include "encapsulate.h"
#include "sha1.h"
#include "cryptoHelper.h"
// For seal, unseal and attest
#include "tciohdr.h"
#include "tcIO.h"
#include "rp_api_code.h"
#include "channelcoding.h"
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <time.h>
#include <unistd.h> 

#ifndef INT32_MAX
#define INT32_MAX	(2147483647)
#endif

int doTrustOp(int opcode, unsigned int* pType, int inSize, byte* inData, int* poutSize, byte* outData);
bool GenQuote(int sizenonce, byte* nonce, char* publicKeyXML, char** ppQuoteXML);
bool VerifyQuote(char* szQuoteData, char* szRoot);
// --------------------------------------------------------------------- 

void InitBigNum()
{
	initBigNum();
}

bool sameBigNum(int size, bnum& bnA, const char* szBase64A)  
//size in # bytes
{
    int iOutLen= 512;
    bnum bnB(64);

    if(!fromBase64(strlen(szBase64A), szBase64A, &iOutLen, (u8*)bnB.m_pValue)) {
        return false;
    }

#ifdef OLD
    if(mpCompare(bnA, bnB)!=s_iIsEqualTo) {
#else
    if(mpCompare(bnA, bnB)!=s_isEqualTo) {
#endif
        return false;
        }
    return true;
}

// -------------------------------------------------------------------------


int ToBase64_WithDir(int inLen, const byte* pbIn, int* poutLen, char* szOut, int iDirFwd)
{
 bool fRet;
 bool fDirFwd;
 if(iDirFwd==1) fDirFwd=true;
 if(iDirFwd==0) fDirFwd=false;
 if((iDirFwd!=0) && (iDirFwd!=1)) return false;
 if((inLen==0) || (pbIn==NULL)) return false;
 if((poutLen==NULL) || (szOut==NULL)) return false;
 if(*poutLen==0) return false;
 fRet= toBase64(inLen, pbIn, poutLen,  szOut /*, fDirFwd*/);
 return fRet;

}

int ToBase64(int inLen, const byte* pbIn, int* poutLen, char* szOut)
{
 bool fRet;
 if((inLen==0) || (pbIn==NULL)) return false;
 if((poutLen==NULL) || (szOut==NULL)) return false;
 if(*poutLen==0) return false;
 fRet= toBase64(inLen, pbIn, poutLen,  szOut);
 return fRet;

}
// -------------------------------------------------------------------------

int FromBase64_WithDir(int inLen, const char* szIn, int* poutLen, unsigned char* puOut, int iDirFwd)
{
 bool fRet;
 bool fDirFwd;
 if(iDirFwd==1) fDirFwd=true;
 if(iDirFwd==0) fDirFwd=false;
 if((iDirFwd!=0) && (iDirFwd!=1)) return false;
 if((inLen==0) || (szIn==NULL)) return false;
 if((poutLen==NULL) || (puOut==NULL)) return false;
 if(*poutLen==0) return false;
 fRet= fromBase64(inLen, szIn, poutLen,  puOut/*,  fDirFwd*/);
 return fRet;

}

int FromBase64(int inLen, const char* szIn, int* poutLen, unsigned char* puOut)
{
 bool fRet;
 if((inLen==0) || (szIn==NULL)) return false;
 if((poutLen==NULL) || (puOut==NULL)) return false;
 if(*poutLen==0) return false;
 fRet= fromBase64(inLen, szIn, poutLen,  puOut);
 return fRet;

}
// -------------------------------------------------------------------------

int GetBase64Rand(int iBytes, byte* puR, int* pOutSize, char* szOut)

{

 int fRet;

 if(iBytes==0) return false;
 if((puR==NULL) || (szOut==NULL) || pOutSize==NULL) return false;
 if(*pOutSize<=0) return false;
 fRet= getBase64Rand(iBytes, puR,  pOutSize, szOut); 
 return fRet;
}

// -------------------------------------------------------------------------

bool ReadAESKeyfromFile(const char* szKeyFile, SymKey *symKeyOut)
{

    KeyInfo*    pParseKey= new KeyInfo;
    symKey*     pAESKey= NULL;
    int         iKeyType;
    bool         fRet= false;

    TiXmlDocument* pDoc= new TiXmlDocument();  
    if(pDoc==NULL) {
        return false;
    }

    if(!pDoc->LoadFile(szKeyFile)) {
        return false;
    }
    iKeyType= pParseKey->getKeyType(pDoc);

    switch(iKeyType) {
      case AESKEYTYPE:
        pAESKey= new symKey();
        if(pAESKey==NULL) {
            break;
        }
        else
            fRet= true;
            pAESKey->m_pDoc= pDoc;
        pAESKey->getDataFromDoc();
        break;
      default:
       break;
    }

    delete pParseKey;
    // Dont forget to delete pDoc;
    if (fRet == false) return false;

#ifdef TEST_D
	fprintf(g_logFile, "Invoking printMe() \n");
#ifdef TEST
        pAESKey->printMe();
#endif
	fprintf(g_logFile, "End Invoking printMe() \n");
	fprintf(g_logFile, "pAESKey->m_iByteSizeKey= %d\n",pAESKey->m_iByteSizeKey);
	fprintf(g_logFile, "pAESKey->m_iByteSizeIV= %d\n",pAESKey->m_iByteSizeIV);

	//PrintBytes("m_rgbKey[SMALLKEYSIZE]",(u8 *)pAESKey->m_rgbKey,SMALLKEYSIZE);
	PrintBytes("m_rgbKey[SMALLKEYSIZE]",(u8 *)pAESKey->m_rgbKey,pAESKey->m_ikeySize/8);
	PrintBytes("m_rgbIV[SMALLKEYSIZE]",pAESKey->m_rgbIV,pAESKey->m_ikeySize/8);
	fprintf(g_logFile, "pAESKey->m_ukeyType= %d\n",pAESKey->m_ukeyType);
	fprintf(g_logFile, "pAESKey->m_uAlgorithm= %d\n",pAESKey->m_uAlgorithm);
	fprintf(g_logFile, "pAESKey->m_ikeySize= %d\n",pAESKey->m_ikeySize);
	fprintf(g_logFile, "pAESKey->m_ikeyNameSize= %d\n",pAESKey->m_ikeyNameSize);
	PrintBytes("m_rgkeyName[KEYNAMEBUFSIZE]",(u8 *)pAESKey->m_rgkeyName,pAESKey->m_ikeyNameSize);
	fprintf(g_logFile, "SMALLKEYSIZE %d\n",SMALLKEYSIZE);
#endif
        if(symKeyOut->len < (unsigned int)(pAESKey->m_ikeySize/8)) {
            delete pDoc;
            pAESKey->m_pDoc= 0;
            delete pAESKey;
	    return false;
	} else {
	    symKeyOut->len= pAESKey->m_ikeySize/8; 
	    memcpy(symKeyOut->data,pAESKey->m_rgbKey,pAESKey->m_ikeySize/8);
            delete pDoc;
            pAESKey->m_pDoc= 0;
            delete pAESKey;
            return true; 
	}

}

// -------------------------------------------------------------------------

bool ReadRSAKeyfromFile(const char* szKeyFile, RSAKeyPair *rsaKeyPairOut)
{
    KeyInfo*    pParseKey= new KeyInfo;
    RSAKey*     pRSAKey= NULL;
    int         iKeyType;
    bool        fRet= false;

    TiXmlDocument* pDoc= new TiXmlDocument();
    if(pDoc==NULL) {
        return false;
    }

    if(!pDoc->LoadFile(szKeyFile)) {
        return false;
    }
    iKeyType= pParseKey->getKeyType(pDoc);

    switch(iKeyType) {
      case RSAKEYTYPE:
        pRSAKey= new RSAKey();
        if(pRSAKey==NULL) {
            break;
        }
        else
            fRet= true;
            pRSAKey->m_pDoc= pDoc;
        pRSAKey->getDataFromDoc();
        break;
      default:
       break;
    }
    delete pParseKey;

    if (fRet == false) return false;
    

    
#ifdef TEST_DD 
	fprintf(g_logFile, "Invoking printMe() \n");
#ifdef TEST
    	pRSAKey->printMe();
#endif
	fprintf(g_logFile, "End Invoking printMe() \n");
	fprintf(g_logFile, "pRSAKey->m_iByteSizeM= %d\n",pRSAKey->m_iByteSizeM);
	fprintf(g_logFile, "pRSAKey->m_iByteSizeD= %d\n",pRSAKey->m_iByteSizeD);
	fprintf(g_logFile, "pRSAKey->m_iByteSizeE= %d\n",pRSAKey->m_iByteSizeE);
	fprintf(g_logFile, "pRSAKey->m_iByteSizeP= %d\n",pRSAKey->m_iByteSizeP);
	fprintf(g_logFile, "pRSAKey->m_iByteSizeQ= %d\n",pRSAKey->m_iByteSizeQ);
	fprintf(g_logFile, "BIGKEYSIZE= %d\n",BIGKEYSIZE);

	//fprintf(g_logFile, "pRSAKey->m_iByteSizeIV= %d\n",pRSAKey->m_iByteSizeIV);

	//PrintBytes("m_rgbM[BIGKEYSIZE]",(u8 *)pRSAKey->m_rgbM,BIGKEYSIZE);
	PrintBytes("m_rgbM[BIGKEYSIZE/M]",(u8 *)pRSAKey->m_rgbM,pRSAKey->m_iByteSizeM);
	PrintBytes("m_rgbP[BIGKEYSIZE/P]",(u8 *)pRSAKey->m_rgbP,pRSAKey->m_iByteSizeP);
	PrintBytes("m_rgbQ[BIGKEYSIZE/Q]",(u8 *)pRSAKey->m_rgbQ,pRSAKey->m_iByteSizeQ);
	PrintBytes("m_rgbE[BIGKEYSIZE/E]",(u8 *)pRSAKey->m_rgbE,pRSAKey->m_iByteSizeE);
	PrintBytes("m_rgbD[BIGKEYSIZE/D]",(u8 *)pRSAKey->m_rgbD,pRSAKey->m_iByteSizeD);
	//PrintBytes("m_rgbIV[SMALLKEYSIZE]",pRSAKey->m_rgbIV,SMALLKEYSIZE);


	fprintf(g_logFile, "pRSAKey->m_pbnM->m_signandSize= %x\n",pRSAKey->m_pbnM->m_signandSize);
	fprintf(g_logFile, "pRSAKey->m_pbnM->m_pValue= %x\n",pRSAKey->m_pbnM->m_pValue);
	fprintf(g_logFile, "pRSAKey->m_pbnM->mpSize()= %d\n",pRSAKey->m_pbnM->mpSize());
	if (pRSAKey->m_pbnP ) {
		fprintf(g_logFile, "pRSAKey->m_pbnP->m_signandSize= %x\n",pRSAKey->m_pbnP->m_signandSize);
		fprintf(g_logFile, "pRSAKey->m_pbnP->m_pValue= %x\n",pRSAKey->m_pbnP->m_pValue);
		fprintf(g_logFile, "pRSAKey->m_pbnP->mpSize()= %d\n",pRSAKey->m_pbnP->mpSize());
	}
	
	if (pRSAKey->m_pbnQ) {
		fprintf(g_logFile, "pRSAKey->m_pbnQ->m_signandSize= %x\n",pRSAKey->m_pbnQ->m_signandSize);
		fprintf(g_logFile, "pRSAKey->m_pbnQ->m_pValue= %x\n",pRSAKey->m_pbnQ->m_pValue);
		fprintf(g_logFile, "pRSAKey->m_pbnQ->mpSize()= %d\n",pRSAKey->m_pbnQ->mpSize());
	}
	
	if (pRSAKey->m_pbnE) {
		fprintf(g_logFile, "pRSAKey->m_pbnE->m_signandSize= %x\n",pRSAKey->m_pbnE->m_signandSize);
		fprintf(g_logFile, "pRSAKey->m_pbnE->m_pValue= %x\n",pRSAKey->m_pbnE->m_pValue);
		fprintf(g_logFile, "pRSAKey->m_pbnE->mpSize()= %d\n",pRSAKey->m_pbnE->mpSize());
	}
	
	if ( pRSAKey->m_pbnD ) {
		fprintf(g_logFile, "pRSAKey->m_pbnD->m_signandSize= %x\n",pRSAKey->m_pbnD->m_signandSize);
		fprintf(g_logFile, "pRSAKey->m_pbnD->m_pValue= %x\n",pRSAKey->m_pbnD->m_pValue);
		fprintf(g_logFile, "pRSAKey->m_pbnD->mpSize()= %d\n",pRSAKey->m_pbnD->mpSize());
	}
	
    if (pRSAKey->m_pbnP ) PrintBytes("P:", (u8*)pRSAKey->m_pbnP->m_pValue, pRSAKey->m_iByteSizeP);
    if (pRSAKey->m_pbnQ ) PrintBytes("Q:", (u8*)pRSAKey->m_pbnQ->m_pValue, pRSAKey->m_iByteSizeQ);
    if (pRSAKey->m_pbnM) PrintBytes("M:", (u8*)pRSAKey->m_pbnM->m_pValue, pRSAKey->m_iByteSizeM);
    if (pRSAKey->m_pbnE) PrintBytes("E:", (u8*)pRSAKey->m_pbnE->m_pValue, pRSAKey->m_iByteSizeE);
    if (pRSAKey->m_pbnD) PrintBytes("D:", (u8*)pRSAKey->m_pbnD->m_pValue, pRSAKey->m_iByteSizeD);

    fprintf(g_logFile, "pRSAKey->m_ukeyType= %d\n",pRSAKey->m_ukeyType);
    fprintf(g_logFile, "pRSAKey->m_uAlgorithm= %d\n",pRSAKey->m_uAlgorithm);
    fprintf(g_logFile, "pRSAKey->m_ikeySize= %d\n",pRSAKey->m_ikeySize);
    fprintf(g_logFile, "pRSAKey->m_ikeyNameSize= %d\n",pRSAKey->m_ikeyNameSize);
    PrintBytes("m_rgkeyName[SMALLKEYSIZE]",(u8 *)pRSAKey->m_rgkeyName,KEYNAMEBUFSIZE);
#endif

    if((rsaKeyPairOut->n.len < (u32)pRSAKey->m_iByteSizeM)
     || (rsaKeyPairOut->e.len < (u32)pRSAKey->m_iByteSizeE)
     || (rsaKeyPairOut->d.len < (u32)pRSAKey->m_iByteSizeD) 
     || (rsaKeyPairOut->p.len < (u32)pRSAKey->m_iByteSizeP)
     || (rsaKeyPairOut->q.len < (u32)pRSAKey->m_iByteSizeQ))  {
        delete pDoc;
        pRSAKey->m_pDoc= 0;
        delete pRSAKey;
        return false;

     } else {

        rsaKeyPairOut->n.len= pRSAKey->m_iByteSizeM;
        memcpy(rsaKeyPairOut->n.data,(u8 *)pRSAKey->m_pbnM->m_pValue, pRSAKey->m_iByteSizeM);

        rsaKeyPairOut->e.len= pRSAKey->m_iByteSizeE;
        memcpy(rsaKeyPairOut->e.data,pRSAKey->m_rgbE, pRSAKey->m_iByteSizeE);

        rsaKeyPairOut->d.len= pRSAKey->m_iByteSizeD;
        memcpy(rsaKeyPairOut->d.data,pRSAKey->m_rgbD, pRSAKey->m_iByteSizeD);


        rsaKeyPairOut->p.len= pRSAKey->m_iByteSizeP;
        memcpy(rsaKeyPairOut->p.data,pRSAKey->m_rgbP,pRSAKey->m_iByteSizeP);

        rsaKeyPairOut->q.len= pRSAKey->m_iByteSizeQ;
        memcpy(rsaKeyPairOut->q.data,pRSAKey->m_rgbQ,pRSAKey->m_iByteSizeQ);

        delete pDoc;
        pRSAKey->m_pDoc= 0;
        delete pRSAKey;
        return true;
    }
   
}

// -------------------------------------------------------------------------

KeyInfo* ReadKeyfromFile(const char* szKeyFile)
{
    KeyInfo*    pParseKey= new KeyInfo;
    RSAKey*     pRSAKey= NULL;
    symKey*     pAESKey= NULL;
    KeyInfo*    pRetKey= NULL;
    int         iKeyType;

    TiXmlDocument* pDoc= new TiXmlDocument();
    if(pDoc==NULL) {
        return NULL;
    }

    if(!pDoc->LoadFile(szKeyFile)) {
        return NULL;
    }
    iKeyType= pParseKey->getKeyType(pDoc);

    switch(iKeyType) {
      case AESKEYTYPE:
        pAESKey= new symKey();
        if(pAESKey==NULL) {
            break;
        }
        else
            pAESKey->m_pDoc= pDoc;
        pAESKey->getDataFromDoc();
        pRetKey= (KeyInfo*) pAESKey;
        break;
      case RSAKEYTYPE:
        pRSAKey= new RSAKey();
        if(pRSAKey==NULL) {
            break;
        }
        else
            pRSAKey->m_pDoc= pDoc;
        pRSAKey->getDataFromDoc();
        pRetKey= (KeyInfo*) pRSAKey;
        break;
      default:
       break;
    }
    delete pParseKey;
    // Dont forget to delete pDoc;

    return pRetKey;
}

// -------------------------------------------------------------------------

int ValidateRSAKeyPair( RSAKeyPair* rsaKeyOut)
{

    int iRet;
    RSAKeyPair rsaKeys;

    u8 bufp[256];
    u8 bufq[256];
    u8 bufn[256];
    u8 bufe[256];
    u8 bufd[256];

    rsaKeys.n.data= bufn;
    rsaKeys.e.data= bufe;
    rsaKeys.d.data= bufd;
    rsaKeys.p.data= bufp;
    rsaKeys.q.data= bufq;
    rsaKeys.n.len= rsaKeyOut->n.len;
    rsaKeys.e.len= rsaKeyOut->e.len;
    rsaKeys.d.len= rsaKeyOut->d.len;
    rsaKeys.p.len= rsaKeyOut->p.len;
    rsaKeys.q.len= rsaKeyOut->q.len;
    memcpy(bufn, rsaKeyOut->n.data, rsaKeyOut->n.len);
    memcpy(bufe, rsaKeyOut->e.data, rsaKeyOut->e.len);
    memcpy(bufd, rsaKeyOut->d.data, rsaKeyOut->d.len);
    memcpy(bufp, rsaKeyOut->p.data, rsaKeyOut->p.len);
    memcpy(bufq, rsaKeyOut->q.data, rsaKeyOut->q.len);


    RSAPublicKey pubKey;
    RSAPrivateKey priKey;
    u8  n[256];
    u8  e[256];
    pubKey.n.len=256;
    pubKey.n.data=n;

    pubKey.e.len=256;
    pubKey.e.data= e;

    u8  d[256];
    u8  nn[256];
    u8  p[256];
    u8  q[256];

    priKey.d.len=256;
    priKey.d.data= d;
    priKey.n.len=256;
    priKey.n.data=nn;
    priKey.p.len=256;
    priKey.p.data=p;
    priKey.q.len=256;
    priKey.q.data=q;

    char szIn[]="t0723";
    const char* szRSAPad="RSA_NO_PADDING";
    byte szEn[2048];
    Data_buffer dataIn;
    Data_buffer Encrypted;
    dataIn.data= (byte *)szIn;
    dataIn.len= strlen(szIn);
    Encrypted.data= szEn;
    Encrypted.len= 2048;

    pubKey.n.len= rsaKeys.n.len;
    memcpy(pubKey.n.data,rsaKeys.n.data, rsaKeys.n.len);
    pubKey.e.len= rsaKeys.e.len;
    memcpy(pubKey.e.data,rsaKeys.e.data, rsaKeys.e.len);

    iRet= RSAEncrypt(szRSAPad, &dataIn, &Encrypted, &pubKey);
    if (iRet !=1 ) {
#ifdef TEST
        fprintf(g_logFile, "RSAEncrypt iRet= %d\n", iRet);
	fprintf(g_logFile, "Failed in RSAEncrypt\n");
#endif
        return 0;

    }

    //Decrypt
    byte szDec[2048];
    Data_buffer Decrypted;

    Decrypted.len= 2048;
    Decrypted.data= szDec;
    priKey.n.len= rsaKeys.n.len;
    memcpy(priKey.n.data,rsaKeys.n.data, rsaKeys.n.len);
    priKey.d.len= rsaKeys.d.len;
    memcpy(priKey.d.data,rsaKeys.d.data, rsaKeys.d.len);
    priKey.p.len= rsaKeys.p.len;
    memcpy(priKey.p.data,rsaKeys.p.data, rsaKeys.p.len);
    priKey.q.len= rsaKeys.q.len;
    memcpy(priKey.q.data,rsaKeys.q.data, rsaKeys.q.len);
    iRet= RSADecrypt(szRSAPad, &Encrypted, &Decrypted, &priKey);
    if (iRet !=1 ) {
#ifdef TEST
        fprintf(g_logFile, "RSADecrypt iRet= %d\n", iRet);
	fprintf(g_logFile, "Failed in RSADecrypt\n");
#endif
        return 0;
    }

   
    if (memcmp(dataIn.data, Decrypted.data, dataIn.len) != 0) {
#ifdef TEST
	fprintf(g_logFile, "Failed in Enc/Decr\n");
#endif
	return 0;
    }

    return 1;
}

int GenRSAKey(const char* szKeyType, const char* reserved, u64 pubExp, RSAKeyPair* keyOut)
{


    int             iTry= 0;
    bool            fGotKey;
    unsigned int    size= 0;
    int iRet= 0;

    if(szKeyType==NULL) {
        return false;
    }

  // These are for test only - RSA128, 256, 512 
#ifdef TEST_DD
    if(strcmp(szKeyType, "RSA128")==0) {
	size= 128;
    }
#endif
#ifdef TEST_D
    if(strcmp(szKeyType, "RSA256")==0) {
	size= 256;
    }
    if(strcmp(szKeyType, "RSA512")==0) {
	size= 512;
    }
#endif
    if(strcmp(szKeyType, "RSA1024")==0) {
	size= 1024;
    }
    if(strcmp(szKeyType, "RSA2048")==0) {
	size= 2048;
    }

    if (size == 0) return false;

    if(keyOut==NULL) {
        return false;
    }
    if(keyOut->p.data==NULL) {
        return false;
    }
    if(keyOut->q.data==NULL) {
        return false;
    }
    if(keyOut->n.data==NULL) {
        return false;
    }
    if(keyOut->e.data==NULL) {
        return false;
    }
    if(keyOut->d.data==NULL) {
        return false;
    }



    int rounds= 0;
    int validKeyPair= 0;
    while (rounds < 4) {
	rounds++;
    	RSAKey* pKey= RSAGenerateKeyPair(size);
    	if (!pKey )  {
#ifdef TEST
		fprintf(g_logFile, "Failed in RSAGenerateKeyPair\n");
#endif
		return false;
	}
        else {
    		memcpy(keyOut->p.data,(u8*)pKey->m_pbnP->m_pValue, pKey->m_iByteSizeP);	
    		keyOut->p.len = pKey->m_iByteSizeP;

    		memcpy(keyOut->q.data,(u8*)pKey->m_pbnQ->m_pValue, pKey->m_iByteSizeQ);	
    		keyOut->q.len = pKey->m_iByteSizeQ;

    		memcpy(keyOut->n.data,(u8*)pKey->m_pbnM->m_pValue, pKey->m_iByteSizeM);	
    		keyOut->n.len = pKey->m_iByteSizeM;

    		memcpy(keyOut->e.data,(u8*)pKey->m_pbnE->m_pValue, pKey->m_iByteSizeE);	
    		keyOut->e.len = pKey->m_iByteSizeE;

    		memcpy(keyOut->d.data,(u8*)pKey->m_pbnD->m_pValue, pKey->m_iByteSizeD);	
    		keyOut->d.len = pKey->m_iByteSizeD;

	}

        iRet= ValidateRSAKeyPair(keyOut);
        if(iRet == 1) {
    		validKeyPair= 1;
		break;
	}
    }

    if(validKeyPair == 0) {
#ifdef TEST_D
	fprintf(g_logFile, "Failed because all tries failed\n");
#endif
	return false;	
    }	


    return true;
}

//#define MAXTRY  20
#define MAXTRY 60


int GenRSAKeyFile(int size, const char*szOutFile)
{
    TiXmlDocument   doc;
    TiXmlNode*      pNode;
    TiXmlNode*      pNode1;
    TiXmlNode*      pNode2;
    extern char*    szRSAKeyProto;
    int             iOutLen= 1024;
    char            szBase64KeyP[1024];
    char            szBase64KeyQ[1024];
    char            szBase64KeyM[1024];
    char            szBase64KeyE[1024];
    char            szBase64KeyD[1024];
    byte            tbuf[512];

    char* type = "RSA1024";
	// Is this in or Out
    RSAKeyPair rsaKeyOut;
    rsaKeyOut.n.len=0;
    rsaKeyOut.e.len=0;
    rsaKeyOut.d.len=0;
    rsaKeyOut.p.len=0;
    rsaKeyOut.q.len=0;

    u8 bufp[256];
    u8 bufq[256];
    u8 bufn[256];
    u8 bufe[256];
    u8 bufd[256];

    rsaKeyOut.n.data=bufn;
    rsaKeyOut.e.data=bufe;
    rsaKeyOut.d.data=bufd;
    rsaKeyOut.p.data=bufp;
    rsaKeyOut.q.data=bufq;

    rsaKeyOut.n.len=256;
    rsaKeyOut.e.len=256;
    rsaKeyOut.d.len=256;
    rsaKeyOut.p.len=256;
    rsaKeyOut.q.len=256;
    
	if (size == 2048)
		type = "RSA2048";
		
	if (! GenRSAKey(type, NULL, 0, &rsaKeyOut))
		return false;
 
    iOutLen= 1024;
    revmemcpy(tbuf,(u8*)rsaKeyOut.p.data, rsaKeyOut.p.len);
    if(!toBase64(rsaKeyOut.p.len, tbuf, &iOutLen, szBase64KeyP)) {
        return false;
    }
    iOutLen= 1024;
    revmemcpy(tbuf,(u8*)rsaKeyOut.q.data, rsaKeyOut.q.len);
    if(!toBase64(rsaKeyOut.q.len, tbuf, &iOutLen, szBase64KeyQ)) {
        return false;
    }
    iOutLen= 1024;
    revmemcpy(tbuf,(u8*)rsaKeyOut.n.data, rsaKeyOut.n.len);
    if(!toBase64(rsaKeyOut.n.len, tbuf, &iOutLen, szBase64KeyM)) {
        return false;
    }
    iOutLen= 1024;
    revmemcpy(tbuf,(u8*)rsaKeyOut.e.data, rsaKeyOut.e.len);
    if(!toBase64(rsaKeyOut.e.len, tbuf, &iOutLen, szBase64KeyE)) {
        return false;
    }
    iOutLen= 1024;
    revmemcpy(tbuf,(u8*)rsaKeyOut.d.data, rsaKeyOut.d.len);
    if(!toBase64(rsaKeyOut.d.len, tbuf, &iOutLen, szBase64KeyD)) {
        return false;
    }

 


    if(!doc.Parse(szRSAKeyProto)) {
        return false;
    }

    TiXmlElement* pRootElement= doc.RootElement();
    TiXmlText* pNewText;
    if(strcmp(pRootElement->Value(),"ds:KeyInfo")==0) {
        pRootElement->SetAttribute("KeyName","KEYNAME");
    }
    
    pNode= pRootElement->FirstChild();
    while(pNode) {
        if(pNode->Type()==TiXmlNode::TINYXML_ELEMENT) {
            if(strcmp(((TiXmlElement*)pNode)->Value(),"KeyType")==0) {
                    pNewText= new TiXmlText("RSAKeyType");
                    pNode->InsertEndChild(*pNewText);
            }
            if(strcmp(((TiXmlElement*)pNode)->Value(),"ds:KeyValue")==0) {
                pNode1= pNode->FirstChild();
                while(pNode1) {
                    if(strcmp(((TiXmlElement*)pNode1)->Value(),"ds:RSAKeyValue")==0) {
                        ((TiXmlElement*) pNode1)->SetAttribute ("size", size);
                        pNode2= pNode1->FirstChild();
                        while(pNode2) {
                            if(strcmp(((TiXmlElement*)pNode2)->Value(),"ds:P")==0) {
                                TiXmlText* pNewTextP= new TiXmlText(szBase64KeyP);
                                pNode2->InsertEndChild(*pNewTextP);
                            }
                            if(strcmp(((TiXmlElement*)pNode2)->Value(),"ds:Q")==0) {
                                TiXmlText* pNewTextQ= new TiXmlText(szBase64KeyQ);
                                pNode2->InsertEndChild(*pNewTextQ);
                            }
                            if(strcmp(((TiXmlElement*)pNode2)->Value(),"ds:M")==0) {
                                TiXmlText* pNewTextM= new TiXmlText(szBase64KeyM);
                                pNode2->InsertEndChild(*pNewTextM);
                            }
                            if(strcmp(((TiXmlElement*)pNode2)->Value(),"ds:E")==0) {
                                TiXmlText* pNewTextE= new TiXmlText(szBase64KeyE);
                                pNode2->InsertEndChild(*pNewTextE);
                            }
                            if(strcmp(((TiXmlElement*)pNode2)->Value(),"ds:D")==0) {
                                TiXmlText* pNewTextD= new TiXmlText(szBase64KeyD);
                                pNode2->InsertEndChild(*pNewTextD);
                            }
                            pNode2= pNode2->NextSibling();
                        }
                    }
                    pNode1= pNode1->NextSibling();
                }
            }
        }
        pNode= pNode->NextSibling();
    }

    TiXmlPrinter printer;
    doc.Accept(&printer);
    const char* szDoc= printer.CStr();
    FILE* out= fopen(szOutFile,"w");
    fprintf(out, "%s", szDoc);
    fclose(out);
    return true;
}



bool GenAESKeyFileNoXML(int size, const char* szOutFile)
{
    u8              buf[32];


    if(!getCryptoRandom(size, buf)) {
        return false;
    }
#ifdef TEST_D
    PrintBytes("AES key:", buf, size/8);
#endif

    int  out=  open(szOutFile, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
    if (out < 0) {
    	return false;
     }

    if(write(out, buf, AES128BYTEBLOCKSIZE)<0) {
	    close(out);
         return false;
     }

    close(out);
    return true;
}


bool GenAESKeyFile(int size, const char* szOutFile)
{
    TiXmlDocument   doc;
    TiXmlNode*      pNode;
    TiXmlNode*      pNode1;
    u8              buf[32];
    extern char*    szAESKeyProto;
    int             iOutLen= 128;
    char            szBase64Key[256];

    /*
     *  <ds:KeyInfo xmlns:ds="http://www.w3.org/2000/09/xmldsig#" KeyName=''>
     *  <ds:KeyValue>
     *  <ds:AESKeyValue size=''>
     */


    if(!getCryptoRandom(size, buf)) {
        return false;
    }

#ifdef TEST_D
    PrintBytes("AES key:", buf, size/8);
#endif
    if(!toBase64(size/8, buf, &iOutLen, szBase64Key)) { 
        return false;
    }

#ifdef TEST_D
    fprintf(g_logFile, "Base64 encoded: %s\n",szBase64Key);
#endif

    doc.Parse(szAESKeyProto);

    TiXmlElement* pRootElement= doc.RootElement();
    TiXmlText* pNewText;
    if(strcmp(pRootElement->Value(),"ds:KeyInfo")==0) {
        pRootElement->SetAttribute("KeyName","KEYNAME");
    }
    pNode= pRootElement->FirstChild();
    while(pNode) {
        if(pNode->Type()==TiXmlNode::TINYXML_ELEMENT) {
            if(strcmp(((TiXmlElement*)pNode)->Value(),"KeyType")==0) {
                    pNewText= new TiXmlText("AESKeyType");
                    pNode->InsertEndChild(*pNewText);
            }
            if(strcmp(((TiXmlElement*)pNode)->Value(),"ds:KeyValue")==0) {
                pNode1= pNode->FirstChild();
                while(pNode1) {
                    if(strcmp(((TiXmlElement*)pNode1)->Value(),"ds:AESKeyValue")==0) {
                        ((TiXmlElement*) pNode1)->SetAttribute ("size", size);
                        pNewText= new TiXmlText(szBase64Key);
                        pNode1->InsertEndChild(*pNewText);
                    }
                    pNode1= pNode1->NextSibling();
                }
            }
        }
        pNode= pNode->NextSibling();
    }

    TiXmlPrinter printer;
    doc.Accept(&printer);
    const char* szDoc= printer.CStr();
    FILE* out= fopen(szOutFile,"w");
    fprintf(out, "%s", szDoc);
    fclose(out);
    return true;
}


int GenSymKey(const char* szKeyType, const char* reserved, SymKey* keyOut)
{
    u8              buf[32];

    unsigned int     size= 0;
    if(szKeyType==NULL) {
        return false;
    }

    if(keyOut->data==NULL) {
        return false;
    }

    if(strcmp(szKeyType, "AES128")==0) {
    	if(!getCryptoRandom(AES128BYTEKEYSIZE*8, buf)) {
       		 return false;
	}
	size = AES128BYTEKEYSIZE;
	if ( keyOut->len < size) return false;
	keyOut->len = AES128BYTEKEYSIZE;
	memcpy(keyOut->data,buf,AES128BYTEKEYSIZE);	
#ifdef TEST_D
    	PrintBytes("AES key:", buf, AES128BYTEKEYSIZE);
#endif
       	return true;
    }

    if(strcmp(szKeyType, "AES256")==0) {
    	if(!getCryptoRandom(AES256BYTEKEYSIZE*8, buf)) {
       		 return false;
	}
	size = AES256BYTEKEYSIZE;
	if ( keyOut->len < size) return false;
	keyOut->len = AES256BYTEKEYSIZE;
	memcpy(keyOut->data,buf,AES256BYTEKEYSIZE);	
#ifdef TEST_D
    	PrintBytes("AES key:", buf, AES256BYTEKEYSIZE);
#endif
       	return true;
    } 

	return false;

}

int GenKeyFile(const char* szKeyType, const char* szOutFile)
{
    bool fRet;

    if(szKeyType==NULL)
        return false;
    if(szOutFile==NULL)
        return false;
    if(strcmp(szKeyType, "AES128")==0) {
        return GenAESKeyFile(128, szOutFile);
    }
    if(strcmp(szKeyType, "AES256")==0) {
        return GenAESKeyFile(256, szOutFile);
    }
    // just for test - RSA128, 256, 512
#ifdef TEST_DD
    if(strcmp(szKeyType, "RSA128")==0) {
        return GenRSAKeyFile(128, szOutFile);
    }
#endif

#ifdef TEST_D
    if(strcmp(szKeyType, "RSA256")==0) {
        return GenRSAKeyFile(256, szOutFile);
    }
    if(strcmp(szKeyType, "RSA512")==0) {
        return GenRSAKeyFile(512, szOutFile);
    }
#endif
    if(strcmp(szKeyType, "RSA1024")==0) {
        fRet= GenRSAKeyFile(1024, szOutFile);
        return fRet;
    }
    if(strcmp(szKeyType, "RSA2048")==0) {
        return GenRSAKeyFile(2048, szOutFile);
    }
    return false;
}

#ifdef GCMENABLED

bool AES128GCMEncrypt(int isize, Data_buffer *dataIn, Data_buffer *dataOut, u8* enckey)
{
    gcm     oGCM;
    int     ivSize= AES128BYTEBLOCKSIZE-sizeof(u32);
    int     dataLeft= isize;
    u8      iv[AES128BYTEBLOCKSIZE];
    u8      rgBufIn[4*AES128BYTEBLOCKSIZE];
    u8      rgBufOut[4*AES128BYTEBLOCKSIZE];

    u8     *sCurPtr, *dCurPtr;
    u8     len;
    sCurPtr= dataIn->data;
    dCurPtr= dataOut->data;

#ifdef TEST_D
    fprintf(g_logFile, "AES128GCMEncrypt\n");
#endif
    // init iv
    if(!getCryptoRandom(ivSize*NBITSINBYTE, iv)) {
        return false;
    }
    memset(&iv[ivSize],0, sizeof(u32));

    // init 
    if(!oGCM.initEnc(AES128, ivSize, iv, AES128BYTEKEYSIZE, enckey, isize, 0, AES128BYTEKEYSIZE))
        return false;

    oGCM.firstCipherBlockOut(rgBufOut);
    memcpy(dCurPtr, rgBufOut, AES128BYTEBLOCKSIZE);
    dCurPtr+= AES128BYTEBLOCKSIZE;
    len = AES128BYTEBLOCKSIZE;

    // read, encrypt, and write bytes
    while(dataLeft>AES128BYTEBLOCKSIZE) {
	    memcpy(rgBufIn,sCurPtr,AES128BYTEBLOCKSIZE);
        oGCM.nextPlainBlockIn(rgBufIn, rgBufOut);
        memcpy(dCurPtr, rgBufOut, AES128BYTEBLOCKSIZE);
        dataLeft-= AES128BYTEBLOCKSIZE;
        dCurPtr+= AES128BYTEBLOCKSIZE; 
        sCurPtr+= AES128BYTEBLOCKSIZE; 
        len+= AES128BYTEBLOCKSIZE;
    }

    // final block
    memcpy(rgBufIn,sCurPtr,dataLeft);
    sCurPtr+= dataLeft;
    int n= oGCM.lastPlainBlockIn(dataLeft, rgBufIn, rgBufOut);
    if(n<0)
        return false;
     memcpy(dCurPtr, rgBufOut, n);
     dCurPtr+= n; 
     len+= n;

    // write tag
    oGCM.getTag(oGCM.m_iBlockSize, rgBufOut);
    memcpy(dCurPtr, rgBufOut, oGCM.m_iBlockSize);
    len+= oGCM.m_iBlockSize;

    return true;
}


bool AES128GCMDecrypt(int isize, Data_buffer *dataIn, Data_buffer *dataOut, u8* enckey)
{
    gcm     oGCM;
    int     dataLeft= isize;
    u8      rgBufIn[4*AES128BYTEBLOCKSIZE];
    u8      rgBufOut[4*AES128BYTEBLOCKSIZE];

    u8     len;
    u8     *sCurPtr, *dCurPtr;
    sCurPtr= dataIn->data;
    dCurPtr= dataOut->data;

#ifdef TEST_D
    fprintf(g_logFile, "AES128GCMDecrypt\n");
#endif
    // init 
    if(!oGCM.initDec(AES128, AES128BYTEKEYSIZE, enckey, isize, 0, AES128BYTEKEYSIZE))
        return false;

    // get and send first cipher block
    memcpy(rgBufIn,sCurPtr,AES128BYTEBLOCKSIZE);
    oGCM.firstCipherBlockIn(rgBufIn);
    dataLeft-= AES128BYTEBLOCKSIZE;
    sCurPtr+= AES128BYTEBLOCKSIZE;
    len= 0;
    // read, decrypt, and write bytes
    while(dataLeft>2*AES128BYTEBLOCKSIZE) {
	    memcpy(rgBufIn,sCurPtr,AES128BYTEBLOCKSIZE);
        oGCM.nextCipherBlockIn(rgBufIn, rgBufOut);
        memcpy(dCurPtr, rgBufOut, AES128BYTEBLOCKSIZE);
        dataLeft-= AES128BYTEBLOCKSIZE;
        dCurPtr+= AES128BYTEBLOCKSIZE; 
        sCurPtr+= AES128BYTEBLOCKSIZE; 
        len+= AES128BYTEBLOCKSIZE;
    }

    // final block
    memcpy(rgBufIn,sCurPtr,dataLeft);
    sCurPtr+= dataLeft;
    int n= oGCM.lastCipherBlockIn(dataLeft, rgBufIn, rgBufOut);
    if(n<0)
        return false;

    len+= n;
    memcpy(dCurPtr, rgBufOut, oGCM.m_iBlockSize);
    dataOut->len= len;
    return oGCM.validateTag();
}
#endif


bool AES128CBCHMACSHA256SYMPADEncrypt (int isize, Data_buffer *dataIn, Data_buffer *dataOut, 
                        u8* enckey, u8* intkey)
{
    cbc     oCBC;
    int     dataLeft= isize;
    u8      iv[AES128BYTEBLOCKSIZE];
    u8      rgBufIn[4*AES128BYTEBLOCKSIZE];
    u8      rgBufOut[4*AES128BYTEBLOCKSIZE];

    u8     *sCurPtr, *dCurPtr;
    unsigned int    len;
    sCurPtr= dataIn->data;
    dCurPtr= dataOut->data;

#ifdef TEST_D
    fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADEncrypt\n");
#endif
    // init iv
    if(!getCryptoRandom(AES128BYTEBLOCKSIZE*NBITSINBYTE, iv)) {
        return false;
    }

    // init 
    if(!oCBC.initEnc(AES128, SYMPAD, HMACSHA256, AES128BYTEKEYSIZE, enckey, AES128BYTEKEYSIZE, 
                     intkey, isize, AES128BYTEBLOCKSIZE, iv))
        return false;

    // get and send first cipher block
    oCBC.firstCipherBlockOut(rgBufOut);
    memcpy(dCurPtr, rgBufOut, AES128BYTEBLOCKSIZE);
    dCurPtr+= AES128BYTEBLOCKSIZE;
    len = AES128BYTEBLOCKSIZE;

    // read, encrypt, and copy bytes
    while(dataLeft>AES128BYTEBLOCKSIZE) {
        memcpy(rgBufIn,sCurPtr,AES128BYTEBLOCKSIZE);
        oCBC.nextPlainBlockIn(rgBufIn, rgBufOut);
    	memcpy(dCurPtr, rgBufOut, AES128BYTEBLOCKSIZE);
        dataLeft-= AES128BYTEBLOCKSIZE;
        dCurPtr+= AES128BYTEBLOCKSIZE;
        sCurPtr+= AES128BYTEBLOCKSIZE;
        len+= AES128BYTEBLOCKSIZE;
    }

    // final block
    memcpy(rgBufIn,sCurPtr,dataLeft);
    sCurPtr+= dataLeft;
    int n= oCBC.lastPlainBlockIn(dataLeft, rgBufIn, rgBufOut);
    if(n<0)
        return false;

    // write final encrypted blocks and HMAC
    memcpy(dCurPtr, rgBufOut, n);
    len+= n;
    
    dataOut->len= len;
    return true;
}


bool AES128CBCHMACSHA256SYMPADDecrypt (int isize, Data_buffer *dataIn, Data_buffer *dataOut,
                         u8* enckey, u8* intkey)
{
    cbc     oCBC;
    int     dataLeft= isize;
    u8      rgBufIn[4*AES128BYTEBLOCKSIZE];
    u8      rgBufOut[4*AES128BYTEBLOCKSIZE];
    u8     *sCurPtr, *dCurPtr;
    u8     len;
    sCurPtr= dataIn->data;
    dCurPtr= dataOut->data;

#ifdef TEST_D
    fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADDecrypt\n");
#endif
    // init 
    if(!oCBC.initDec(AES128, SYMPAD, HMACSHA256, AES128BYTEKEYSIZE, enckey, AES128BYTEKEYSIZE, 
                     intkey, isize))
        return false;

    // get and send first cipher block
    memcpy(rgBufIn,sCurPtr,AES128BYTEBLOCKSIZE);
    oCBC.firstCipherBlockIn(rgBufIn);
    dataLeft-= AES128BYTEBLOCKSIZE;
    sCurPtr+= AES128BYTEBLOCKSIZE;
    len= 0;
    // read, decrypt, and write bytes
    while(dataLeft>3*AES128BYTEBLOCKSIZE) {
        memcpy(rgBufIn,sCurPtr,AES128BYTEBLOCKSIZE);
        oCBC.nextCipherBlockIn(rgBufIn, rgBufOut);
        memcpy(dCurPtr, rgBufOut, AES128BYTEBLOCKSIZE);
        dataLeft-= AES128BYTEBLOCKSIZE;
        dCurPtr+= AES128BYTEBLOCKSIZE;
        sCurPtr+= AES128BYTEBLOCKSIZE;
        len+= AES128BYTEBLOCKSIZE;
    }

    // final blocks
    memcpy(rgBufIn,sCurPtr,dataLeft);
    sCurPtr+= dataLeft;
    int n= oCBC.lastCipherBlockIn(dataLeft, rgBufIn, rgBufOut);
    if(n<0)
        return false;

    // write final decrypted bytes
    memcpy(dCurPtr, rgBufOut, n);
    len+= n;
    dataOut->len= len;
    return oCBC.validateMac();
}


#ifdef GCMENABLED

bool AES128GCMEncryptFile(int filesize, int iRead, int iWrite, u8* enckey)
{
    gcm     oGCM;
    int     ivSize= AES128BYTEBLOCKSIZE-sizeof(u32);
    int     fileLeft= filesize;
    u8      iv[AES128BYTEBLOCKSIZE];
    u8      rgBufIn[4*AES128BYTEBLOCKSIZE];
    u8      rgBufOut[4*AES128BYTEBLOCKSIZE];

#ifdef TEST_D
    fprintf(g_logFile, "AES128GCMDecryptFile\n");
#endif
    // init iv
    if(!getCryptoRandom(ivSize*NBITSINBYTE, iv)) {
        return false;
    }
    memset(&iv[ivSize],0, sizeof(u32));

    // init 
    if(!oGCM.initEnc(AES128, ivSize, iv, AES128BYTEKEYSIZE, enckey, filesize, 0, AES128BYTEKEYSIZE))
        return false;

    // get and send first cipher block
    oGCM.firstCipherBlockOut(rgBufOut);
    write(iWrite, rgBufOut, AES128BYTEBLOCKSIZE);

    // read, encrypt, and write bytes
    while(fileLeft>AES128BYTEBLOCKSIZE) {
        if(read(iRead, rgBufIn, AES128BYTEBLOCKSIZE)<0) {
            return false;
        }
        oGCM.nextPlainBlockIn(rgBufIn, rgBufOut);
        write(iWrite, rgBufOut, AES128BYTEBLOCKSIZE);
        fileLeft-= AES128BYTEBLOCKSIZE;
    }

    // final block
    if(read(iRead, rgBufIn, fileLeft)<0) {
        return false;
    }
    int n= oGCM.lastPlainBlockIn(fileLeft, rgBufIn, rgBufOut);
    if(n<0)
        return false;

    // write final encrypted block
    write(iWrite, rgBufOut, n);

    // write tag
    oGCM.getTag(oGCM.m_iBlockSize, rgBufOut);
    write(iWrite, rgBufOut, oGCM.m_iBlockSize);

    return true;
}


bool AES128GCMDecryptFile(int filesize, int iRead, int iWrite, u8* enckey)
{
    gcm     oGCM;
    int     fileLeft= filesize;
    u8      rgBufIn[4*AES128BYTEBLOCKSIZE];
    u8      rgBufOut[4*AES128BYTEBLOCKSIZE];

#ifdef TEST_D
    fprintf(g_logFile, "AES128GCMDecryptFile\n");
#endif
    // init 
    if(!oGCM.initDec(AES128, AES128BYTEKEYSIZE, enckey, filesize, 0, AES128BYTEKEYSIZE))
        return false;

    // get and send first cipher block
    if(read(iRead, rgBufIn, AES128BYTEBLOCKSIZE)<0) {
        fprintf(g_logFile, "bad read\n");
        return false;
    }
    oGCM.firstCipherBlockIn(rgBufIn);
    fileLeft-= AES128BYTEBLOCKSIZE;

    // read, decrypt, and write bytes
    while(fileLeft>2*AES128BYTEBLOCKSIZE) {
        if(read(iRead, rgBufIn, AES128BYTEBLOCKSIZE)<0) {
            fprintf(g_logFile, "bad read\n");
            return false;
        }
        oGCM.nextCipherBlockIn(rgBufIn, rgBufOut);
        write(iWrite, rgBufOut, AES128BYTEBLOCKSIZE);
        fileLeft-= AES128BYTEBLOCKSIZE;
    }

    // final block
    read(iRead, rgBufIn, fileLeft);
    int n= oGCM.lastCipherBlockIn(fileLeft, rgBufIn, rgBufOut);
    if(n<0)
        return false;

    // write final decrypted bytes
    if(write(iWrite, rgBufOut, n)<0) {
        return false;
    }

    return oGCM.validateTag();
}
#endif


bool AES128CBCHMACSHA256SYMPADEncryptFile (int filesize, int iRead, int iWrite, 
                        u8* enckey, u8* intkey)
{
    cbc     oCBC;
    int     fileLeft= filesize;
    u8      iv[AES128BYTEBLOCKSIZE];
    u8      rgBufIn[4*AES128BYTEBLOCKSIZE];
    u8      rgBufOut[4*AES128BYTEBLOCKSIZE];

#ifdef TEST_D
    fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADEncryptFile\n");
#endif
    // init iv
    if(!getCryptoRandom(AES128BYTEBLOCKSIZE*NBITSINBYTE, iv)) {
        return false;
    }

    // init 
    if(!oCBC.initEnc(AES128, SYMPAD, HMACSHA256, AES128BYTEKEYSIZE, enckey, AES128BYTEKEYSIZE, 
                     intkey, filesize, AES128BYTEBLOCKSIZE, iv))
        return false;

    // get and send first cipher block
    oCBC.firstCipherBlockOut(rgBufOut);
    if(write(iWrite, rgBufOut, AES128BYTEBLOCKSIZE)<0) {
        return false;
    }

    // read, encrypt, and copy bytes
    while(fileLeft>AES128BYTEBLOCKSIZE) {
        if(read(iRead, rgBufIn, AES128BYTEBLOCKSIZE)<0) {
            return false;
        }
        oCBC.nextPlainBlockIn(rgBufIn, rgBufOut);
        if(write(iWrite, rgBufOut, AES128BYTEBLOCKSIZE)<0) {
            return false;
        }
        fileLeft-= AES128BYTEBLOCKSIZE;
    }

    // final block
    if(read(iRead, rgBufIn, fileLeft)<0) {
        return false;
    }
    int n= oCBC.lastPlainBlockIn(fileLeft, rgBufIn, rgBufOut);
    if(n<0)
        return false;

    // write final encrypted blocks and HMAC
    if(write(iWrite, rgBufOut, n)<0) {
        return false;
    }

    return true;
}


bool AES128CBCHMACSHA256SYMPADDecryptFile (int filesize, int iRead, int iWrite,
                         u8* enckey, u8* intkey)
{
    cbc     oCBC;
    int     fileLeft= filesize;
    u8      rgBufIn[4*AES128BYTEBLOCKSIZE];
    u8      rgBufOut[4*AES128BYTEBLOCKSIZE];

#ifdef TEST_D
    fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADDecryptFile\n");
#endif
    // init 
    if(!oCBC.initDec(AES128, SYMPAD, HMACSHA256, AES128BYTEKEYSIZE, enckey, AES128BYTEKEYSIZE, 
                     intkey, filesize))
        return false;

    // get and send first cipher block
    if(read(iRead, rgBufIn, AES128BYTEBLOCKSIZE)<0) {
            return false;
        }
    oCBC.firstCipherBlockIn(rgBufIn);
    fileLeft-= AES128BYTEBLOCKSIZE;

    // read, decrypt, and write bytes
    while(fileLeft>3*AES128BYTEBLOCKSIZE) {
        if(read(iRead, rgBufIn, AES128BYTEBLOCKSIZE)<0) {
            return false;
        }
        oCBC.nextCipherBlockIn(rgBufIn, rgBufOut);
        if(write(iWrite, rgBufOut, AES128BYTEBLOCKSIZE)<0) {
            return false;
        }
        fileLeft-= AES128BYTEBLOCKSIZE;
    }

    // final blocks
    if(read(iRead, rgBufIn, fileLeft)<0) {
            return false;
        }
    int n= oCBC.lastCipherBlockIn(fileLeft, rgBufIn, rgBufOut);
    if(n<0)
        return false;

    // write final decrypted bytes
    if(write(iWrite, rgBufOut, n)<0) {
        return false;
    }

    return oCBC.validateMac();
}


// --------------------------------------------------------------------- 

#define ONERSA 1

int RSAEncrypt(const char *szRSAPad, Data_buffer* dataIn, Data_buffer* dataOut, RSAPublicKey *rsaPubKey)
{
    int dataLeft= 0;
    u8  *sCurPtr, *dCurPtr;
    int  len=0;

    //This is in bytes
    int blockSize= 0;
    int padSize = 0;
    

    if (szRSAPad == NULL) return false;
    if (dataIn == NULL || dataOut == NULL) return false;
    if (dataIn->data == NULL || dataOut->data == NULL) return false;
    if (rsaPubKey == NULL) return false;
    if ( rsaPubKey->n.data == NULL || rsaPubKey->n.len <=0 ) return false;
    if ( rsaPubKey->e.data == NULL || rsaPubKey->e.len <=0 ) return false;


    blockSize= rsaPubKey->n.len;
 
    int     iRet= 0;
    
    int  irsaPadType= RSANOPADDING;

    if(strcmp(szRSAPad,"RSA_NO_PADDING")==0) {
        irsaPadType= RSANOPADDING;
        padSize = 0;
    }

    if(strcmp(szRSAPad,"RSA_PKCS1_PADDING")==0) {
        irsaPadType= RSAPKCS1PADDING;
        padSize = 12;
    }
    if(strcmp(szRSAPad,"RSA_PKCS1_OAEP_PADDING")==0) {
        irsaPadType= RSAPKCS1OAEPPADDING;
        padSize = 42;
    }

    //For now, ignore padding.
    if(irsaPadType !=RSANOPADDING) 
	return false;
    
    blockSize-= padSize ;
    dataLeft= dataIn->len;
    
    if (dataLeft > blockSize) 
	return false;

    sCurPtr= dataIn->data;
    dCurPtr= dataOut->data;

    u8  rguoriginalMsg[1024];
    u8  rguencryptedMsg[1024];

    RSAKey rsakey;
    rsakey.m_pbnM = new bnum(rsaPubKey->n.len/sizeof(u64)+1);
    rsakey.m_pbnE = new bnum(rsaPubKey->e.len/sizeof(u64)+1);


    ZeroWords(rsakey.m_pbnM->mpSize(), rsakey.m_pbnM->m_pValue);
    ZeroWords(rsakey.m_pbnE->mpSize(), rsakey.m_pbnE->m_pValue);
    memcpy((u8 *)rsakey.m_pbnM->m_pValue, rsaPubKey->n.data ,rsaPubKey->n.len);
    memcpy((u8 *)rsakey.m_pbnE->m_pValue, rsaPubKey->e.data ,rsaPubKey->e.len); 
	
    memcpy((u8 *)rsakey.m_rgbM, rsaPubKey->n.data ,rsaPubKey->n.len);
    rsakey.m_iByteSizeM = rsaPubKey->n.len;
	
    memcpy((u8 *)rsakey.m_rgbE, rsaPubKey->e.data ,rsaPubKey->e.len);
    rsakey.m_iByteSizeE = rsaPubKey->e.len;

    
    int  outSize = blockSize;

    if (!RSAEncrypt_i(rsakey, blockSize, sCurPtr, &outSize, dCurPtr)) {
	fprintf(g_logFile, "Can't encrypt\n");
	iRet= false;
    }

    len= rsaPubKey->n.len;
    iRet= true;
    dataOut->len= outSize;

    // For later code refactoring
    return iRet;
}




int RSADecrypt(const char *szRSAPad, Data_buffer* dataIn, Data_buffer* dataOut,RSAPrivateKey* rsaPriKey)
{

    int dataLeft= 0;
    u8  *sCurPtr, *dCurPtr;
    int len= 0;

    //This is in bytes
    int blockSize= 0;
    int padSize = 0;

    if (szRSAPad == NULL) return false;
    if(dataIn == NULL || dataOut == NULL) return false;
    if(dataIn->data == NULL || dataOut->data == NULL) return false;
    if (rsaPriKey == NULL) return false;
    if(rsaPriKey->n.data == NULL || rsaPriKey->n.len <=0 ) return false;
    if(rsaPriKey->d.data == NULL || rsaPriKey->d.len <=0 ) return false;

    blockSize= rsaPriKey->n.len;
  
    int     iRet= true;;
    int     irsaPadType= RSANOPADDING;

    if(strcmp(szRSAPad,"RSA_NO_PADDING")==0) {
        irsaPadType= RSANOPADDING;
        padSize = 0;
    }

    if(strcmp(szRSAPad,"RSA_PKCS1_PADDING")==0) {
        irsaPadType= RSAPKCS1PADDING;
        padSize = 11;
    }
    if(strcmp(szRSAPad,"RSA_PKCS1_OAEP_PADDING")==0) {
        irsaPadType= RSAPKCS1OAEPPADDING;
        padSize = 43;
    }
    if(irsaPadType !=RSANOPADDING) return false; //blank statement to avoid warning
    if(padSize); //Empty statement to remove warning
    dataLeft= dataIn->len;

    if (dataLeft > blockSize ) return false;

    u8  rguoriginalMsg[1024];
    u8  rgudecryptedMsg[1024]; 
    
    sCurPtr= dataIn->data;
    dCurPtr= dataOut->data;
   
    RSAKey rsakey;
    rsakey.m_pbnM = new bnum(rsaPriKey->n.len/sizeof(u64)+1);
    rsakey.m_pbnD = new bnum(rsaPriKey->d.len/sizeof(u64)+1);

    memcpy((u8 *)rsakey.m_pbnM->m_pValue, rsaPriKey->n.data ,rsaPriKey->n.len);
    memcpy((u8 *)rsakey.m_pbnD->m_pValue, rsaPriKey->d.data ,rsaPriKey->d.len);

    memcpy((u8 *)rsakey.m_rgbM, rsaPriKey->n.data ,rsaPriKey->n.len);
    rsakey.m_iByteSizeM = rsaPriKey->n.len;
	
    memcpy((u8 *)rsakey.m_rgbD, rsaPriKey->d.data ,rsaPriKey->d.len);
    rsakey.m_iByteSizeD = rsaPriKey->d.len;
   
       
    int  outSize = blockSize;

    if (!RSADecrypt_i(rsakey, blockSize, sCurPtr, &outSize, dCurPtr)) {
	fprintf(g_logFile, "Can't encrypt\n");
	iRet= false;
     }
    

    dataOut->len= outSize;

    //iRet for later code refactoring
    return iRet;
}


int EncryptFile(SymKey* pSymKey, SymKey* intKey, const char* szInFile, const char* szOutFile, u32 mode, u32 alg,u32 pad,u32 mac)
{
    u8          rguEncKey[BIGSYMKEYSIZE];
    u8          rguIntKey[BIGSYMKEYSIZE];

#ifdef TEST_D
    fprintf(g_logFile, "EncryptFile (%s, %s)\n", szInFile, szOutFile);
    if(mode==CBCMODE)
        fprintf(g_logFile, "CBC Mode\n");
    else
        fprintf(g_logFile, "GCM Mode\n");
#endif
    if(szInFile==NULL) return false;
    if(szOutFile==NULL) return false;
    if(pSymKey->data==NULL) return false;
    if(pSymKey->len<=0) return false;
    memset(rguEncKey , 0, BIGSYMKEYSIZE);
    memset(rguIntKey , 0, BIGSYMKEYSIZE);

    // Get File size
    struct stat statBlock;
    if(stat(szInFile, &statBlock)<0) {
        return false;
    }
    int fileSize= statBlock.st_size;

    int iRead= open(szInFile, O_RDONLY);
    if(iRead<0) {
        return false;
    }

    int iWrite= open(szOutFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(iWrite<0) {
        return false;
    }

    if (alg==AES128) {
    	memcpy(rguEncKey, pSymKey->data,AES128BYTEKEYSIZE); 
    	if(mode==CBCMODE) {
    		    if(intKey->data==NULL) return false;
    		    if(intKey->len<=0) return false;
		     memcpy(rguIntKey, intKey->data,AES128BYTEKEYSIZE); 
	}
    } else {
        if (alg==AES256) {
           	memcpy(rguEncKey, pSymKey->data,AES256BYTEKEYSIZE); 
        	if(mode==CBCMODE) {
			if(intKey->data==NULL) return false;
			if(intKey->len<=0) return false;
			 memcpy(rguIntKey, intKey->data,AES256BYTEKEYSIZE);
		}
           }
        else return false;
    }

    bool fRet= false;


    //Currently only the following combinations are supported. New functions need to be defined
    // if the options are expanded.

    if(alg==AES128 && mode==CBCMODE && mac==HMACSHA256 && pad==SYMPAD)
        fRet= AES128CBCHMACSHA256SYMPADEncryptFile(fileSize, iRead, iWrite, rguEncKey, rguIntKey);
#ifdef GCMENABLED
    else if(alg==AES128 && mode==GCMMODE)
        fRet= AES128GCMEncryptFile(fileSize, iRead, iWrite, rguEncKey);
#endif
    else
        fRet= false;
    
    close(iRead);
    close(iWrite);
    memset(rguEncKey , 0, BIGSYMKEYSIZE);
    memset(rguIntKey , 0, BIGSYMKEYSIZE);

#ifdef TEST_D
    if(fRet)
        fprintf(g_logFile, "Encrypt/Decrypt returns true\n");
    else
        fprintf(g_logFile, "Encrypt/Decrypt returns false\n");
#endif
    return fRet;

}

int DecryptFile(SymKey* pSymKey, SymKey* intKey, const char* szInFile, const char* szOutFile, u32 mode, u32 alg,u32 pad,u32 mac)
{

    u8          rguEncKey[BIGSYMKEYSIZE];
    u8          rguIntKey[BIGSYMKEYSIZE];

#ifdef TEST_D
    fprintf(g_logFile, "DecryptFile (%s, %s)\n", szInFile, szOutFile);
    if(mode==CBCMODE)
        fprintf(g_logFile, "CBC Mode\n");
    else
        fprintf(g_logFile, "GCM Mode\n");
#endif
    if(szInFile==NULL) return false;
    if(szOutFile==NULL) return false;
    if(pSymKey->data==NULL) return false;
    if(pSymKey->len<=0) return false;
    memset(rguEncKey , 0, BIGSYMKEYSIZE);
    memset(rguIntKey , 0, BIGSYMKEYSIZE);

    // Get File size
    struct stat statBlock;
    if(stat(szInFile, &statBlock)<0) {
        return false;
    }
    int fileSize= statBlock.st_size;

    int iRead= open(szInFile, O_RDONLY);
    if(iRead<0) {
        return false;
    }

    int iWrite= open(szOutFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(iWrite<0) {
        return false;
    }
   
    if (alg==AES128) {
    	memcpy(rguEncKey, pSymKey->data,AES128BYTEKEYSIZE); 
    	if(mode==CBCMODE) {
		if(intKey->data==NULL) return false;
		if(intKey->len<=0) return false;
    		memcpy(rguIntKey, intKey->data,AES128BYTEKEYSIZE); 
	}
    } else {
        if (alg==AES256) {
    	     memcpy(rguEncKey, pSymKey->data,AES256BYTEKEYSIZE); 
             if(mode==CBCMODE) {
			if(intKey->data==NULL) return false;
			if(intKey->len<=0) return false;
        		memcpy(rguIntKey, intKey->data,AES256BYTEKEYSIZE); 
	     }
        } else return false;
     }

    bool fRet= false;

    //Currently only the following combinations are supported. New functions need to be defined
    // if the options are expanded. This has to be in sync with EncryptFile function.
  if( alg==AES128 && mode==CBCMODE && mac==HMACSHA256 && pad==SYMPAD) 
    fRet= AES128CBCHMACSHA256SYMPADDecryptFile(fileSize, iRead, iWrite, rguEncKey, rguIntKey);
#ifdef GCMENABLED
    if( alg==AES128 && mode==GCMMODE)
        fRet= AES128GCMDecryptFile(fileSize, iRead, iWrite, rguEncKey);
#endif
    else
        fRet= false;
    
    close(iRead);
    close(iWrite);
    memset(rguEncKey , 0, BIGSYMKEYSIZE);
    memset(rguIntKey , 0, BIGSYMKEYSIZE);

#ifdef TEST_D
    if(fRet)
        fprintf(g_logFile, "Encrypt/Decrypt returns true\n");
    else
        fprintf(g_logFile, "Encrypt/Decrypt returns false\n");
#endif
    return fRet;
}


int Encrypt(SymKey* pSymKey,SymKey* intKey, Data_buffer* dataIn, Data_buffer* dataOut, u32 mode, u32 alg,  u32 pad, u32 mac)
{
    u8          rguEncKey[BIGSYMKEYSIZE];
    u8          rguIntKey[BIGSYMKEYSIZE];


#ifdef TEST_D
    fprintf(g_logFile, "Encrypt ()\n");
    if(mode==CBCMODE)
        fprintf(g_logFile, "CBC Mode\n");
    else
        fprintf(g_logFile, "GCM Mode\n");
#endif
    if(pSymKey->data==NULL) return false;
    if(pSymKey->len<=0) return false;

    if(dataIn->data==NULL) return false;
    if(dataIn->len<=0) return false;

    if(dataOut->data==NULL) return false;
    if(dataOut->len<=0) return false;

    memset(rguEncKey , 0, BIGSYMKEYSIZE);
    memset(rguIntKey , 0, BIGSYMKEYSIZE);

    int iSize= dataIn->len;

    if (alg ==AES128) {
    	memcpy(rguEncKey, pSymKey->data,AES128BYTEKEYSIZE); 
    	if(mode==CBCMODE) {
    		if(intKey->data==NULL) return false;
    		if(intKey->len<=0) return false;
    		memcpy(rguIntKey, intKey->data,AES128BYTEKEYSIZE);  
	}
    } else {
        if (alg==AES256) {
    	    memcpy(rguEncKey, pSymKey->data,AES256BYTEKEYSIZE); 
    	    if(mode==CBCMODE) {
    		if(intKey->data==NULL) return false;
    		if(intKey->len<=0) return false;
    	    	memcpy(rguIntKey, intKey->data,AES256BYTEKEYSIZE);  
	    }
          } else return false;
        }

    bool fRet= false;

    //Currently only the following combinations are supported. New functions need to be defined
    // if the options are expanded. This has to be in sync with Encrypt function.
    if(alg==AES128 && mode==CBCMODE && mac==HMACSHA256 && pad==SYMPAD)
        fRet= AES128CBCHMACSHA256SYMPADEncrypt(iSize, dataIn,dataOut,
                     rguEncKey, rguIntKey);
#ifdef GCMENABLED
    else if( alg==AES128 && mode==GCMMODE)
        fRet= AES128GCMEncrypt(iSize, dataIn,dataOut, rguEncKey);
#endif
    else
        fRet= false;
    
    memset(rguEncKey , 0, BIGSYMKEYSIZE);
    memset(rguIntKey , 0, BIGSYMKEYSIZE);

#ifdef TEST_D
    if(fRet)
        fprintf(g_logFile, "Encrypt/Decrypt returns true\n");
    else
        fprintf(g_logFile, "Encrypt/Decrypt returns false\n");
#endif
    return fRet;

}

int Decrypt(SymKey* pSymKey, SymKey* intKey, Data_buffer* dataIn, Data_buffer* dataOut, u32 mode, u32 alg,  u32 pad, u32 mac)
{

    u8          rguEncKey[BIGSYMKEYSIZE];
    u8          rguIntKey[BIGSYMKEYSIZE];
#ifdef TEST_D
    fprintf(g_logFile, "Decrypt() \n");
    if(mode==CBCMODE)
        fprintf(g_logFile, "CBC Mode\n");
    else
        fprintf(g_logFile, "GCM Mode\n");
#endif
    if(pSymKey->data==NULL) return false;
    if(pSymKey->len<=0) return false;

    if(dataIn->data==NULL) return false;
    if(dataIn->len<=0) return false;

    if(dataOut->data==NULL) return false;
    if(dataOut->len<=0) return false;
    memset(rguEncKey , 0, BIGSYMKEYSIZE);
    memset(rguIntKey , 0, BIGSYMKEYSIZE);

    int iSize= dataIn->len;

    if (alg==AES128) {
    	memcpy(rguEncKey, pSymKey->data,AES128BYTEKEYSIZE); 
    	if(mode==CBCMODE) {
    		if(intKey->data==NULL) return false;
    		if(intKey->len<=0) return false;
    		memcpy(rguIntKey, intKey->data,AES128BYTEKEYSIZE);  
	}
    } else {
        if (alg==AES256) {
    	    memcpy(rguEncKey, pSymKey->data,AES256BYTEKEYSIZE); 
    	    if(mode==CBCMODE) {
    		    if(intKey->data==NULL) return false;
    		    if(intKey->len<=0) return false;
    		    memcpy(rguIntKey, intKey->data,AES256BYTEKEYSIZE); 
	     }
        } else return false;
    }

    bool fRet= false;

    //Currently only the following combinations are supported. New functions need to be defined
    // if the options are expanded. This has to be in sync with Encrypt function.
    if( alg==AES128 && mode==CBCMODE && mac==HMACSHA256 && pad==SYMPAD)
        fRet= AES128CBCHMACSHA256SYMPADDecrypt(iSize, dataIn, dataOut, 
                     rguEncKey, rguIntKey);
#ifdef GCMENABLED
    if( alg==AES128 && mode==GCMMODE)
        fRet= AES128GCMDecrypt(iSize, dataIn, dataOut, rguEncKey);
#endif
    else
        fRet= false;
    
    memset(rguEncKey , 0, BIGSYMKEYSIZE);
    memset(rguIntKey , 0, BIGSYMKEYSIZE);

#ifdef TEST_D
    if(fRet)
        fprintf(g_logFile, "Encrypt/Decrypt returns true\n");
    else
        fprintf(g_logFile, "Encrypt/Decrypt returns false\n");
#endif
    return fRet;
}


int DeriveKey(const char* szKeyType, const char* szMethod, Data_buffer *password, Data_buffer *salt, SymKey *keyOut,u32 iterations)
{
	u8 *ptrSalt, rgKeybuf[SHA256_DIGESTSIZE_BYTES];
	u8 uD1[SHA256_DIGESTSIZE_BYTES], uD2[SHA256_DIGESTSIZE_BYTES];
	u32 i, j;
	u32 k;
	u32 minLen;
	u8 *ptrKey; 

	if(szKeyType==NULL) {
		return 0;
	}
	if(szMethod==NULL) {
		return 0;
	}
	if(iterations < 1 ) {
		return 0;
	}
	if(salt->data==NULL) {
		return 0;
	}
	if (salt->len == 0 || salt->len > INT32_MAX - 4) {
		return 0;
	}
	if(password->data==NULL) {
		return 0;
	}

	if(password->len <1 ) {
		return 0;
	}
	//u8 dLen= 0;
	u32 keyLen= 0;
	i32 leftKeyLen= 0;
	// For now only PBKDF2 is accepted.
	if(strcmp(szMethod,"PBKDF2") != 0) return 0;
	if(strcmp(szKeyType,"AES128") == 0)  keyLen=AES128BYTEKEYSIZE;
	if(strcmp(szKeyType,"AES256") == 0) keyLen=AES256BYTEKEYSIZE;
	if (keyLen==0) return 0;

	if(keyOut->data == NULL) return 0;
	if(keyOut->len<keyLen) return 0;

	ptrKey = keyOut->data;
        leftKeyLen = keyLen;
	if ((ptrSalt = (u8 *)malloc(salt->len + 4)) == NULL)
		return 0;
	memcpy(ptrSalt, salt->data, salt->len);

	for (k = 1; leftKeyLen > 0; k++) {
		ptrSalt[salt->len + 0] = (k >> 24) & 0xff;
		ptrSalt[salt->len + 1] = (k >> 16) & 0xff;
		ptrSalt[salt->len + 2] = (k >> 8) & 0xff;
		ptrSalt[salt->len + 3] = k & 0xff;
		//hmac_sha1(ptrSalt, salt->len + 4, password->data, password->len, uD1);
		hmac_sha256(ptrSalt, salt->len + 4, password->data, password->len, uD1);
		//memcpy(rgKeybuf, uD1, dLen);
		memcpy(rgKeybuf, uD1, SHA256_DIGESTSIZE_BYTES);

		for (i = 1; i < iterations; i++) {
			hmac_sha256(uD1, SHA256_DIGESTSIZE_BYTES, password->data, password->len, uD2);
			memcpy(uD1, uD2, SHA256_DIGESTSIZE_BYTES);
			for (j = 0; j < SHA256_DIGESTSIZE_BYTES; j++)
				rgKeybuf[j] ^= uD1[j];
		}

		minLen =  SHA256_DIGESTSIZE_BYTES;
		if (keyLen < SHA256_DIGESTSIZE_BYTES) minLen = keyLen;
		memcpy(ptrKey, rgKeybuf, minLen);
		ptrKey+= minLen;
		leftKeyLen -= minLen;
	};

	free(ptrSalt);
	keyOut->len= keyLen;
	return 1;
}

//ToDo
//KDF functions
int DeriveKey1(char* szKeyType, char* szMethod, Data_buffer *keyIn, SymKey *keyOut)
{
	int iRet;
	iRet=0;
	return iRet;
}

//ToDo
// Digest functions

int GetSha256Digest(Data_buffer *In,Data_buffer *Digest)
{
    Sha256      oHash;

    //Not enough buffer;

    if(In->data==NULL) return false;
    if(In->len<=0) return false;
    if(Digest->data==NULL) return false;
    if(Digest->len < SHA256_DIGESTSIZE_BYTES) return false;
    oHash.Init();
    oHash.Update(In->data, In->len);
    oHash.Final();
    oHash.GetDigest(Digest->data);

    Digest->len =SHA256_DIGESTSIZE_BYTES;
    return true;
}

#define SHA1_DIGESTSIZE_BYTES 20

int GetSha1Digest(Data_buffer *In,Data_buffer *Digest)
{
    Sha1      oHash;

    if(In->data==NULL) return false; 
    if(In->len<=0) return false;
    if(Digest->data==NULL) return false;
    if(Digest->len < SHA1_DIGESTSIZE_BYTES)
        return false;

    oHash.Init();
    oHash.Update(In->data, In->len);
    oHash.Final();
    oHash.getDigest(Digest->data);
    Digest->len =SHA1_DIGESTSIZE_BYTES;

    return true;
}

int Sha256InitDigest(char* szDigestType, Digest_context *ctx)
{

    if (szDigestType==NULL) return false;
    if (ctx==NULL) return false;
    // For now accept on SHA-256 type
    if(strcmp(szDigestType, "SHA-256") != 0) return false;
    Sha256      *pHash= new Sha256;
    pHash->Init();
    *ctx= (Digest_context)pHash;
    return true;
}

int Sha256UpdateDigest(Digest_context *ctx, Data_buffer *data)
{

    if (ctx==NULL) return false;
    if (*ctx==NULL) return false;
    Sha256      *pHash= (Sha256 *)*ctx;
    if(data->data==NULL) return false;
    if(data->len<=0) return false;
    pHash->Update(data->data, data->len);
    return true;
}

int Sha256GetDigest(Digest_context *ctx, Data_buffer *digest)
{

    if (ctx==NULL) return false;
    if (*ctx==NULL) return false;
    Sha256      *pHash= (Sha256 *)*ctx;
    pHash->Final();
    if(digest->data==NULL) return false;
    if(digest->len<SHA256_DIGESTSIZE_BYTES) return false;
    pHash->GetDigest(digest->data);
    digest->len= SHA256_DIGESTSIZE_BYTES;
    return true;
}

int Sha256CloseDigest(Digest_context *ctx)
{

    if (ctx==NULL) return false;
    if (*ctx==NULL) return false;
    Sha256      *pHash= (Sha256 *)*ctx;
    delete pHash;
    return true;
}

//--------------------------------------------------------------
int Sha1InitDigest(char* szDigestType, Digest_context *ctx)
{

    if (szDigestType==NULL) return false;
    if (ctx==NULL) return false;
    // For now accept on SHA-256 type
    if(strcmp(szDigestType, "SHA-1") != 0) return false;
    Sha1      *pHash= new Sha1;
    pHash->Init();
    *ctx= (Digest_context)pHash;
    return true;
}

int Sha1UpdateDigest(Digest_context *ctx, Data_buffer *data)
{

    if (ctx==NULL) return false;
    if (*ctx==NULL) return false;
    Sha1      *pHash= (Sha1 *)*ctx;
    if(data->data==NULL) return false;
    if(data->len<=0) return false;
    pHash->Update(data->data, data->len);
    return true;
}

int Sha1GetDigest(Digest_context *ctx, Data_buffer *digest)
{

    if (ctx==NULL) return false;
    if (*ctx==NULL) return false;
    Sha1      *pHash= (Sha1 *)*ctx;
    pHash->Final();
    if(digest->data==NULL) return false;
    if(digest->len<SHA1_DIGESTSIZE_BYTES) return false;
    pHash->getDigest(digest->data);
    digest->len= SHA1_DIGESTSIZE_BYTES;
    return true;
}

int Sha1CloseDigest(Digest_context *ctx)
{
    if (ctx==NULL) return false;
    if (*ctx==NULL) return false;
    Sha1      *pHash= (Sha1 *)*ctx;
    delete pHash;
    return true;
}
//MAC functions
//int GetMAC(char* szMACType, SymKey *key, Data_buffer *digest, Data_buffer *mac)
int GetMAC(char* szMACType, SymKey *key, Data_buffer *rguMsg, Data_buffer *mac)
{
hmacsha256  ohmac;

    if(szMACType==NULL) return false;
    if(strcmp(szMACType, "HMAC") != 0) return false;

    //not enough buffer
    if(mac==NULL) return false;
    if(mac->len < SHA256_DIGESTSIZE_BYTES)
       return false;

    if(key->data==NULL) return false;
    if(key->len<=0) return false;
    ohmac.Init(key->data, key->len);
    if(rguMsg->data==NULL) return false;
    if(rguMsg->len<=0) return false;
    ohmac.Update(rguMsg->data, rguMsg->len);
    ohmac.Final(mac->data);
    mac->len= SHA256_DIGESTSIZE_BYTES;
    return true;
}

// --------------------------------------------------------------------

const char*   szSigHeader= 
          "<ds:Signature xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" Id='uniqueid'>\n";
const char*   szSigValueBegin= "    <ds:SignatureValue>    \n";
const char*   szSigValueEnd= "\n    </ds:SignatureValue>\n";
const char*   szSigTrailer= "</ds:Signature>\n";


bool SignXML(const char* szKeyFile, const char* szAlgorithm, const char* szInFile, const char* szOutFile)
{
    int         iAlgIndex;
    Sha256      oHash;
    u8          rgHashValue[SHA256_DIGESTSIZE_BYTES];
    u8          rgToSign[512];
    char        rgBase64Sig[1024];
    int         iSigSize= 512;
    TiXmlNode*  pNode= NULL;
    TiXmlNode*  pNode1= NULL;
    char*       szToHash= NULL;
    RSAKey*     pRSAKey= NULL;
    char*       szKeyInfo= NULL;
    int         iWrite= -1;
    bool        fRet= true;

    if(szKeyFile==NULL) {
        fprintf(g_logFile, "No Key file\n");
        return false;
    }
    if(szAlgorithm==NULL) {
        fprintf(g_logFile, "No Algorithm specifier\n");
        return false;
    }
    if(szInFile==NULL) {
        fprintf(g_logFile, "No Input file\n");
        return false;
    }
    if(szOutFile==NULL) {
        fprintf(g_logFile, "No Output file\n");
        return false;
    }

#ifdef TEST
    fprintf(g_logFile, "SignXML(%s, %s, %s, %s)\n", szKeyFile, szAlgorithm, szInFile, szOutFile);
#endif

    try {

        pRSAKey= (RSAKey*)ReadKeyfromFile(szKeyFile);
        if(pRSAKey==NULL)
            throw "Cant parse or open Keyfile\n";
        if(((KeyInfo*)pRSAKey)->m_ukeyType!=RSAKEYTYPE) {
            delete (KeyInfo*) pRSAKey;
            pRSAKey= NULL;
            throw "Wrong key type for signing\n";
        }
        // Signature algorithm
        iAlgIndex= algorithmIndexFromShortName(szAlgorithm);
        if(iAlgIndex<0)
            throw "Unsupported signing algorithm\n";

        fprintf(g_logFile, "\n");
#ifdef TEST
        pRSAKey->printMe();
#endif
        fprintf(g_logFile, "\n");

        // read input file
        TiXmlDocument toSignDoc;
        if(!toSignDoc.LoadFile(szInFile))
            throw "Can't open file for signing\n";
        
        pNode= Search(toSignDoc.RootElement(),"ds:SignedInfo");
        if(pNode==NULL) {
            fprintf(g_logFile, "Can't find SignedInfo\n");
            return false;
        }

        // Canonicalize
        szToHash= canonicalize(pNode);
        if(szToHash==NULL) 
            throw "Can't canonicalize\n";

        pNode1= Search(toSignDoc.RootElement(), "ds:SignatureMethod");
        if(pNode1==NULL)
            throw "Can't find SignatureMethod\n";

        char* szAlgLongName= longAlgNameFromIndex(iAlgIndex);
        if(szAlgLongName==NULL)
            throw "Can't find Algorithm index\n";
        ((TiXmlElement*) pNode1)->SetAttribute("Algorithm", szAlgLongName);
    
        // hash it
        if(hashAlgfromIndex(iAlgIndex)==SHA256HASH) {
            oHash.Init();
            oHash.Update((byte*) szToHash , strlen(szToHash));
            oHash.Final();
            oHash.GetDigest(rgHashValue);
        }
        else 
            throw "Unsupported hash algorithm\n";

        // pad it
        if(padAlgfromIndex(iAlgIndex)==PKCSPAD) {
            memset(rgToSign, 0, 512);
            if(!emsapkcspad(SHA256HASH, rgHashValue, pRSAKey->m_iByteSizeM, rgToSign)) 
                throw "Padding failure in Signing\n";
        }
        else
            throw  "Unsupported hash algorithm\n";

        #ifdef TEST
           //PrintBytes("Padded block\n", rgSig, sigSize);
           PrintBytes("Padded block\n", rgToSign, pRSAKey->m_iByteSizeM);
        #endif

//ToDo

        bnum    bnMsg(pRSAKey->m_iByteSizeM/2);
        bnum    bnOut(pRSAKey->m_iByteSizeM/2);

        iSigSize= pRSAKey->m_iByteSizeM;
        // encrypt with private key
        if( (pkAlgfromIndex(iAlgIndex)==RSA2048 && pRSAKey->m_iByteSizeM == 256) ||
            (pkAlgfromIndex(iAlgIndex)==RSA1024 && pRSAKey->m_iByteSizeM == 128)) {

            memset(bnMsg.m_pValue, 0, pRSAKey->m_iByteSizeM);
            memset(bnOut.m_pValue, 0, pRSAKey->m_iByteSizeM);

            revmemcpy((byte*)bnMsg.m_pValue, rgToSign, iSigSize);
            if(iSigSize>pRSAKey->m_iByteSizeM) 
                throw "Signing key block too small\n";
            if((pRSAKey->m_iByteSizeM==0) || (pRSAKey->m_iByteSizeD==0))
                throw "Signing keys not available\n";

            if(!mpRSAENC(bnMsg, *(pRSAKey->m_pbnD), *(pRSAKey->m_pbnM), bnOut))
                throw "Can't sign with private key\n";
        }
        else 
            throw "Unsupported public key algorithm\n";

#ifdef TEST
        fprintf(g_logFile, "Signed output\n");
        fprintf(g_logFile, "\tM: "); printNum(*(pRSAKey->m_pbnM)); printf("\n");
        fprintf(g_logFile, "\tD: "); printNum(*(pRSAKey->m_pbnD)); printf("\n");
        fprintf(g_logFile, "\tIn: "); printNum(bnMsg); printf("\n");
        fprintf(g_logFile, "\tOut: "); printNum(bnOut); printf("\n");

        bnum  bnCheck(pRSAKey->m_iByteSizeM/2);
        mpRSAENC(bnOut, *(pRSAKey->m_pbnE), *(pRSAKey->m_pbnM), bnCheck);
        fprintf(g_logFile, "\tCheck: "); printNum(bnCheck); printf("\n"); printf("\n");

#endif

        // write XML to output file
        iWrite= open(szOutFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if(iWrite<0)
            throw "Can't open file to write signed version\n";

        // Header
        if(write(iWrite, szSigHeader, strlen(szSigHeader))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }

        // write SignedInfo
        if(write(iWrite, szToHash, strlen(szToHash))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }

        // write signature valuee
        if(write(iWrite, szSigValueBegin, strlen(szSigValueBegin))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }
        int iOutLen= 1024;
        if(!toBase64(pRSAKey->m_iByteSizeM, (u8*)bnOut.m_pValue, &iOutLen, rgBase64Sig))
            throw "Cant base64 encode signature value\n";
        if(write(iWrite, rgBase64Sig, strlen(rgBase64Sig))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }
        if(write(iWrite, szSigValueEnd, strlen(szSigValueEnd))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }

        // public key info of signer
        szKeyInfo= pRSAKey->SerializePublictoString();
        if(szKeyInfo==NULL)
            throw "Can't Serialize key class\n";
    
        if(write(iWrite, szKeyInfo, strlen(szKeyInfo))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }
        if(write(iWrite, szSigTrailer, strlen(szSigTrailer))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }

#ifdef TEST
        fprintf(g_logFile, "Signature written\n");
#endif
    }
    catch(const char* szError) {
        fRet= false;
        fprintf(g_logFile, "Sign error: %s\n", szError);
    }

    // clean up
    if(iWrite>0)
        close(iWrite);
    if(szKeyInfo!=NULL) {
        free(szKeyInfo);
        szKeyInfo= NULL;
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

bool VerifyXML(const char* szKeyFile, const char* szInFile)
{
    int         iAlgIndex= 0;  // Fix: there's only one now
    Sha256      oHash;
    u8          rgHashValue[SHA256_DIGESTSIZE_BYTES];
    u8          rguDecoded[1024];
    u8          rguOut[1024];
    TiXmlNode*  pNode= NULL;
    TiXmlNode*  pNode1= NULL;
    TiXmlNode*  pNode2= NULL;

    RSAKey*     pRSAKey= NULL;
    char*       szToHash= NULL;
    bool        fRet= true;

#ifdef TEST
    fprintf(g_logFile, "VerifyXML(%s, %s)\n", szKeyFile, szInFile);
#endif
    if(szKeyFile==NULL) {
        fprintf(g_logFile, "No Key file\n");
        return false;
    }
    if(szInFile==NULL) {
        fprintf(g_logFile, "No Input file\n");
        return false;
    }

    try {

        // read input file
        TiXmlDocument signedDoc;
        if(!signedDoc.LoadFile(szInFile)) 
            throw "Can't read signed file\n";

        // SignedInfo
        pNode= Search(signedDoc.RootElement(), "ds:SignedInfo");
        if(pNode==NULL)
            throw "Can't find SignedInfo\n";
        szToHash= canonicalize(pNode);
        if(szToHash==NULL)
            throw "Can't canonicalize\n";

        // hash it
        if(hashAlgfromIndex(iAlgIndex)==SHA256HASH) {
            oHash.Init();
            oHash.Update((byte*) szToHash, strlen(szToHash));
            oHash.Final();
            oHash.GetDigest(rgHashValue);
        }
        else
            throw "Unsupported hash algorithm\n";

#ifdef TEST
            fprintf(g_logFile, "Canonical SignedInfo\n%s\n", szToHash);
            fprintf(g_logFile, "Size hashed: %d\n", (int)strlen(szToHash));
            PrintBytes("Hash", rgHashValue, SHA256_DIGESTSIZE_BYTES);
            fprintf(g_logFile, "\tBytes hashed: %d\n", (int)strlen(szToHash));
#endif

        // key from keyfile
        pRSAKey= (RSAKey*)ReadKeyfromFile(szKeyFile);
        if(pRSAKey==NULL) {
            delete (KeyInfo*) pRSAKey;
            throw "Cant parse or open Keyfile\n";
        }
        if(((KeyInfo*)pRSAKey)->m_ukeyType!=RSAKEYTYPE) {
            delete (KeyInfo*) pRSAKey;
            throw "Wrong key type for signing\n";
        }

        fprintf(g_logFile, "\n");
#ifdef TEST
        pRSAKey->printMe();
#endif
        fprintf(g_logFile, "\n");

        // signature method
        pNode1= Search(signedDoc.RootElement(), "ds:SignatureMethod");
        if(pNode1==NULL)
            throw "Can't find SignatureMethod\n";

        const char* szAlgorithm= ((((TiXmlElement*) pNode1)->Attribute("Algorithm")));
        if(szAlgorithm==NULL)
            throw "Cant get signing algorithm\n";
        iAlgIndex= algorithmIndexFromLongName(szAlgorithm);
        if(iAlgIndex<0)
            throw "Unsupported signing algorithm\n";

        // get SignatureValue
        pNode1= Search(signedDoc.RootElement(), "ds:SignatureValue");
        if(pNode1==NULL)
            throw "Can't find SignatureValue\n";
        pNode2= pNode1->FirstChild();
        if(pNode2==NULL)
            throw "Can't find SignatureValue element\n";

        const char* szBase64Sign= pNode2->Value();
        if(szBase64Sign==NULL)
            throw "Can't get base64 signature value\n";
        int iOutLen= 1024;
        if(!fromBase64(strlen(szBase64Sign), szBase64Sign, &iOutLen, rguDecoded))
            throw "Cant base64 decode signature block\n";

        // decrypt with public key
        bnum    bnMsg(pRSAKey->m_iByteSizeM/2);
        bnum    bnOut(pRSAKey->m_iByteSizeM/2);
    
        if( (pkAlgfromIndex(iAlgIndex)==RSA2048 && pRSAKey->m_iByteSizeM == 256) ||
            (pkAlgfromIndex(iAlgIndex)==RSA1024 && pRSAKey->m_iByteSizeM == 128)) {
            if((pRSAKey->m_iByteSizeM==0) || (pRSAKey->m_iByteSizeE==0))
                throw "Verifying keys not available\n";
            memset(bnMsg.m_pValue, 0, pRSAKey->m_iByteSizeM);
            memset(bnOut.m_pValue, 0, pRSAKey->m_iByteSizeM);
            memcpy(bnMsg.m_pValue, rguDecoded, iOutLen);
    
            if(!mpRSAENC(bnMsg, *(pRSAKey->m_pbnE), *(pRSAKey->m_pbnM), bnOut))
                throw "Can't sign with private key\n";
            revmemcpy(rguOut, (byte*)bnOut.m_pValue, pRSAKey->m_iByteSizeM);
        }
        else 
            throw "Unsupported public key algorithm\n";

        PrintBytes("Decrypted:\n", rguOut, pRSAKey->m_iByteSizeM);

#ifdef TEST
        fprintf(g_logFile, "Decrypted signature\n");
        fprintf(g_logFile, "\tM: "); printNum(*(pRSAKey->m_pbnM)); printf("\n");
        fprintf(g_logFile, "\tE: "); printNum(*(pRSAKey->m_pbnE)); printf("\n");
        fprintf(g_logFile, "\tIn: "); printNum(bnMsg); printf("\n");
        fprintf(g_logFile, "\tOut: "); printNum(bnOut); printf("\n"); printf("\n");
#endif

        // pick alg from index
        if(padAlgfromIndex(iAlgIndex)==PKCSPAD) 
            fRet= emsapkcsverify(SHA256HASH, rgHashValue, pRSAKey->m_iByteSizeM, rguOut);
        else 
            throw "(char*) Unsupported public key algorithm\n";
    }
    catch(const char* szError) {
        fRet= false;
        fprintf(g_logFile, "Verify error: %s\n", szError);
    }

    // clean up
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

const char* g_aikTemplate=
"<ds:SignedInfo>\n" \
"    <ds:CanonicalizationMethod Algorithm=\"http://www.manferdelli.com/2011/Xml/canonicalization/tinyxmlcanonical#\" />\n" \
"    <ds:SignatureMethod Algorithm=\"http://www.manferdelli.com/2011/Xml/algorithms/rsa1024-sha256-pkcspad#\" />\n" \
"    <Certificate Id='%s' version='1'>\n" \
"        <SerialNumber>20110930001</SerialNumber>\n" \
"        <PrincipalType>Hardware</PrincipalType>\n" \
"        <IssuerName>manferdelli.com</IssuerName>\n" \
"        <IssuerID>manferdelli.com</IssuerID>\n" \
"        <ValidityPeriod>\n" \
"            <NotBefore>2011-01-01Z00:00.00</NotBefore>\n" \
"            <NotAfter>2021-01-01Z00:00.00</NotAfter>\n" \
"        </ValidityPeriod>\n" \
"        <SubjectName>//www.manferdelli.com/Keys/attest/0001</SubjectName>\n" \
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

// Cert ID
// Key name
// M
// Subject Key id

// -------------------------------------------------------------------------



bool SignHexModulus(const char* szKeyFile, const char* szInFile, const char* szOutFile)
{
    Sha256      oHash;
    u8          rgHashValue[SHA256_DIGESTSIZE_BYTES];
    u8          rgToSign[512];
    int         iSigSize= 512;
    char        rgBase64Sig[1024];
    int         size= 512;
    char        rgBase64[512];
    TiXmlNode*  pNode= NULL;
    char*       szToHash= NULL;
    RSAKey*     pRSAKey= NULL;
    char*       szKeyInfo= NULL;
    int         iWrite= -1;
    bool        fRet= true;
    char        szSignedInfo[4096];

    fprintf(g_logFile, "SignHexModulus(%s, %s, %s)\n", szKeyFile, szInFile, szOutFile);
    char* modString= readandstoreString(szInFile); 
    if(modString==NULL) {
        fprintf(g_logFile, "Couldn't open modulusfile %s\n", szInFile);
        return false;
    }

    byte    modHex[1024];
    int     modSize=  MyConvertFromHexString(modString, 1024, modHex);
    PrintBytes("\nmodulus\n", modHex, modSize);

    if(szKeyFile==NULL) {
        fprintf(g_logFile, "No Key file\n");
        return false;
    }
    if(szInFile==NULL) {
        fprintf(g_logFile, "No Input file\n");
        return false;
    }
    if(szOutFile==NULL) {
        fprintf(g_logFile, "No Output file\n");
        return false;
    }

    try {

        pRSAKey= (RSAKey*)ReadKeyfromFile(szKeyFile);
        if(pRSAKey==NULL)
            throw "Cant open Keyfile\n";
        if(((KeyInfo*)pRSAKey)->m_ukeyType!=RSAKEYTYPE) {
            delete (KeyInfo*) pRSAKey;
            pRSAKey= NULL;
            throw "Wrong key type for signing\n";
        }

        fprintf(g_logFile, "\n");
#ifdef TEST
        pRSAKey->printMe();
#endif
        fprintf(g_logFile, "\n");

        // construct key XML from modulus
        const char*   szCertid= "www.manferdelli.com/certs/000122";
        const char*   szKeyName= "Gauss-AIK-CERT";
        byte    revmodHex[1024];

        revmemcpy(revmodHex, modHex, modSize);
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
            fprintf(g_logFile, "Can't find SignedInfo\n");
            return false;
        }

        // Canonicalize
        szToHash= canonicalize(pNode);
        if(szToHash==NULL) 
            throw "Can't canonicalize\n";

        // hash it
        oHash.Init();
        oHash.Update((byte*) szToHash , strlen(szToHash));
        oHash.Final();
        oHash.GetDigest(rgHashValue);

        // pad it
        memset(rgToSign, 0, 512);
        if(!emsapkcspad(SHA256HASH, rgHashValue, pRSAKey->m_iByteSizeM, rgToSign)) 
            throw "Padding failure in Signing\n";

        bnum    bnMsg(pRSAKey->m_iByteSizeM/2);
        bnum    bnOut(pRSAKey->m_iByteSizeM/2);

        iSigSize= pRSAKey->m_iByteSizeM;

        // encrypt with private key
        memset(bnMsg.m_pValue, 0, pRSAKey->m_iByteSizeM);
        memset(bnOut.m_pValue, 0, pRSAKey->m_iByteSizeM);

        revmemcpy((byte*)bnMsg.m_pValue, rgToSign, iSigSize);
#ifdef TEST
        PrintBytes("from pad: ", rgToSign, iSigSize);
        PrintBytes("As signed: ", (byte*)bnMsg.m_pValue, iSigSize);
#endif
        if(iSigSize>pRSAKey->m_iByteSizeM) 
            throw "Signing key block too small\n";
        if((pRSAKey->m_iByteSizeM==0) || (pRSAKey->m_iByteSizeD==0))
            throw "Signing keys not available\n";

        if(!mpRSAENC(bnMsg, *(pRSAKey->m_pbnD), *(pRSAKey->m_pbnM), bnOut))
            throw "Can't sign with private key\n";

#ifdef TEST
        fprintf(g_logFile, "Signed output\n");
        fprintf(g_logFile, "\tM: "); printNum(*(pRSAKey->m_pbnM)); printf("\n");
        fprintf(g_logFile, "\tD: "); printNum(*(pRSAKey->m_pbnD)); printf("\n");
        fprintf(g_logFile, "\tIn: "); printNum(bnMsg); printf("\n");
        fprintf(g_logFile, "\tOut: "); printNum(bnOut); printf("\n");

        bnum  bnCheck(pRSAKey->m_iByteSizeM/2);
        mpRSAENC(bnOut, *(pRSAKey->m_pbnE), *(pRSAKey->m_pbnM), bnCheck);
        fprintf(g_logFile, "\tCheck: "); printNum(bnCheck); printf("\n"); printf("\n");
#endif

        // write XML to output file
        iWrite= open(szOutFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if(iWrite<0)
            throw "Can't open file to write signed version\n";

        // Header
        if(write(iWrite, szSigHeader, strlen(szSigHeader))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }

        // write SignedInfo
        if(write(iWrite, szToHash, strlen(szToHash))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }

        // write signature value
        if(write(iWrite, szSigValueBegin, strlen(szSigValueBegin))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }
        int iOutLen= 1024;
        if(!toBase64(pRSAKey->m_iByteSizeM, (u8*)bnOut.m_pValue, &iOutLen, rgBase64Sig))
            throw "Cant base64 encode signature value\n";
        if(write(iWrite, rgBase64Sig, strlen(rgBase64Sig))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }
        if(write(iWrite, szSigValueEnd, strlen(szSigValueEnd))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }

        // public key info of signer
        szKeyInfo= pRSAKey->SerializePublictoString();
        if(szKeyInfo==NULL)
            throw "Can't Serialize key class\n";
    
        if(write(iWrite, szKeyInfo, strlen(szKeyInfo))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }
        if(write(iWrite, szSigTrailer, strlen(szSigTrailer))<0) {
            fprintf(g_logFile, "bad write\n");
            return false;
        }

#ifdef TEST
        fprintf(g_logFile, "Signature written\n");
#endif
    }
    catch(const char* szError) {
        fRet= false;
        fprintf(g_logFile, "Sign error: %s\n", szError);
    }

    // clean up
    if(iWrite>0)
        close(iWrite);
    if(szKeyInfo!=NULL) {
        free(szKeyInfo);
        szKeyInfo= NULL;
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



//ToDo
// Sign and Verifications
int Sign   (const char* szAlgorithm, Data_buffer *digestIn, Data_buffer *sigOut, RSAPrivateKey* privKey)
{
    int         iAlgIndex;
    Sha256      oHash;
    u8          rgHashValue[SHA256_DIGESTSIZE_BYTES]; //SHA256_DIGESTSIZE_BYTES 32
    u8          rgToSign[512];
    char        rgBase64Sig[1024];
    int         iSigSize= 512;
    //char*       szKeyInfo= NULL;
    //int         iWrite= -1;
    bool        fRet= true;

#ifdef TEST_D
    fprintf(g_logFile, "Sign (%d, %d)\n", digestIn->len ,sigOut->len);
    PrintBytes("\t digestIn (bytes)", digestIn->data, digestIn->len);
#endif

    if(szAlgorithm==NULL) {
        return false;
    }
    if(digestIn==NULL) {
        return false;
    }
    if(digestIn->data==NULL) {
        return false;
    }
    if(digestIn->len<=0) {
        return false;
    }
    if(sigOut == NULL) {
        return false;
    } 
    if(sigOut->data == NULL) {
        return false;
    } 
    if(sigOut->len <= 0) {
        return false;
    } 

    if (privKey == NULL) return false;
    if(privKey->n.data == NULL || privKey->n.len <=0 )  {
       	 return false;
    };
    if(privKey->d.data == NULL || privKey->d.len <=0 )  {
       	 return false;
    };

    // Signature algorithm
    iAlgIndex= algorithmIndexFromShortName(szAlgorithm);
    if(iAlgIndex<0)  {
        //Unsupported signing algorithm
        return false;
    }
    // Print the Keys for debugging - TodO

    char* szAlgLongName= longAlgNameFromIndex(iAlgIndex);
    if(szAlgLongName==NULL) {
        //Can't find Algorithm index
        return false;
    }

    u8      szToHash[2048];
    u8     *sCurPtr;
    sCurPtr= digestIn->data;
    //SHA256_BLOCKSIZE_BYTES
    int     dataLeft= digestIn->len;
    // read, hash, and copy bytes
    if(hashAlgfromIndex(iAlgIndex)==SHA256HASH) {
            oHash.Init();
    } else  //Unsupported hash algorithm
            return false;

    while(dataLeft>SHA256_BLOCKSIZE_BYTES) {
		memcpy(szToHash,sCurPtr,SHA256_BLOCKSIZE_BYTES);
			oHash.Update(szToHash, SHA256_BLOCKSIZE_BYTES);
			
		sCurPtr+= SHA256_BLOCKSIZE_BYTES;
			dataLeft-= SHA256_BLOCKSIZE_BYTES;
    }
    
    memcpy(szToHash,sCurPtr,dataLeft);
    oHash.Update(szToHash , dataLeft);
    oHash.Final();
    oHash.GetDigest(rgHashValue);


    // pad it
    if(padAlgfromIndex(iAlgIndex)==PKCSPAD) {
        memset(rgToSign, 0, 512);
        if(!emsapkcspad(SHA256HASH, rgHashValue,privKey->n.len , rgToSign)) { // n.len is sig len
		//Padding failure in Signing
		// The length of rgHashValue depends on the type
		return false;
	}
     }
     else 	//Unsupported hash algorithm
		return false;
		
	Data_buffer dataHash;
	dataHash.data = rgToSign;
	dataHash.len = privKey->n.len;
	
	fRet = RSADecrypt("RSA_NO_PADDING", &dataHash, sigOut, privKey);
	

    return fRet;
}

int Verify (const char* szAlgorithm, Data_buffer *digestIn, Data_buffer *sigIn, RSAPublicKey* pubKey)
{
    int         iAlgIndex= 0;  //For now only SHA256 
    Sha256      oHash;
    u8          rgHashValue[SHA256_DIGESTSIZE_BYTES];
    //u8          rguDecoded[1024];
    u8          rguOut[1024];

    bool        fRet= true;

#ifdef TEST_D
    fprintf(g_logFile, "Verify(digestIn->len-%d, sigIn->len-%d)\n",digestIn->len , sigIn->len);
    PrintBytes("\tRead Signature (bytes)", (byte*)sigIn->data, sigIn->len);
#endif

    if(szAlgorithm==NULL) {
        return false;
    }
    if(digestIn==NULL) {
        return false;
    }
    if(digestIn->data==NULL) {
        return false;
    }
    if(digestIn->len<=0) {
        return false;
    }
    if(sigIn == NULL) {
        return false;
    } 
    if(sigIn->data == NULL) {
        return false;
    } 
    if(sigIn->len <= 0) {
        return false;
    } 

    if (pubKey == NULL) return false;
    if(pubKey->n.data == NULL || pubKey->n.len <=0 )  {
       	 return false;
    };
    if(pubKey->e.data == NULL || pubKey->e.len <=0 )  {
       	 return false;
    };

    // Signature algorithm
    iAlgIndex= algorithmIndexFromShortName(szAlgorithm);
    if(iAlgIndex<0)  {
        //Unsupported signing algorithm
        return false;
    }
    // Print the Keys for debugging - TodO

    //This is needed when signing only.
    char* szAlgLongName= longAlgNameFromIndex(iAlgIndex);
    if(szAlgLongName==NULL) {
        //Can't find Algorithm index
        return false;
    }

    u8      szToHash[2048];
    u8     *sCurPtr;
    sCurPtr= digestIn->data;
    //SHA256_BLOCKSIZE_BYTES
    int     dataLeft= digestIn->len;
    // read, hash, and copy bytes
    if(hashAlgfromIndex(iAlgIndex)==SHA256HASH) {
            oHash.Init();
    } else  //Unsupported hash algorithm
            return false;

    while(dataLeft>SHA256_BLOCKSIZE_BYTES) {
		memcpy(szToHash,sCurPtr,SHA256_BLOCKSIZE_BYTES);
			oHash.Update(szToHash, SHA256_BLOCKSIZE_BYTES);
		sCurPtr+= SHA256_BLOCKSIZE_BYTES;
			dataLeft-= SHA256_BLOCKSIZE_BYTES;
    }
    memcpy(szToHash,sCurPtr,dataLeft);
    oHash.Update(szToHash , dataLeft);
    oHash.Final();
    oHash.GetDigest(rgHashValue);

#ifdef TEST_D
     
      PrintBytes("Calculated SHA256 Hash Value\n", rgHashValue, SHA256_DIGESTSIZE_BYTES);
#endif


	Data_buffer decSign;
	decSign.data = rguOut;
	decSign.len = pubKey->n.len;
	
	RSAEncrypt("RSA_NO_PADDING", sigIn, &decSign, pubKey);
	

	// pick alg from index
	if(padAlgfromIndex(iAlgIndex)==PKCSPAD) {
	   fRet= emsapkcsverify(SHA256HASH, rgHashValue, pubKey->n.len, rguOut);
    }
	else // Unsupported public key algorithm
	   return false;

	return fRet;
}

int SignFile   (const char* szAlgorithm, const char* szInFile, Data_buffer *sigOut, RSAPrivateKey* privKey)
{
    int         iAlgIndex;
    Sha256      oHash;
    u8          rgHashValue[SHA256_DIGESTSIZE_BYTES]; //SHA256_DIGESTSIZE_BYTES 32
    u8          rgToSign[512];
    char        rgBase64Sig[1024];
    int         iSigSize= 512;
    //char*       szKeyInfo= NULL;
    //int         iWrite= -1;
    bool        fRet= true;

    if(szAlgorithm==NULL) {
        return false;
    }
    if(szInFile==NULL) {
        return false;
    }
    if(sigOut == NULL) {
        return false;
    } 
    if(sigOut->data == NULL) {
        return false;
    } 
    if(sigOut->len <= 0) {
        return false;
    } 

    if (privKey == NULL) return false;
    if(privKey->n.data == NULL || privKey->n.len <=0 )  {
       	 return false;
    };
    if(privKey->d.data == NULL || privKey->d.len <=0 )  {
       	 return false;
    };
#ifdef TEST_D
    fprintf(g_logFile, "SignFile (%s, %s)\n", szAlgorithm ,szInFile);
#endif

    // Signature algorithm
    iAlgIndex= algorithmIndexFromShortName(szAlgorithm);
    if(iAlgIndex<0)  {
        //Unsupported signing algorithm
        return false;
    }
    // Print the Keys for debugging - TodO

    char* szAlgLongName= longAlgNameFromIndex(iAlgIndex);
    if(szAlgLongName==NULL) {
        //Can't find Algorithm index
        return false;
    }

    // Get File size
    struct stat statBlock;
    if(stat(szInFile, &statBlock)<0) {
        return false;
    }
    int fileSize= statBlock.st_size;

    int iRead= open(szInFile, O_RDONLY);
    if(iRead<0) {
        return false;
    }

    u8      szToHash[2048];
    int     fileLeft= fileSize;
    // read, hash, and copy bytes
    if(hashAlgfromIndex(iAlgIndex)==SHA256HASH) {
            oHash.Init();
    } else  //Unsupported hash algorithm
            return false;

    while(fileLeft>1024) {
        if(read(iRead, szToHash, 1024)<0) {
            return false;
        }
        oHash.Update(szToHash, 1024);
        fileLeft-= 1024;
    }

    // final block
    if(read(iRead, szToHash, fileLeft)<0) {
        return false;
    }

    oHash.Update(szToHash , fileLeft);
    oHash.Final();
    oHash.GetDigest(rgHashValue);

#ifdef TEST_D
      //PrintBytes("SHA256 Hash Value\n", rgHashValue, privKey->n.len);
      PrintBytes("SHA256 Hash Value\n", rgHashValue, SHA256_DIGESTSIZE_BYTES);
#endif
    // pad it
    if(padAlgfromIndex(iAlgIndex)==PKCSPAD) {
        memset(rgToSign, 0, 512);
        if(!emsapkcspad(SHA256HASH, rgHashValue,privKey->n.len , rgToSign)) { // n.len is sig len
		//Padding failure in Signing
		// The length of rgHashValue depends on the type
		return false;
	}
     }
     else 	//Unsupported hash algorithm
		return false;

#ifdef TEST_D
     //PrintBytes("Padded block\n", rgSig, sigSize);
      PrintBytes("Padded block up to sig size\n", rgToSign, privKey->n.len);
#endif

        bnum    bnMsg(privKey->n.len/2);
        bnum    bnOut(privKey->n.len/2);

	bnum    pbnM(privKey->n.len/sizeof(u64)+1);
	bnum    pbnD(privKey->d.len/sizeof(u64)+1);

        iSigSize= privKey->n.len;
        // encrypt with private key
        if( (pkAlgfromIndex(iAlgIndex)==RSA2048 && privKey->n.len == 256) ||
            (pkAlgfromIndex(iAlgIndex)==RSA1024 && privKey->n.len == 128)) {

            memset(bnMsg.m_pValue, 0, privKey->n.len);
            memset(bnOut.m_pValue, 0, privKey->n.len);

            revmemcpy((byte*)bnMsg.m_pValue, rgToSign, iSigSize);
            //if(iSigSize > privKey->n.len)  
            if((unsigned) iSigSize > sigOut->len)  {
                //Signing key block too small;
		return false;
	    }
            if((privKey->n.len==0) || (privKey->n.len==0))
	    {
                //Signing keys not available;
		return false;
	    }

	    memcpy((u8 *)pbnM.m_pValue, privKey->n.data ,privKey->n.len);
    	    memcpy((u8 *)pbnD.m_pValue, privKey->d.data ,privKey->d.len);

            if(!mpRSAENC(bnMsg, pbnD,pbnM, bnOut)) {
                //Can't sign with private key
		return false;
	     }
        }
        else  //Unsupported public key algorithm	
		return false;

	//padded and then ecoded
	memcpy(sigOut->data,(u8 *)bnOut.m_pValue, privKey->n.len);
	sigOut->len= privKey->n.len;

#ifdef TEST_D
        fprintf(g_logFile, "Signed output\n");
        fprintf(g_logFile, "\tM: "); printNum(pbnM); printf("\n");
        fprintf(g_logFile, "\tD: "); printNum(pbnD); printf("\n");
        fprintf(g_logFile, "\tIn: "); printNum(bnMsg); printf("\n");
        fprintf(g_logFile, "\tOut: "); printNum(bnOut); printf("\n");
        PrintBytes("Out in bytes\n", (u8 *)bnOut.m_pValue, privKey->n.len);
#endif
        int iOutLen= 1024;
        // The following is fo future
        if(!toBase64(privKey->n.len, (u8*)bnOut.m_pValue, &iOutLen, rgBase64Sig))
	{
            //Cant base64 encode signature value
		return false;
	}

#ifdef TEST_D
	printf("iOutLen %d\n", iOutLen);
	printf("strlen(rgBase64Sig)  %d\n", strlen(rgBase64Sig));
	printf("rgBase64Sig\n"); printf("%s\n",rgBase64Sig);
#endif
    

#ifdef TEST_D
    fprintf(g_logFile, "Signing done\n");
#endif
    close(iRead);
    return fRet;
}

int VerifyFile (const char* szAlgorithm, const char* szInFile, Data_buffer *sigIn, RSAPublicKey* pubKey)
{
    int         iAlgIndex= 0;  //For now only SHA256 
    Sha256      oHash;
    u8          rgHashValue[SHA256_DIGESTSIZE_BYTES];
    //u8          rguDecoded[1024];
    u8          rguOut[1024];

    bool        fRet= true;

    if(szAlgorithm==NULL) {
        return false;
    }
    if(szInFile==NULL) {
        return false;
    }
    if(sigIn == NULL) {
        return false;
    } 
    if(sigIn->data == NULL) {
        return false;
    } 
    if(sigIn->len <= 0) {
        return false;
    } 

    if (pubKey == NULL) return false;
    if(pubKey->n.data == NULL || pubKey->n.len <=0 )  {
       	 return false;
    };
    if(pubKey->e.data == NULL || pubKey->e.len <=0 )  {
       	 return false;
    };

    // Signature algorithm
#ifdef TEST_D
    fprintf(g_logFile, "VerifyFile(%s, %s)\n", szAlgorithm, szInFile);
    PrintBytes("\tRead Signature (bytes)", (byte*)sigIn->data, sigIn->len);
#endif

    // Signature algorithm
    iAlgIndex= algorithmIndexFromShortName(szAlgorithm);
    if(iAlgIndex<0)  {
        //Unsupported signing algorithm
        return false;
    }
    // Print the Keys for debugging - TodO

    //This is needed when signing only.
    char* szAlgLongName= longAlgNameFromIndex(iAlgIndex);
    if(szAlgLongName==NULL) {
        //Can't find Algorithm index
        return false;
    }

    // Get File size
    struct stat statBlock;
    if(stat(szInFile, &statBlock)<0) {
        return false;
    }
    int fileSize= statBlock.st_size;

    int iRead= open(szInFile, O_RDONLY);
    if(iRead<0) {
        return false;
    }

    u8      szToHash[2048];
    int     fileLeft= fileSize;
    // read, hash, and copy bytes

    if(hashAlgfromIndex(iAlgIndex)==SHA256HASH) {
            oHash.Init();
    } else  //Unsupported hash algorithm
            return false;

    while(fileLeft>1024) {
        if(read(iRead, szToHash, 1024)<0) {
            return false;
        }
        oHash.Update(szToHash, 1024);
        fileLeft-= 1024;
    }

    // final block
    if(read(iRead, szToHash, fileLeft)<0) {
        return false;
    }

    oHash.Update(szToHash , fileLeft);
    oHash.Final();
    oHash.GetDigest(rgHashValue);

#ifdef TEST_D
      //PrintBytes("SHA256 Hash Value\n", rgHashValue, privKey->n.len);
      PrintBytes("Calculated SHA256 Hash Value\n", rgHashValue, SHA256_DIGESTSIZE_BYTES);
#endif
    // Decrypt it
    bnum    bnMsg(pubKey->n.len/2);
    bnum    bnOut(pubKey->n.len/2);

   // Decrypt with public key
    bnum    pbnM(pubKey->n.len/sizeof(u64)+1);
    bnum    pbnE(pubKey->e.len/sizeof(u64)+1);

   if( (pkAlgfromIndex(iAlgIndex)==RSA2048 && pubKey->n.len == 256) ||
       (pkAlgfromIndex(iAlgIndex)==RSA1024 && pubKey->n.len == 128)) {

       memset(bnMsg.m_pValue, 0, pubKey->n.len);
       memset(bnOut.m_pValue, 0, pubKey->n.len);
       //memcpy(bnMsg.m_pValue, rguDecoded, iOutLen);
       memcpy((u8 *)bnMsg.m_pValue, sigIn->data, sigIn->len);

       memcpy((u8 *)pbnM.m_pValue, pubKey->n.data, pubKey->n.len);
       memcpy((u8 *)pbnE.m_pValue, pubKey->e.data, pubKey->e.len);

       if(!mpRSAENC(bnMsg, pbnE,pbnM, bnOut)) {
         //Can't sign with private key
          return false;
	}
	revmemcpy(rguOut, (byte*)bnOut.m_pValue, pubKey->n.len);

     }
     else  //Unsupported public key algorithm	
	return false;
	
#ifdef TEST_D
       PrintBytes("Decrypted (rgOut - rev):\n", rguOut, pubKey->n.len);
       //PrintBytes("Decrypted:\n", (u8 *)bnOut.m_pValue, pubKey->n.len);
       fprintf(g_logFile, "Decrypted signature\n");
       fprintf(g_logFile, "\tM: "); printNum(pbnM); printf("\n");
       fprintf(g_logFile, "\tE: "); printNum(pbnE); printf("\n");
       fprintf(g_logFile, "\tIn: "); printNum(bnMsg); printf("\n");
       fprintf(g_logFile, "\tOut: "); printNum(bnOut); printf("\n"); printf("\n");
       PrintBytes("\tOut(bytes))", (byte*)bnOut.m_pValue, pubKey->n.len);
#endif

        // pick alg from index
       if(padAlgfromIndex(iAlgIndex)==PKCSPAD) 
           fRet= emsapkcsverify(SHA256HASH, rgHashValue, pubKey->n.len, rguOut);
       else // Unsupported public key algorithm
           return false;
    
        return fRet;
}


int InitCryptoRand() {

int iRet;
    iRet= initCryptoRand();
    return iRet;
}
int GetCryptoRandom(int iNumBits, byte* buf)
{
int iRet;
    //if( iNumBits < 128) return false;
    if( iNumBits%8 !=0 ) return false;
    if(buf==NULL) return false;
    iRet= getCryptoRandom(iNumBits,buf);
    return iRet;
}
int CloseCryptoRand()
{
int iRet;
    iRet= closeCryptoRand();
    return iRet;

}


int RPLibInit()
{
bool fRet;
    fRet= initLog("rp.log");
    if (fRet==true) return 1; else return 0;
}
int RPLibExit()
{
    closeLog();
    return 1;
}

int GetPublicKeyFromCert(const char* szCertificate, RSAPublicKey* pubKey) {

int iRet;
    iRet= -99;
    return iRet;
}

// -------------------------------------------------------------------------
#if 0
//ToDo
int EstablishSessionKey (const char* szRemoteMachine, const char* method, int* nuKeysOut, Data_buffer **keys)
{

	int iRet;

	iRet= -99;
	return iRet;
}

//int doTrustOp(int opcode, unsigned int* pType, int inSize, byte* inData, int* poutSize, byte* outData);

int doTrustOpX(int opcode, int inSize, byte* inData, int* poutSize, byte* outData) {

        int fd1;
        int rc = -1;
        int i = 0;
        int size= 0;
        byte rgBuf[PARAMSIZE] = {0};
        tcBuffer* pReq= (tcBuffer *)rgBuf;
        
        fd1 = open("/dev/chandu", O_RDWR); 
        if(fd1 < 0)
        {
            fprintf(stdout, "Open error chandu: %s\n", strerror(errno));
            return -10;
        }

       size = sizeof(tcBuffer);
       memset(outData, 0, *poutSize);
	switch (opcode){
	case VM2RP_ATTESTFOR:
            size = encodeVM2RP_ATTESTFOR(inSize, inData, PARAMSIZE - size, &rgBuf[size]);
             break;
	case VM2RP_SEALFOR:
            size = encodeVM2RP_SEALFOR(inSize, inData, PARAMSIZE - size, &rgBuf[size]);
            break;
	case VM2RP_UNSEALFOR:
	    PrintBytes(" Input param checking before encode", (byte*)inData, inSize);
            size = encodeVM2RP_UNSEALFOR(inSize, inData, PARAMSIZE - size, &rgBuf[size]);
            break;
	default:
            size= -1;
            break;
	}

       if(size < 0) {
            fprintf(stdout, "getQuote: encodeVM2RP_ATTESTFOR failed\n");
            close (fd1);
            return -11;
       }
       pReq->m_procid= 0;
       pReq->m_reqID= opcode;
       pReq->m_ustatus= 0;
       pReq->m_origprocid= 0;
       pReq->m_reqSize= size;

       rc = write(fd1, rgBuf, size + sizeof(tcBuffer) );
       if (rc < 0){
           fprintf(stdout, "write error: %s\n", strerror(errno));
           goto fail;
       }
again:
       rc = read(fd1, rgBuf, sizeof(rgBuf));
       if (rc < 0){
           if (errno == 11) {
               sleep(1);
               goto again;
           }

           fprintf(stdout, "read error:%d  %s\n", errno, strerror(errno));
           goto fail;
       }
       size = rc;
       size-= sizeof(tcBuffer);
       switch (opcode){
            case VM2RP_ATTESTFOR:
                    rc = decodeRP2VM_ATTESTFOR(poutSize, outData, &rgBuf[sizeof(tcBuffer)]);
                    break;
            case VM2RP_SEALFOR:
                    rc = decodeRP2VM_SEALFOR(poutSize, outData, &rgBuf[sizeof(tcBuffer)]);
                    break;
            case VM2RP_UNSEALFOR:
                    rc = decodeRP2VM_UNSEALFOR(poutSize, outData, &rgBuf[sizeof(tcBuffer)]);
                    break;
	    default:
                rc= 0;
                break;
       }

       if(!rc) {
           fprintf(stdout, "getQuote: gettcBuf for decodeRP2VM_ATTESTFORfailed\n");
           close (fd1);
           return -12;
       }
       size = *poutSize;
fail:
       if ( fd1 >= 0)
           close (fd1);
	return rc;
}

int Seal(Data_buffer * secretIn, Data_buffer *dataOut)
{
	int iRet;
        u32 uType = 0;
	if((secretIn == NULL) || (dataOut == NULL)) {
            return -1;
	}
	if((secretIn->len <= 0) || (dataOut->len <= 0))  {
            return -2;
        }

	iRet= doTrustOp(VM2RP_SEALFOR, &uType, secretIn->len, secretIn->data,
                         (int *)&dataOut->len, dataOut->data);
	return iRet;
}

int UnSeal(Data_buffer *dataIn, Data_buffer *secretOut)
{
	int iRet;
        u32 uType = 0;

	if((dataIn == NULL) || (secretOut == NULL)) {
            return -1;
	}
	if((dataIn->len <= 0) || (secretOut->len <= 0))  {
            return -2;
        }
	iRet= doTrustOp(VM2RP_UNSEALFOR, &uType, dataIn->len, dataIn->data,
                        (int *)&secretOut->len, secretOut->data);
	return iRet;
}


int Attest(Data_buffer *dataIn, Data_buffer *dataOut)
{
	int iRet;
        u32 uType = 0;

	if((dataIn == NULL) || (dataOut == NULL)) {
            return -1;
	}
	if((dataIn->len <= 0) || (dataOut->len <=0))  {
            return -2;
        }
	iRet= doTrustOp(VM2RP_ATTESTFOR, &uType, dataIn->len, dataIn->data,
                        (int *)&dataOut->len, dataOut->data);
	return iRet;
}

int GenerateQuote(Data_buffer *nonce, Data_buffer * challengeIn, Data_buffer *quoteOut)
{
    //int sizenonce;
    //byte     nonce[20];
    if((challengeIn == NULL) || (quoteOut == NULL) || (nonce == NULL)) {
        return -1;
    }

    // Nonce can be of size 0.
    if((challengeIn->len <= 0) || (quoteOut->len <= 0) || (nonce->len < 0))  {
        return -2;
    }
    // challengeIn->data[challengeIn->len]=0;
    // The passed in data  was basically a constant string
    // The above results in SEGV
    bool fRet = GenQuote(nonce->len, nonce->data, (char *)challengeIn->data, (char **)&quoteOut->data);

    return fRet;
}

int VerifyQuote( Data_buffer * quoteIn,  const char* szCertificate, int iCertFlag)
{
#ifdef TEST
    printf("VerifyQuote entered\n");
#endif
    if((quoteIn == NULL) || (szCertificate == NULL)) {
        return -1;
    }
    if (quoteIn->len <= 0)  {
        return -2;
    }


#ifdef TEST
        printf("Cert Flag is %d\n",iCertFlag);
#endif
    if (iCertFlag) {
    // Placeholder to get the Key out of Certificate.
    //
#ifdef TEST
        printf("Currently can only accept the Public Key instead of the ceritificate\n");
#endif
        return -3;
    }

    bool fRet = VerifyQuote((char*)quoteIn->data, (char *)szCertificate);
#ifdef TEST
    printf("Returning VerifyQuote %d\n",fRet);
#endif
    return fRet;
}
#endif

// -------------------------------------------------------------------------

