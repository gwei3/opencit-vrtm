//
//	
//  File: rpapicryputil.cpp
//	Author: 
//
//  Description: rp crypto util tool.
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

//Fixes 
// Date		Bugs       Comment
// Sep 12       15 bugs    Just for testing so not a big deal. Backported.
//
#include "rpapi.h"
#include "rpapicryputil.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#define NOACTION                   0
#define GENKEYFILE                 1
#define GENSYMKEY                  2 
#define GENRSAKEY                  3 
#define READAESKEY                 4 
#define READRSAKEY                 5 
#define SIGNXML                    6 
#define VERIFYXML                  7 
#define ENCRYPTFILE                8 
#define DECRYPTFILE                9 
#define ENCRYPT                   10
#define DECRYPT                   11
#define RSAENCRYPT                12
#define RSADECRYPT                13
#define DIGEST                    14
#define HMAC                      15
#define SIGN                      16
#define VERIFY                    17
#define SIGNFILE                  18
#define VERIFYFILE                19
#define PKDF               	  20 
#define BASE64               	  21
#define RANDOM               	  22
#define BASE64RANDOM          	  23
#define SEAL               	  24
#define UNSEAL               	  25
#define ATTEST          	  26


// --------------------------------------------------------------------- 
extern FILE*   g_logFile;
bool  ReadRSAKeyfromFile(const char* szKeyFile, RSAKeyPair*  pRSAKey);
bool  ReadAESKeyfromFile(const char* szKeyFile, SymKey*  pSymKey);
bool  ReadRSAKeyfromFile(const char* szKeyFile, SymKey*  pSymKey);
void  PrintBytes(const char* szMsg, byte* pbData, int iSize, int col=32);
int   GetSha256Digest(Data_buffer* dataIn,Data_buffer* dataOut);
int   GetSha1Digest(Data_buffer* dataIn,Data_buffer* dataOut);
bool  SignXML(const char* szKeyFile, const char* szAlgorithm, const char* szInFile, const char* szOutFile);
bool  VerifyXML(const char* szKeyFile, const char* szInFile);
int   SignFile1   (const char* szAlgorithm, const char* szInFile, Data_buffer *sigOut, RSAPrivateKey* privKey, RSAPublicKey *pubKey);

bool  toBase64(int iInLen, const unsigned char* puIn, int* piOutLen, char* rgszOut, bool fDirFwd=true);
bool  fromBase64(int iInLen, const char* pszIn, int* piOutLen, unsigned char* puOut , bool fDirFwd=true);
bool  getBase64Rand(int iBytes, byte* puR, int* pOutSize, char* szOut);


bool  EncryptSym(u32 op, const char* szKeyFile,  const char* szIntKeyFile, Data_buffer* dataIn , Data_buffer* dataOut, const char* szAlgo, const char* szMode, const char* szPad,const char* szHmac)


{

u32	uAlgo= NOALG;
u32	uMode= NOMODE;
u32	uPad= NOPAD;
u32	uHmac= NOHMAC;
int iRet;

    printf( "In EncryptSym\n");
    printf( "szKeyFile=%s\n",szKeyFile);
    printf( "szIntKeyFile=%s\n",szIntKeyFile);
    printf( "szMode=%s\n",szMode);
    printf( "szAlgo=%s\n",szAlgo);
    printf( "szPad=%s\n",szPad);
    printf( "szHmac=%s\n",szHmac);

	if(strcmp(szAlgo, "AES128")==0) {
		uAlgo = AES128;
	} 
    printf( "uAlgo=%d\n",uAlgo);
	if(strcmp(szAlgo, "AES256")==0) {
		uAlgo = AES256;
	}
	if (uAlgo == NOALG) {
		//iRet = -1;  
		return false;
	}


	if (uAlgo == AES128 || uAlgo == AES256) { 
		if(strcmp(szMode, "CBCMODE")==0) {
			uMode = CBCMODE;
		} 
		if(strcmp(szMode, "GCMMODE")==0) {
			uMode = GCMMODE;
		} 
	}
    printf( "uMode=%d\n",uMode);
	if (uMode == NOMODE) {
		iRet = -2;  
		//return iRet;
		return false;
	}

	if(strcmp(szPad, "PKCSPAD")==0) {
		uPad = PKCSPAD;
	} 
	if(strcmp(szPad, "SYMPAD")==0) {
		uPad = SYMPAD;
	} 
    printf( "uPad=%d\n",uPad);
	if (uPad == NOPAD) {
		iRet = -3; 
		//return iRet;
		return false;

	}


	if(strcmp(szHmac, "HMACSHA256")==0) {
		uHmac = HMACSHA256;
	} 
    printf( "uMac=%d\n",uHmac);
	if (uHmac == NOHMAC) {
		iRet = -4;  
		//return iRet;
		return false;
	}
	
    printf( "uAlgo=%d uMode=%d uPad=%d uHmac=%d\n",uAlgo,uMode,uPad,uHmac);
    
    u8   buf[32];
    SymKey symKeyIn;
    symKeyIn.data= buf;
    symKeyIn.len= 32;
    iRet=  ReadAESKeyfromFile(szKeyFile,&symKeyIn);
    if(iRet == 0) {
        printf( "Key Reading failed\n");
        return 0;
    }
    printf( "symKeyIn.len=%d\n",symKeyIn.len);
    PrintBytes("symKeyIn.data=",symKeyIn.data,symKeyIn.len);

    u8   bufIntKey[32];
    SymKey intKey;
    intKey.data= bufIntKey;
    intKey.len= 32;
    iRet=  ReadAESKeyfromFile(szIntKeyFile,&intKey);
    if(iRet == 0) {
        printf( "Int Key Reading failed\n");
        return 0;
    }
    printf( "intKey.len=%d\n",intKey.len);
    PrintBytes("intKey.data=",intKey.data,intKey.len);


	switch(op) {
		case ENCRYPT:
 			iRet= Encrypt(&symKeyIn, &intKey, dataIn, dataOut, uMode,uAlgo, uPad,uHmac);
			break;
		case DECRYPT:
 			iRet= Decrypt(&symKeyIn, &intKey, dataIn, dataOut, uMode, uAlgo, uPad,uHmac);
			break;
		default:
			printf( "Unknown Encrypt/Decrypt option\n");
			break;
	}
    return iRet;
	
}

int EncryptSymFile(u32 op, const char* szKeyFile,const char* szIntKeyFile, const char* szInFile, const char* szOutFile,
         const char* szAlgo, const char* szMode, const char* szPad,const char* szHmac)
{

u32	uAlgo= NOALG;
u32	uMode= NOMODE;
u32	uPad= NOPAD;
u32	uHmac= NOHMAC;
int     iRet;

    printf( "In EncryptSymFile\n");
    printf( "szKeyFile=%s\n",szKeyFile);
    printf( "szInFile=%s\n",szInFile);
    printf( "szOutFile=%s\n",szOutFile);
    printf( "szMode=%s\n",szMode);
    printf( "szAlgo=%s\n",szAlgo);
    printf( "szPad=%s\n",szPad);
    printf( "szHmac=%s\n",szHmac);

	if(strcmp(szAlgo, "AES128")==0) {
		uAlgo = AES128;
	} 
	if(strcmp(szAlgo, "AES256")==0) {
		uAlgo = AES256;
	}
    printf( "uAlgo=%d\n",uAlgo);
	if (uAlgo == NOALG) {
		iRet = -1; 
		return iRet;
	}

	if (uAlgo == AES128 || uAlgo == AES256) { 
		if(strcmp(szMode, "CBCMODE")==0) {
			uMode = CBCMODE;
		} 
		if(strcmp(szMode, "GCMMODE")==0) {
			uMode = GCMMODE;
		} 
	}
    printf( "uMode=%d\n",uMode);
	if (uMode == NOMODE) {
		iRet = -2;  
		return iRet;
	}


	if(strcmp(szPad, "PKCSPAD")==0) {
		uPad = PKCSPAD;
	} 
	if(strcmp(szPad, "SYMPAD")==0) {
		uPad = SYMPAD;
	} 
    printf( "uPad=%d\n",uPad);
	if (uPad == NOPAD) {
		iRet = -3; 
		return iRet;

	}

	if(strcmp(szHmac, "HMACSHA256")==0) {
		uHmac = HMACSHA256;
	} 
    printf( "uMac=%d\n",uHmac);
	if (uHmac == NOHMAC) {
		iRet = -4;  
		return iRet;
	}

    printf( "uAlgo=%d uMode=%d uPad=%d uHmac=%d\n",uAlgo,uMode,uPad,uHmac);

    u8   buf[32];
    SymKey symKeyIn;
    symKeyIn.data= buf;
    symKeyIn.len= 32;
    iRet=  ReadAESKeyfromFile(szKeyFile,&symKeyIn);
    if(iRet == 0) {
        printf( "Key Reading failed\n");
        return 0;
    }
    printf( "symKeyIn.len=%d\n",symKeyIn.len);
    PrintBytes("symKeyIn.data=",symKeyIn.data,symKeyIn.len);

    u8   bufIntKey[32];
    SymKey intKey;
    intKey.data= bufIntKey;
    intKey.len= 32;
    iRet=  ReadAESKeyfromFile(szIntKeyFile,&intKey);
    if(iRet == 0) {
        printf( "Int Key Reading failed\n");
        return 0;
    }
    printf( "intKey.len=%d\n",intKey.len);
    PrintBytes("intKey.data=",intKey.data,intKey.len);

    switch(op) {
    	case ENCRYPTFILE:
 	    	iRet= EncryptFile(&symKeyIn, &intKey, szInFile, szOutFile, uMode, uAlgo, uPad,uHmac);
	    	break;
	    case DECRYPTFILE:
 	    	iRet= DecryptFile(&symKeyIn, &intKey, szInFile, szOutFile, uMode, uAlgo, uPad,uHmac);
	    	break;
	    default:
	    	printf( "Unknown Encrypt/Decrypt option\n");
	    	break;
    }
	
return iRet;
}

void PrintBytes_LR(const char* szMsg, byte* pbData, int iSize, int col=32)
{
    int i;

    printf("%s", szMsg);
    for (i= 0; i<iSize; i++) {
        printf("%02x", pbData[i]);
        //if((i%col)==(col-1))
         //   printf("\n");
        }
    printf("\n");
}

//Start...

void GenKeyFileIfc (const char*   szKeyType, const char*   szOutFile ) {
	int    iRet;
	InitCryptoRand();
	InitBigNum();
	iRet= GenKeyFile(szKeyType, szOutFile);
	if(iRet==1)
		printf( "%s: SUCCESSFUL\n", __FUNCTION__);
	else
		printf( "%s: FAILED\n", __FUNCTION__);
	CloseCryptoRand();
}

void GenSymKeyIfc (const char* szKeyType) {
	int    iRet;
    const char* reserved=NULL;
    u8              buf[32];
    
 
    SymKey symKeyOut;
    symKeyOut.data= buf;
    symKeyOut.len= 32; 

	InitCryptoRand();
	InitBigNum();

	iRet= GenSymKey(szKeyType, reserved, &symKeyOut);
	if(iRet==1) {
		char szBuf[512]; 
		int outlen = 512;
		
		toBase64 (symKeyOut.len, symKeyOut.data, &outlen, szBuf);
		printf( "OUT:%s\n", szBuf);
	}
	else
		printf( "%s: FAILED\n", __FUNCTION__);
        
    CloseCryptoRand();
    
}

void GenRSAKeyIfc(const char* szKeyType) {
	int    iRet;
	const char* reserved=NULL;
    u64 pubExp = 0;
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

  
	InitCryptoRand();
	InitBigNum();

	iRet= GenRSAKey(szKeyType, reserved, pubExp, &rsaKeyOut);
	if(iRet==1) {
		printf( "%s->SUCCESSFUL\n", __FUNCTION__);
		PrintBytes_LR(":n->", rsaKeyOut.n.data,rsaKeyOut.n.len);
		PrintBytes_LR(":e->", rsaKeyOut.e.data,rsaKeyOut.e.len);
		PrintBytes_LR(":d->", rsaKeyOut.d.data,rsaKeyOut.d.len);
		PrintBytes_LR(":p->", rsaKeyOut.p.data,rsaKeyOut.p.len);
		PrintBytes_LR(":q->", rsaKeyOut.q.data,rsaKeyOut.q.len);
	}
	else
		printf( "%s: FAILED\n", __FUNCTION__);

	CloseCryptoRand();

}

void ReadAESKeyFromFileIfc (const char* szKeyFile){
	int    iRet;
	u8              buf[32];
	SymKey symKeyOut;
	symKeyOut.data= buf;
	symKeyOut.len= 32;

	iRet= ReadAESKeyfromFile(szKeyFile, &symKeyOut);
	if(iRet==1) {
		char szBuf[512]; 
		int outlen = 512;
		
		toBase64 (symKeyOut.len, symKeyOut.data, &outlen, szBuf);
		printf( "OUT:%s\n", szBuf);	
	}
	else
		printf( "%s: FAILED\n", __FUNCTION__);
}

void ReadRSAKeyFromFileIfc (const char* szKeyFile){
	int    iRet;
	   RSAKeyPair rsaKeyOut;
	rsaKeyOut.n.len=256;
	rsaKeyOut.e.len=256;
	rsaKeyOut.d.len=256;
	rsaKeyOut.p.len=256;
	rsaKeyOut.q.len=256;

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

	iRet= ReadRSAKeyfromFile(szKeyFile, &rsaKeyOut);
	if(iRet==1) {
		printf( "%s->SUCCESSFUL\n", __FUNCTION__);
		PrintBytes_LR(":n->", rsaKeyOut.n.data,rsaKeyOut.n.len);
		PrintBytes_LR(":e->", rsaKeyOut.e.data,rsaKeyOut.e.len);
		PrintBytes_LR(":d->", rsaKeyOut.d.data,rsaKeyOut.d.len);
		PrintBytes_LR(":p->", rsaKeyOut.p.data,rsaKeyOut.p.len);
		PrintBytes_LR(":q->", rsaKeyOut.q.data,rsaKeyOut.q.len);
}
	else
		printf( "%s: FAILED\n", __FUNCTION__);
}
	 
void SymEncryptFileIfc(const char* szKeyFile,  const char* szIntKeyFile, const char* szInFile, const char* szOutFile,
					const char* szAlgo, const char* szMode, const char* szPad,const char* szHmac) 
{
	int    iRet;
	InitCryptoRand();
	InitBigNum();
	iRet= EncryptSymFile(ENCRYPTFILE, szKeyFile, szIntKeyFile, szInFile, szOutFile,szAlgo,szMode,szPad,szHmac );
	if(iRet==1)
		printf( "%s: SUCCESSFUL\n", __FUNCTION__);
	else
		printf( "%s: FAILED\n", __FUNCTION__);
		
	CloseCryptoRand();
}

void SymDecryptFileIfc(const char* szKeyFile,  const char* szIntKeyFile, const char* szInFile, const char* szOutFile,
					const char* szAlgo, const char* szMode, const char* szPad,const char* szHmac)
{
	int    iRet;
	InitCryptoRand();
	InitBigNum();
	iRet= EncryptSymFile(DECRYPTFILE, szKeyFile, szIntKeyFile, szInFile, szOutFile, szAlgo,szMode,szPad,szHmac);
	if(iRet==1)
		printf( "%s: SUCCESSFUL\n", __FUNCTION__);
	else
		printf( "%s: FAILED\n", __FUNCTION__);
		
	CloseCryptoRand();
}

void SymEncrypt(const char* szKey, const char* szIntKey, const char* szInData, const char* szAlgo, 
			const char*szMode, const char*szPad, const char* szHmac) {
	
	InitCryptoRand();
	
	int    iRet;
	byte szBuf[4*1024]; 
	
	const int  dbsz = 4*1024;
	
	byte 	 dIn[4*1024];
	byte 	 dOut[4*1024];
	
	int outlen = dbsz;
		
	//keys
	byte 	kbCipher[32] = {0};
	byte	kbInt[32] = {0};
	
	SymKey keyCipher;
	keyCipher.data = &kbCipher[0];
	
	SymKey keyInt;
	keyInt.data = &kbInt[0];
					
	  
	byte szOut[512];	
	
	Data_buffer dataIn;
	dataIn.data = dIn;
	dataIn.len = dbsz;
		
	Data_buffer dataOut;	
	dataOut.data = dOut;
	dataOut.len = dbsz;

		
	do {

		//initilize keys
		iRet = fromBase64 (strlen(szKey), (const char *)szKey, &outlen, szBuf);
		if ( !iRet )
			break;
	
		keyCipher.len = outlen;
		memcpy(keyCipher.data, szBuf, outlen);
		
		outlen = dbsz;
		iRet = fromBase64 (strlen(szIntKey), szIntKey, &outlen, szBuf);
		if ( !iRet )
			break;
	
		keyInt.len = outlen;
		memcpy(keyInt.data, szBuf, outlen);
		
		outlen = dbsz;
		iRet = fromBase64 (strlen(szInData), szInData, &outlen, szBuf);
		if ( !iRet )
			break;

		dataIn.len = outlen;
		memcpy(dataIn.data, szBuf, outlen);
		
		
		u8 uAlgo = AES128;		
		if(strcmp(szAlgo, "AES256")==0) {
			uAlgo = AES256;
		}

		iRet= Encrypt(&keyCipher, &keyInt, &dataIn, &dataOut, CBCMODE, uAlgo, SYMPAD, HMACSHA256);
		
		if(iRet==1) {
			printf("Here %d datalen %d\n", __LINE__, dataOut.len);
			PrintBytes("before b64 : \n", dataOut.data, dataOut.len);
			
			iRet = toBase64 (dataOut.len, dataOut.data, &outlen, (char*)szBuf);
			if ( !iRet )
				break;
				
			printf( "%s: SUCCESSFUL\n", __FUNCTION__);
			printf( "OUT:%s\n", szBuf);
		}

	
	}while (0);
	
	if (iRet == 0 )
		printf( "%s: FAILED\n", __FUNCTION__);
	
	CloseCryptoRand();
	
}

void SymDecrypt (const char* szKey, const char* szIntKey, const char* szInData, const char* szAlgo, 
			const char*szMode, const char*szPad, const char* szHmac) {
	
	int    iRet;
	
	InitCryptoRand();
	
	byte szBuf[4*1024]; 
	
	const int  dbsz = 4*1024;
	
	byte 	 dIn[4*1024];
	byte 	 dOut[4*1024];
	
	int outlen = dbsz;
		
	//keys
	byte 	kbCipher[32] = {0};
	byte	kbInt[32] = {0};
	
	SymKey keyCipher;
	keyCipher.data = &kbCipher[0];
	
	SymKey keyInt;
	keyInt.data = &kbInt[0];
					
	  
	byte szOut[512];	
	
	Data_buffer dataIn;
	dataIn.data = dIn;
	dataIn.len = dbsz;
		
	Data_buffer dataOut;	
	dataOut.data = dOut;
	dataOut.len = dbsz;

		
	do {
		//initilize keys
		iRet = fromBase64 (strlen(szKey), (const char *)szKey, &outlen, szBuf);
		if ( !iRet )
			break;
		
		keyCipher.len = outlen;
		memcpy(keyCipher.data, szBuf, outlen);
		
		outlen = dbsz;
		iRet = fromBase64 (strlen(szIntKey), szIntKey, &outlen, szBuf);
		if ( !iRet )
			break;
		
		keyInt.len = outlen;
		memcpy(keyInt.data, szBuf, outlen);
		
		outlen = dbsz;
		iRet = fromBase64 (strlen(szInData), szInData, &outlen, szBuf);
		if ( !iRet )
			break;
		
		dataIn.len = outlen;
		memcpy(dataIn.data, szBuf, outlen);
		
		
		u8 uAlgo = AES128;		
		if(strcmp(szAlgo, "AES256")==0) {
			uAlgo = AES256;
		}
		
		iRet= Decrypt(&keyCipher, &keyInt, &dataIn, &dataOut, CBCMODE, uAlgo, SYMPAD, HMACSHA256);
		
		if(iRet==1) {
			printf("before b64 OUT:%s\n", dataOut.data);
			
			iRet = toBase64 (dataOut.len, dataOut.data, &outlen, (char*)szBuf);
			if ( !iRet )
				break;
				
			printf( "%s: SUCCESSFUL\n", __FUNCTION__);
			printf( "OUT:%s\n", szBuf);
		}
		
	
	}while (0);
	
	if (iRet != 1)
		printf( "%s: FAILED\n", __FUNCTION__);
	
	CloseCryptoRand();

}

 

void RSAEncryptIfc(const char* szKeyFile, const char* szRSAPad, const char* szInData) 
{
	int    iRet;
	
	InitCryptoRand();
	InitBigNum();

	RSAPublicKey pubKey;
	RSAPrivateKey priKey;
	u8	n[256];
	u8	e[256];

	pubKey.n.len=256;
	pubKey.n.data=n;

	pubKey.e.len=256;
	pubKey.e.data= e;

	u8	d[256];
	u8	nn[256];
	priKey.d.len=256;
	priKey.d.data= d;
	priKey.n.len=256;
	priKey.n.data=nn;

	char szBuf[512]; 
	int outlen = 512;

	//byte szIn[100];
	byte szOut[512];
	
	Data_buffer dataIn;
    Data_buffer dataOut;	
	
	dataIn.len= 512;
	dataIn.data=(byte *)szBuf;
	
	//fprintf(stdout, "%s\n",  szInData);
	memset(szBuf, 0, outlen);
	fromBase64(strlen(szInData), szInData, (int*)&dataIn.len, (u8*) szBuf);
	
	//fprintf(stdout, "after b64 %s\n",  szBuf);
	
	dataOut.data=szOut;
	dataOut.len=512;


	//Function to read an RSA key file from an XML file - ToDo
	RSAKeyPair rsaKeyOut;

	u8 bufp[256];
	u8 bufq[256];
	u8 bufn[256];
	//u8 bufnn[256];
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

	//RSAPublicKey pubKey;;
	iRet=  ReadRSAKeyfromFile(szKeyFile, &rsaKeyOut);
	if (iRet) 
	{
		pubKey.n.len= rsaKeyOut.n.len;
		memcpy(pubKey.n.data,rsaKeyOut.n.data, rsaKeyOut.n.len);
		
		PrintBytes_LR("rsaKeyOut.n.data ", rsaKeyOut.n.data, rsaKeyOut.n.len);
		
		pubKey.e.len= rsaKeyOut.e.len;
		memcpy(pubKey.e.data,rsaKeyOut.e.data, rsaKeyOut.e.len);

		PrintBytes_LR("rsaKeyOut.e.data ", rsaKeyOut.e.data, rsaKeyOut.e.len);
		
		PrintBytes_LR("dataIn.data ",dataIn.data,dataIn.len);


		iRet= RSAEncrypt(szRSAPad, &dataIn, &dataOut, &pubKey);
		if(iRet==1)  {
			printf( "%s: SUCCESSFUL\n", __FUNCTION__);
			printf( "dataOut.len = %d\n", dataOut.len);
			PrintBytes_LR("dataOut.data ",dataOut.data,dataOut.len);
			
		    toBase64 (dataOut.len, dataOut.data, &outlen, szBuf);
		    fprintf(stdout, "OUT:%s\n", szBuf);
		}			
		else
			printf( "%s: FAILED\n", __FUNCTION__);
	} else 
		printf( "%s: FAILED\n", __FUNCTION__);
 
    CloseCryptoRand();
}

void RSADecryptIfc(const char* szKeyFile, const char* szRSAPad, const char* szInData)
{
	int    iRet;

	InitCryptoRand();
	InitBigNum();
	RSAPrivateKey priKey;

	u8	n[256];
	u8	d[256];
	u8	p[256];
	u8	q[256];

	priKey.n.len=256;
	priKey.n.data=n;

	priKey.d.len=256;
	priKey.d.data= d;

	priKey.p.len=256;
	priKey.p.data= p;

	priKey.q.len=256;
	priKey.q.data= q;

	char szBuf[512]; 
	int outlen = 512;

	//byte szIn[100];
	byte szOut[512];
	Data_buffer dataIn;
    Data_buffer dataOut;	
    
    dataIn.len = 512; 
    dataIn.data = (byte *)szBuf;
      
    fromBase64(strlen(szInData), szInData, (int*)&dataIn.len, (u8*) szBuf);
		
	dataOut.data=szOut;
	dataOut.len=512;

	//Function to read an RSA key file from an XML file - ToDo
	RSAKeyPair keyPair;
	//RSAPrivateKey priKey;

	u8 bufp[256];
	u8 bufq[256];
	u8 bufn[256];
	u8 bufe[256];
	u8 bufd[256];

	keyPair.n.data=bufn;
	keyPair.e.data=bufe;
	keyPair.d.data=bufd;
	keyPair.p.data=bufp;
	keyPair.q.data=bufq;
	keyPair.n.len=256;
	keyPair.e.len=256;
	keyPair.d.len=256;
	keyPair.p.len=256;
	keyPair.q.len=256;

	iRet=  ReadRSAKeyfromFile(szKeyFile, &keyPair);

	if (iRet) {
		priKey.n.len= keyPair.n.len;
		memcpy(priKey.n.data,keyPair.n.data, keyPair.n.len);
		
		PrintBytes_LR("keyPair.n.data ", keyPair.n.data, keyPair.n.len);
		
		priKey.d.len= keyPair.d.len;
		memcpy(priKey.d.data,keyPair.d.data, keyPair.d.len);
		
		PrintBytes_LR("keyPair.d.data ", keyPair.d.data, keyPair.d.len);
		
		
		priKey.p.len= keyPair.p.len;
		memcpy(priKey.p.data,keyPair.p.data, keyPair.p.len);
		priKey.q.len= keyPair.q.len;
		memcpy(priKey.q.data,keyPair.q.data, keyPair.q.len);

		PrintBytes_LR("dataIn.data= ",dataIn.data,dataIn.len);
		
		iRet= RSADecrypt(szRSAPad, &dataIn, &dataOut, &priKey);

		if(iRet==1) {
			printf( "%s: SUCCESSFUL\n", __FUNCTION__);
			printf( "dataOut len = %d\n",dataOut.len);		
			PrintBytes_LR("dataOut.data=",dataOut.data,dataIn.len);
			
		   toBase64 (dataOut.len, dataOut.data, &outlen, szBuf);
		   fprintf(stdout, "OUT:%s\n", szBuf);                
		}
		else
			printf( "%s: FAILED\n", __FUNCTION__);
	} 
	else {
		printf( "%s: FAILED\n", __FUNCTION__);
	}
	
	CloseCryptoRand();
    
}

void DigestIfc (const char* szAlgorithm, const char* szIn) {
	int    iRet;
	int     i;
    unsigned int     ui;
    
	printf( "%s \n", szAlgorithm);
	int col=64;

	char szBuf[4*1024]; 
	int outlen = 4*1024;
	
	Data_buffer digest;
	Data_buffer data;
	byte buf[256];
	//typedef void* Digest_context;
	Digest_context ctx;


	data.data=(byte *)szIn;
	data.len=strlen(szIn);
	
	digest.data= buf;
	digest.len= 256;

	//InitDigest((char *)szDigestType, &ctx);
  
        if (strcmp(szAlgorithm,"SHA-1")==0) {
		Sha1InitDigest((char *)szAlgorithm, &ctx);
		Sha1UpdateDigest(&ctx, &data);
		Sha1GetDigest(&ctx, &digest);
		Sha1CloseDigest(&ctx);
		PrintBytes_LR("Digest:",digest.data, digest.len);
		fprintf(stdout, "\n");               
		toBase64 (digest.len, digest.data, &outlen, szBuf);
		fprintf(stdout, "OUT:%s\n", szBuf);               
	} 

        if (strcmp(szAlgorithm,"SHA-256")==0) {

		Sha256InitDigest((char *)szAlgorithm, &ctx);
		Sha256UpdateDigest(&ctx, &data);
		Sha256GetDigest(&ctx, &digest);
		Sha256CloseDigest(&ctx);
		PrintBytes_LR("Digest:",digest.data, digest.len);
		fprintf(stdout, "\n");               
		toBase64 (digest.len, digest.data, &outlen, szBuf);
		fprintf(stdout, "OUT:%s\n", szBuf);               
        }

	CloseCryptoRand();
}

void HMACIfc(const char* szKey, const char* szAlgorithm, const char* szIn) {
	int    iRet;
	int     i;
    unsigned int     ui;
    
	int col=32;
	
	char szBuf[1024]; 
	int outlen = 1024;
	
	const char* szMACType="HMAC";
	Data_buffer mac;
	Data_buffer data;
	byte buf[256];


	data.data=(byte *)szIn;
	data.len=strlen(szIn);

	mac.data= buf;
	mac.len=256;

	u8   bufk[32];
	SymKey symKey;
	symKey.data= bufk;
	symKey.len= 32;

	iRet = fromBase64(strlen(szKey), szKey, (int*)&symKey.len, (u8*) symKey.data);
	if ( ! iRet)
		return;

	
	GetMAC((char *)szMACType, &symKey, &data, &mac);
	
	toBase64 (mac.len, mac.data, &outlen, szBuf);
	fprintf(stdout, "OUT:%s\n", szBuf);        
	
	CloseCryptoRand();

 
}

void SignIfc (const char*  szKeyFile, const char*  szAlgorithm, const char* szInData) {
	int    iRet;
	
	InitCryptoRand();
	InitBigNum();

	RSAPrivateKey priKey;
	u8	n[256];
	u8	d[256];
	u8	p[256];
	u8	q[256];

	priKey.n.len=0;
	priKey.n.data=n;

	priKey.d.len=0;
	priKey.d.data= d;

	priKey.p.len=0;
	priKey.p.data= p;

	priKey.q.len=0;
	priKey.q.data= q;

	char szBuf[512]; 
	int outlen = 512;
	
	byte bOut[512];
	

	Data_buffer dataIn;
	Data_buffer sigOut;	

	dataIn.data = (byte *)szBuf;
	dataIn.len = 512;

	dataIn.len= 512;
	dataIn.data=(byte *)szBuf;
	
	//fprintf(stdout, "%s\n",  szInData);
	
	iRet = fromBase64(strlen(szInData), szInData, (int*)&dataIn.len, (u8*) dataIn.data);
	if ( ! iRet)
		return;


	sigOut.data=bOut;
	sigOut.len=512; // Buffer size

	RSAKeyPair keyPair;
	keyPair.n.len=0;
	keyPair.e.len=0;
	keyPair.d.len=0;
	keyPair.p.len=0;
	keyPair.q.len=0;

	u8 bufp[256];
	u8 bufq[256];
	u8 bufn[256];
	u8 bufe[256];
	u8 bufd[256];

	keyPair.n.data=bufn;
	keyPair.e.data=bufe;
	keyPair.d.data=bufd;
	keyPair.p.data=bufp;
	keyPair.q.data=bufq;
	keyPair.n.len=256;
	keyPair.e.len=256;
	keyPair.d.len=256;
	keyPair.p.len=256;
	keyPair.q.len=256;
        
	iRet=  ReadRSAKeyfromFile(szKeyFile, &keyPair);
	
	if (iRet) {
		priKey.n.len= keyPair.n.len;
		memcpy(priKey.n.data,keyPair.n.data, keyPair.n.len);
		priKey.d.len= keyPair.d.len;
		memcpy(priKey.d.data,keyPair.d.data, keyPair.d.len);
		priKey.p.len= keyPair.p.len;
		memcpy(priKey.p.data,keyPair.p.data, keyPair.p.len);
		priKey.q.len= keyPair.q.len;
		memcpy(priKey.q.data,keyPair.q.data, keyPair.q.len);
	
		iRet= Sign(szAlgorithm, &dataIn, &sigOut, &priKey);
		if(iRet==1) {			
			outlen = 512;
			iRet = toBase64 (sigOut.len, sigOut.data, &outlen, szBuf);
			if (iRet ){
				fprintf(stdout, "OUT:%s\n", szBuf);
			}
		}
		else
			printf( "%s: FAILED\n", __FUNCTION__);			
	} 
	else 
		printf( "%s: FAILED\n", __FUNCTION__);
	
	CloseCryptoRand();
}

void VerifyIfc (const char*  szKeyFile, const char*  szAlgorithm, const char* szInData, const char* szSigIn) {
	
	int    iRet;

	InitCryptoRand();
	InitBigNum();

	RSAPublicKey pubKey;
	RSAPrivateKey priKey;

	u8	n[256];
	u8	e[256];
	pubKey.n.len=256;
	pubKey.n.data=n;

	pubKey.e.len=256;
	pubKey.e.data= e;

	u8	d[256];
	u8	nn[256];
	priKey.d.len=256;
	priKey.d.data= d;
	priKey.n.len=256;
	priKey.n.data=nn;

	int outlen = 512;
	
	char szBuf[512]; 	
	byte sigBuf[512];
	
	Data_buffer dataIn;
	Data_buffer sigIn;	

	dataIn.data=(byte *)szBuf;
	dataIn.len= 512;

	sigIn.data=sigBuf;
	sigIn.len=512;

	iRet = fromBase64(strlen(szInData), szInData, (int*)&dataIn.len, (u8*) dataIn.data);
	if ( ! iRet)
		return;

	iRet = fromBase64(strlen(szSigIn), szSigIn, (int*)&sigIn.len, (u8*) sigIn.data);
	if ( ! iRet)
		return;

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

	iRet=  ReadRSAKeyfromFile(szKeyFile, &rsaKeyOut);
	if (iRet) {
		pubKey.n.len= rsaKeyOut.n.len;
		memcpy(pubKey.n.data,rsaKeyOut.n.data, rsaKeyOut.n.len);
		pubKey.e.len= rsaKeyOut.e.len;
		memcpy(pubKey.e.data,rsaKeyOut.e.data, rsaKeyOut.e.len);

		priKey.d.len= rsaKeyOut.d.len;
		memcpy(priKey.d.data,rsaKeyOut.d.data, rsaKeyOut.d.len);

		priKey.n.len= rsaKeyOut.n.len;
		memcpy(priKey.n.data,rsaKeyOut.n.data, rsaKeyOut.n.len);


		iRet= Verify(szAlgorithm, &dataIn, &sigIn, &pubKey);
		if(iRet==1) {
			printf( "OUT: SUCCESSFUL\n");
		}
		else
			printf( "%s: FAILED\n", __FUNCTION__);
	}  
	else 
		printf( "Error in reading key file\n");
	
	
	CloseCryptoRand();

}

void SignFileIfc(const char*  szKeyFile, const char*  szAlgorithm, const char*  szInFile, const char*  szOutFile) {
		int    iRet;
	
		InitCryptoRand();
		InitBigNum();

		RSAPrivateKey priKey;
		u8	n[256];
		u8	d[256];
		u8	p[256];
		u8	q[256];

		priKey.n.len=256;
		priKey.n.data=n;

		priKey.d.len=256;
		priKey.d.data= d;

		priKey.p.len=256;
		priKey.p.data= p;

		priKey.q.len=256;
		priKey.q.data= q;

		byte bOut[512];

		Data_buffer sigOut;	

		sigOut.data=bOut;
		sigOut.len=512; // Give enough buffer size.

		RSAKeyPair keyPair;

		u8 bufp[256];
		u8 bufq[256];
		u8 bufn[256];
		u8 bufe[256];
		u8 bufd[256];

		keyPair.n.data=bufn;
		keyPair.e.data=bufe;
		keyPair.d.data=bufd;
		keyPair.p.data=bufp;
		keyPair.q.data=bufq;
		keyPair.n.len=256;
		keyPair.e.len=256;
		keyPair.d.len=256;
		keyPair.p.len=256;
		keyPair.q.len=256;
        
        iRet=  ReadRSAKeyfromFile(szKeyFile, &keyPair);
        
        if (iRet) {
            priKey.n.len= keyPair.n.len;
            memcpy(priKey.n.data,keyPair.n.data, keyPair.n.len);
            priKey.d.len= keyPair.d.len;
            memcpy(priKey.d.data,keyPair.d.data, keyPair.d.len);
            priKey.p.len= keyPair.p.len;
            memcpy(priKey.p.data,keyPair.p.data, keyPair.p.len);
            priKey.q.len= keyPair.q.len;
            memcpy(priKey.q.data,keyPair.q.data, keyPair.q.len);
        
	    iRet= SignFile(szAlgorithm, szInFile, &sigOut, &priKey);
        } 
        else  {
		printf( "Error in reading key file\n");
        
        	CloseCryptoRand();
		return ;
	}
        
        if(iRet==1) {
            	printf( "%s: SUCCESSFUL\n", __FUNCTION__);
		int iWrite= open(szOutFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if(iWrite<0) {
			printf("Can't open write file to write signature\n");
			return;
		}

		iRet= write(iWrite, sigOut.data, sigOut.len);
		close(iWrite);
	}
        else
            	printf( "%s: FAILED\n", __FUNCTION__);

        CloseCryptoRand();
	return ;
	
}

void VerifyFileIfc(const char*  szKeyFile, const char*  szAlgorithm, const char*  szInFile, const char*  szSigInFile) {
	int    iRet;
	
	InitCryptoRand();
	InitBigNum();

	RSAPublicKey pubKey;
	RSAPrivateKey priKey;

	u8	n[256];
	u8	e[256];
	pubKey.n.len=256;
	pubKey.n.data=n;

	pubKey.e.len=256;
	pubKey.e.data= e;

	u8	d[256];
	u8	nn[256];
	priKey.d.len=256;
	priKey.d.data= d;
	priKey.n.len=256;
	priKey.n.data=nn;

	byte sigBuf[512];
	Data_buffer sigIn;	

	sigIn.data=sigBuf;
	sigIn.len=512;

	RSAKeyPair rsaKeyOut;

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

	iRet=  ReadRSAKeyfromFile(szKeyFile, &rsaKeyOut);
	if (iRet) {
		pubKey.n.len= rsaKeyOut.n.len;
		memcpy(pubKey.n.data,rsaKeyOut.n.data, rsaKeyOut.n.len);
		pubKey.e.len= rsaKeyOut.e.len;
		memcpy(pubKey.e.data,rsaKeyOut.e.data, rsaKeyOut.e.len);

		priKey.d.len= rsaKeyOut.d.len;
		memcpy(priKey.d.data,rsaKeyOut.d.data, rsaKeyOut.d.len);

		priKey.n.len= rsaKeyOut.n.len;
		memcpy(priKey.n.data,rsaKeyOut.n.data, rsaKeyOut.n.len);

	    	struct stat statBlock;
	    	if(stat(szSigInFile, &statBlock)<0) {
			return ;
	    	}
	    
	    	int fileSize= statBlock.st_size;
	    
	    	int iRead= open(szSigInFile, O_RDONLY);
	    	if(iRead<0) {
			printf("Can't open read file\n");
	    		return;
	    	}

	    	if(read(iRead, sigIn.data, fileSize)<0) {
           		printf("bad read\n");
            		return;
	    	}
	    
	    	sigIn.len= fileSize;	
	    	close(iRead);
            	iRet= VerifyFile(szAlgorithm, szInFile, &sigIn, &pubKey);
	}  
	else   {
			printf( "Error in reading key file\n");
			CloseCryptoRand();
			return;
		}
	
		if(iRet==1)
			printf( "%s: SUCCESSFUL\n", __FUNCTION__);
		else
			printf( "%s: FAILED\n", __FUNCTION__);
	
		CloseCryptoRand();
}

void PKDFIfc (const char* szKeyType, const char* szMethod, const char* szPassword, 
				const char* szSaltIn , int iRoundIn) { //last two params ignore temporarly
	int    iRet;

 
	InitCryptoRand();
	InitBigNum();
	const char* szSalt= "Saltyfromcracker";
	int iRounds = 20;
	
	byte szOut[100];
	
	Data_buffer dataIn;
	Data_buffer ssymKeyOut;	
	Data_buffer salt;	
	
	dataIn.data=(byte *)szPassword;
	dataIn.len=strlen(szPassword);
	
	ssymKeyOut.data=szOut;	
	ssymKeyOut.len=100;
	
	salt.data=(byte *)szSalt;
	salt.len=strlen(szSalt);
	

	iRet= DeriveKey(szKeyType, szMethod, &dataIn, &salt, &ssymKeyOut,iRounds);
	if(iRet==1)
	{
		int outlen = 512;
		char szBuf [512];
		iRet = toBase64 (ssymKeyOut.len, ssymKeyOut.data, &outlen, szBuf);
		if (iRet ){
			fprintf(stdout, "OUT:%s\n", szBuf);
		}
	}
	else {
		printf( "%s: FAILED\n", __FUNCTION__);
		CloseCryptoRand();
		return;
	}
	//printf( "The generated key is:\n");
	PrintBytes_LR("The generated key is:", ssymKeyOut.data,ssymKeyOut.len);
	CloseCryptoRand();

}

void Base64Ifc(const char* op, const char* szInData) {
	int    iRet;

	char szOut[1024];
	int piOutLen;
	int iInLen;
	bool fDir;
	bool iDir;

 
	InitCryptoRand();
        InitBigNum();
    iInLen= strlen(szInData);
	piOutLen=1024;

	if (strcmp(op, "encode") == 0) {
		//PrintBytes_LR("ToBase64 String- ",(byte*)szInData,iInLen);

		iRet= toBase64(iInLen, (byte *)szInData, &piOutLen, szOut);
		if(iRet==1) 
			printf( "%s-> SUCCESSFUL\n", __FUNCTION__);
		else {
			printf( "%s-> FAILED\n", __FUNCTION__);
			CloseCryptoRand();
			return;
		}
		
		printf("toBase64 String-> %s\n",(byte*)szOut,piOutLen);
	}
	else {
		int iInLen1;
		int piOutLen1;
		unsigned char szOut1[1024];
		piOutLen1= 1024;
		iInLen1= piOutLen;

		iRet= fromBase64(iInLen, szInData, &piOutLen1, szOut1 );
		
		if(iRet==1) 
			printf( "%s->SUCCESSFUL\n", __FUNCTION__);
		else {
			printf( "%s-> FAILED\n", __FUNCTION__);			
        		CloseCryptoRand();
			return;
		}
	
		//PrintBytes_LR("fromBase64 String-> ",szOut1,piOutLen1);
		printf("toBase64 String-> %s\n",szOut1);
	}

        CloseCryptoRand();
}

void RandomIfc(const char *szInData) {
	int    iRet;
	InitCryptoRand();
        InitBigNum();
        byte buf[2048];
        int numBits= atoi(szInData);
        iRet= GetCryptoRandom(numBits,buf);
        if(iRet==1) {
	    printf( "%s->SUCCESSFUL\n", __FUNCTION__);
            PrintBytes_LR("Random # - ", buf, numBits/8);
        }
        else
	    printf( "%s-> FAILED\n", __FUNCTION__);			
        CloseCryptoRand();
}

void Base64RandomIfc(const char *szInData) {
	int    iRet;

	InitCryptoRand();
        InitBigNum();
        byte buf[2048];
        char bufBase64[2048];
        int numBytes= atoi(szInData);
        int ibufBase64 = 2048;

        
	//printf( "In Base64RandomIfc\n");
        iRet= GetBase64Rand(numBytes,buf,&ibufBase64, bufBase64);

        if(iRet==1) {
	    printf( "%s->SUCCESSFUL\n", __FUNCTION__);
            PrintBytes("Random # \n", buf, numBytes);
            PrintBytes_LR("Base 64 Encoded- \n", (byte*)bufBase64, ibufBase64 );
        }
        else
	    printf( "%s-> FAILED\n", __FUNCTION__);			
        CloseCryptoRand();

}

void ShowHelp() {
	printf( "\n");
	printf( "\nUsage: rpapicryputil -GenKeyFile [AES128|AES256|RSA1024|RSA2048] outputfile\n");
	printf( "       rpapicryputil -GenSymKey [AES128|AES256]\n");
	printf( "       rpapicryputil -GenRSAKey {RSA1024|RSA2048]\n");
	printf( "       rpapicryputil -ReadAESKey keyfile\n");
	printf( "       rpapicryputil -ReadRSAKey keyfile\n");
	printf( "       rpapicryputil -Encrypt key intkey base64data [AES128|AES256] CBCMODE SYMPAD HMACSHA256 \n");
	printf( "       rpapicryputil -Decrypt key intkey base64data [AES128|AES256] CBCMODE SYMPAD HMACSHA256\n");
	printf( "       rpapicryputil -EncryptFile keyfile intkeyfile inputfile outputfile algotype modetype padtype hmactype\n");
	printf( "       rpapicryputil -DecryptFile keyfile intkeyfile inputfile outputfile algotype modetype padtype hmactype\n");
	printf( "       rpapicryputil -RSAEncrypt keyfile RSA_NO_PADDING base64data\n");
	printf( "       rpapicryputil -RSADecrypt keyfile RSA_NO_PADDING base64data\n");
	printf( "       rpapicryputil -Digest [SHA1, SHA-256] data\n");
	printf( "       rpapicryputil -HMAC  key HMACSHA256 data\n");
	//printf( "       rpapicryputil -SignXML keyfile rsa1024-sha256-pkcspad inputfile outputfile\n");
	//printf( "       rpapicryputil -VerifyXML keyfile inputfile\n");
	printf( "       rpapicryputil -Sign keyfile rsa1024-sha256-pkcspad base64data \n");
	printf( "       rpapicryputil -Verify keyfile rsa1024-sha256-pkcspad base64data signature\n");
	printf( "       rpapicryputil -SignFile keyfile rsa1024-sha256-pkcspad inputfile outputfile\n");
	printf( "       rpapicryputil -VerifyFile rsa1024-sha256-pkcspad inputfile sigfile\n");
	printf( "       rpapicryputil -PKDF  [AES128|AES256] PKDF2 password salt [round]\n");
	printf( "       rpapicryputil -Base64  inputstring dir\n");
	printf( "       rpapicryputil -Random  numbits\n");
	printf( "       rpapicryputil -Base64Random  numbytes\n");
 
}

int main(int an, char** av)
{
    const char*   szAlgo= NULL;
    const char*   szMode= NULL;
    const char*   szPad= NULL;
    const char*   szRSAPad= NULL;
    const char*   szHmac= NULL;

    const char*   szInFile= NULL;
    const char*   szKeyType= NULL;
    const char*   szOutFile= NULL;
    const char*   szSigInFile= NULL;
    const char*   szAlgorithm= NULL;
    const char*   szKeyFile= NULL;
    const char*   szIntKeyFile= NULL;
    const char*   szPassword= NULL;
    const char*   szSalt= NULL;
    const char*   szMethod= NULL;
    const char*   szInData= NULL;
    const char*   szDir= NULL;
    int     iRounds=0;


    int     iAction= NOACTION;
    int     mode= CBCMODE;
    int    iRet;
    int     i;
    unsigned int     ui;

    if(RPLibInit() != 1) {        
        printf( "%s\n","RP lib initialization failed\n");
        return -1;
    }

    for(i=0; i<an; i++) {
        //printf( "%s\n",av[i]);
        if(strcmp(av[i], "-help")==0) {
			ShowHelp();
            return 0;
        }

        if(strcmp(av[i], "-GenKeyFile")==0) {
            if(an<(i+3)) {
                printf( "Too few arguments: [AES128|RSA1024] output-file\n");
                return 1;
            }
            szKeyType= av[i+1];
            szOutFile= av[i+2];
            iAction= GENKEYFILE;
            GenKeyFileIfc(szKeyType, szOutFile);
            break;
        }

        if(strcmp(av[i], "-GenSymKey")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: [AES128|AES256] \n");
                return 1;
            }
            
            szKeyType= av[i+1];
            
            iAction= GENSYMKEY;
            GenSymKeyIfc( szKeyType );
            
            break;
        }
        if(strcmp(av[i], "-GenRSAKey")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: [RSA1024|RSA2048] \n");
                return 1;
            }
            szKeyType= av[i+1];
            iAction= GENRSAKEY;
            GenRSAKeyIfc(szKeyType);
            
            break;
        }

        if(strcmp(av[i], "-ReadAESKey")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: keyfile\n");
                return 1;
            }
            szKeyFile= av[i+1];
            iAction= READAESKEY;
            ReadAESKeyFromFileIfc(szKeyFile);
            break;
        }

        if(strcmp(av[i], "-ReadRSAKey")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: keyfile\n");
                return 1;
            }
            szKeyFile= av[i+1];
            iAction= READRSAKEY;
            ReadRSAKeyFromFileIfc(szKeyFile);
            
            break;
        }

        if(strcmp(av[i], "-SignXML")==0) {
            if(an<(i+4)) {
                printf( "Too few arguments: key-file rsa2048-sha256-pkcspad input-file output-file\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szAlgorithm= av[i+2];
            szInFile= av[i+3];
            szOutFile= av[i+4];
            iAction= SIGNXML;
            printf( "%s\n",szAlgorithm);
            break;
        }
        if(strcmp(av[i], "-Sign")==0) {
            if(an<(i+4)) {
                printf( "Too few arguments: key-file rsa2048-sha256-pkcspad input-file output-file\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szAlgorithm= av[i+2];
            szInFile= av[i+3];
            //szOutFile= av[i+4];
            //szSigInFile= av[i+4];
            iAction= SIGN;
            //printf( "%s\n",szAlgorithm);
            SignIfc	(szKeyFile, szAlgorithm, szInFile);
            break;
        }
        if(strcmp(av[i], "-SignFile")==0) {
            if(an<(i+4)) {
                printf( "Too few arguments: key-file rsa2048-sha256-pkcspad input-file output-file\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szAlgorithm= av[i+2];
            szInFile= av[i+3];
            szOutFile= av[i+4];
            iAction= SIGNFILE;
            printf( "%s\n",szAlgorithm);
            SignFileIfc(szKeyFile, szAlgorithm, szInFile, szOutFile);
            break;
        }
        if(strcmp(av[i], "-VerifyXML")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: key-file input-file\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szInFile= av[i+3];
            iAction= VERIFYXML;
            break;
        }
        if(strcmp(av[i], "-Verify")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: key-file input-file\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szAlgorithm= av[i+2];
            szInFile= av[i+3];
            //szOutFile= av[i+4];
            szSigInFile= av[i+4];
            iAction= VERIFY;
            VerifyIfc(szKeyFile, szAlgorithm, szInFile, szSigInFile );
            
            break;
        }
        if(strcmp(av[i], "-VerifyFile")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: key-file input-file\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szAlgorithm= av[i+2];
            szInFile= av[i+3];
            szOutFile= av[i+4];
            szSigInFile= av[i+4];
            iAction= VERIFYFILE;
            VerifyFileIfc(szKeyFile, szAlgorithm, szInFile, szSigInFile );
            break;
        }

	if(strcmp(av[i], "-RSAEncrypt")==0) {
            if(an<(i+3)) {
                printf( "Too few arguments: key-file padding data\n");
                return 1;
            }

            szKeyFile= av[i+1];
            szRSAPad= av[i+2];
            szInData= av[i+3];
			printf( "%s %s %s  \n", szKeyFile,szRSAPad,szInData);
            iAction= RSAENCRYPT;
            RSAEncryptIfc(szKeyFile, szRSAPad, szInData);
            break;
        }

        if(strcmp(av[i], "-EncryptFile")==0) {
            if(an<(i+9)) {
                printf( "Too few arguments: key-file intkeyfile input-file output-file algotype modetype padtype hmactype\n");
                return 1;
            }

            szKeyFile= av[i+1];
            szIntKeyFile= av[i+2];
            szInFile= av[i+3];
            szOutFile= av[i+4];
            szAlgo= av[i+5];
            szMode= av[i+6];
            szPad= av[i+7];
            szHmac= av[i+8];
            iAction= ENCRYPTFILE;
            if(an>(i+9) && strcmp(av[i+9],"gcm")==0)
                mode= CBCMODE;

			printf( "an=%d \n", an);
			printf( "i=%d \n", i);
			printf( "%d \n", mode);
			printf( "%s %s %s %s %s %s %s %s  \n", szKeyFile, szIntKeyFile, szInFile,szOutFile,szAlgo,szMode,szPad,szHmac);
			//printf( "%s \n", av[i+8]);
			
			SymEncryptFileIfc( szKeyFile, szIntKeyFile, szInFile,szOutFile,szAlgo,szMode,szPad,szHmac);

            break;
        }

        if(strcmp(av[i], "-Encrypt")==0) {
            if(an<(i+7)) {
                printf( "Too few arguments: key-file intkeyfile algotype modetype padtype hmactype data\n");
                return 1;
            }
			
			i++;
			
            //szOutFile= av[i+1];
            szKeyFile= av[i++];
            szIntKeyFile= av[i++];
            szInData= av[i++];
            szAlgo= av[i++];
            szMode= av[i++];
            szPad= av[i++];
            szHmac= av[i++];
          
            iAction= ENCRYPT;
            if(an>(i+8) && strcmp(av[i+8],"gcm")==0)
                mode= CBCMODE;

			printf( "%s %s  %s %s %s %s  \n",szKeyFile,szIntKeyFile,szAlgo,szMode,szPad,szHmac);
	    //printf( "%s \n", av[i+6]);
			SymEncrypt( szKeyFile, szIntKeyFile, szInData, szAlgo,szMode,szPad,szHmac);
            break;
		}

        if(strcmp(av[i], "-RSADecrypt")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: key-file padding\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szRSAPad= av[i+2];
            szInData= av[i+3];
            iAction= RSADECRYPT;
			printf( "%s %s  \n",szKeyFile,szRSAPad);
			RSADecryptIfc (szKeyFile, szRSAPad, szInData);
            break;
        }

        if(strcmp(av[i], "-DecryptFile")==0) {
            if(an<(i+9)) {
                printf( "Too few arguments: key-file intkeyfile input-file output-file algotype modetype padtype hmactype\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szIntKeyFile= av[i+2];
            szInFile= av[i+3];
            szOutFile= av[i+4];
            szAlgo= av[i+5];
            szMode= av[i+6];
            szPad= av[i+7];
            szHmac= av[i+8];
            iAction= DECRYPTFILE;
            if(an>(i+9) && strcmp(av[i+9],"gcm")==0)
                mode= CBCMODE;

			printf( "an=%d \n", an);
			printf( "i=%d \n", i);
			printf( "%s %s %s %s %s %s %s %s  \n", szKeyFile, szIntKeyFile, szInFile,szOutFile,szAlgo,szMode,szPad,szHmac);
	    //printf( "%s \n", av[i+8]);
			SymDecryptFileIfc( szKeyFile, szIntKeyFile, szInFile,szOutFile,szAlgo,szMode,szPad,szHmac);
            break;
        }

        if(strcmp(av[i], "-Decrypt")==0) {
            if(an<(i+7)) {
                printf( "Too few arguments: key-file intkeyfile algotype modetype padtype hmactype\n");
                return 1;
            }
            i++;
            
            szKeyFile= av[i++];
            szIntKeyFile= av[i++];
            szInData= av[i++];
            szAlgo= av[i++];
            szMode= av[i++];
            szPad= av[i++];
            szHmac= av[i++];
            
            iAction= DECRYPT;
            if(an>(i+7) && strcmp(av[i+7],"gcm")==0)
                mode= CBCMODE;


			printf( "%s %s %s %s %s %s  \n", szKeyFile,szIntKeyFile,szAlgo,szMode,szPad,szHmac);
			//printf( "%s \n", av[i+6]);
			SymDecrypt( szKeyFile, szIntKeyFile, szInData, szAlgo, szMode, szPad, szHmac);
			
            break;
        }

		if(strcmp(av[i], "-Digest")==0) {
			if(an<(i+2)) {
					printf("Too few arguments: alg\n");
					return 1;
				}
			iAction= DIGEST;
			szAlgorithm= av[i+1];
			szInData = av[i+2];
			//szAlgorithm= "SHA256";
			DigestIfc (szAlgorithm, szInData);
			break;
        }

		if(strcmp(av[i], "-HMAC")==0) {
			if(an<(i+2)) {
					printf("Too few arguments: key-file alg\n");
					return 1;
            }

            iAction= HMAC;
            szKeyFile= av[i+1];
            szAlgorithm= av[i+2];
            szInData= av[i+3];
            //szAlgorithm= "SHA256";
            szAlgorithm= "HMAC";
            
            HMACIfc(szKeyFile, szAlgorithm, szInData);
            
            break;
        }

		if(strcmp(av[i], "-PKDF")==0) {
			if(an<(i+3)) {
					printf("Too few arguments: keytype method password\n");
					return 1;
				}

            iAction= PKDF;
            szKeyType= av[i+1];
            szMethod= av[i+2];
            szPassword= av[i+3];
            szSalt= av[i+4];
            iRounds= atoi(av[i+5]);
			printf( "%s %s %s %s \n", szKeyType,szMethod,szPassword,szSalt, iRounds);

			PKDFIfc (szKeyType,szMethod,szPassword,szSalt, iRounds);
            break;
        }


		if(strcmp(av[i], "-Base64")==0) {
			if(an<(i+3)) {
					printf("Too few arguments: input string [true|false]\n");
					return 1;
            }
			
            iAction= BASE64;
            const char* szOp= av[i+1];
            szInData= av[i+2];
	    //printf( "%s %s\n", szInData,szDir);
	     Base64Ifc(szOp, szInData);

            break;
        }
        
		if(strcmp(av[i], "-Random")==0) {
			if(an<(i+2)) {
					printf("Too few arguments: numbits\n");
					return 1;
            }

            iAction= RANDOM;
            szInData= av[i+1];
	    //printf( "%s \n", szInData);
	    RandomIfc(szInData);
            break;
        }
        
		if(strcmp(av[i], "-Base64Random")==0) {
			if(an<(i+2)) {
                printf("Too few arguments: numbytes\n");
                return 1;
            }

            iAction= BASE64RANDOM;
            szInData= av[i+1];
	    //printf( "%s \n", szInData);
	    Base64RandomIfc(szInData);
            break;
        }
    }



   // 
#ifdef ENABLE_ACTIONS
    if(iAction==NOACTION) {
        printf( "Cant find option\n");
        ShowHelp();
        return 1;
    }


    if(iAction==SIGNXML) {
        InitCryptoRand();
        InitBigNum();
        iRet= SignXML(szKeyFile, szAlgorithm, szInFile, szOutFile);
        CloseCryptoRand();
        if(iRet==1)
            printf( "SignXML succeeded\n");
        else
            printf( "SignXML failed\n");
    }
    

    if(iAction==VERIFYXML) {
        InitCryptoRand();
        InitBigNum();
        iRet= VerifyXML(szKeyFile, szInFile);
        if(iRet==1)
            printf( "VerifyXML - Signature verifies\n");
        else
            printf( "VerifyXML - Signature fails\n");
        CloseCryptoRand();


    }
    

    if(iAction==RANDOM) {
        InitCryptoRand();
        InitBigNum();
      	byte buf[2048]; 
	int numBits= atoi(szInData); 
        iRet= GetCryptoRandom(numBits,buf);
        if(iRet==1) {
            printf( "GetCryptoRandom returning successfully\n");
            PrintBytes("Random # - ", buf, numBits/8);
            PrintBytes("Key- ", buf, numBits/8);
	}
        else
            printf( "GetCryptoRandom returning unsuccessfully\n");
        CloseCryptoRand();
    }
    if(iAction==BASE64RANDOM) {
        InitCryptoRand();
        InitBigNum();
      	byte buf[2048]; 
      	char bufBase64[2048]; 
	int numBytes= atoi(szInData); 
        int ibufBase64 = 2048;
	
        iRet= GetBase64Rand(numBytes,buf,&ibufBase64, bufBase64);

        if(iRet==1) {
            printf( "GetBase64Rand returning successfully\n");
            PrintBytes("Random # \n", buf, numBytes);
            PrintBytes("Base 64 Encoded- \n", (byte*)bufBase64, ibufBase64 );
	}
        else
            printf( "GetBase64Rand returning unsuccessfully\n");
        CloseCryptoRand();
    }

#endif
    printf("\n");

    RPLibExit();
    return 0;
}


// -------------------------------------------------------------------------

