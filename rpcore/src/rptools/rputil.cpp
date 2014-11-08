//
//  File: rpapiutil.cpp
//        Sujan Negi
//  Description: rp crypto util sunny day scenario test tool.
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


#include "rpapi.h"
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
#define BASE64TO               	  211
#define BASE64FROM            	  212
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

bool  toBase64(int iInLen, const unsigned char* puIn, int* piOutLen, char* rgszOut/*, bool fDirFwd=true*/);
bool  fromBase64(int iInLen, const char* pszIn, int* piOutLen, unsigned char* puOut/*, bool fDirFwd=true*/);
bool  getBase64Rand(int iBytes, byte* puR, int* pOutSize, char* szOut);

bool hmac_sha256(byte* rguMsg, int iInLen, byte* rguKey, int iKeyLen, byte* rguDigest);
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

byte HexToVal(char a)
{
    if( (a>='0')  && (a<='9') )
        return a-'0';
    if( (a>='a') && ( a<='f'))
        return a-'a'+10;
    return -1;
}

int FromHexStringToByteString(const char* szIn, int iSizeOut, byte* rgbBuf)
{
    char    a, b,c;
    int     j= 0;
    byte aval, bval;

    if (szIn == NULL) return -1;
   
    while(*szIn != 0) {
        if(*szIn=='\n' || *szIn==' ') {
            szIn++;
            continue;
         }
        if (*szIn=='0' || *(szIn+1)=='x') {
            szIn++; szIn++;
            continue;
        }

        a= *(szIn++);
        b= *(szIn++);
        if(a==0 || b==0) break;
        aval = HexToVal(a);
        if (aval == -1) return -1;
        bval = HexToVal(b);
        if (bval == -1) return -1;
        c= aval*16 + bval;

        rgbBuf[j++]= c;
    }
    
    return j;
}
   
void PrintBytes_LR(const char* szMsg, byte* pbData, int iSize, int col=32)
{
    int i;

    printf("%s", szMsg);
    for (i= 0; i<iSize; i++) {
        printf("%02x", pbData[i]);
        if((i%col)==(col-1))
            printf("\n");
        }
    printf("\n");
}


int RPUtilInit()
{
 return 0;
}
int RPUtilExit()
{
 return 0;
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
    const char*   szEnDec= NULL;
    int     iRounds=0;


    int     iAction= NOACTION;
    int     mode= CBCMODE;
    int    iRet;
    int     i;
    unsigned int     ui;

    RPUtilInit();

    // The above two for just testing the RPLibInit() and RPLibExit() APIs.

    for(i=0; i<an; i++) {
        printf( "%s\n",av[i]);
        if(strcmp(av[i], "-help")==0) {
            printf( "\n");
            printf( "\nUsage: rpapicryputil -GenKeyFile keytype outputfile\n");
            printf( "       rpapicryputil -GenSymKey keytype\n");
            printf( "       rpapicryputil -GenRSAKey keytype\n");
            printf( "       rpapicryputil -ReadAESKey keyfile\n");
            printf( "       rpapicryputil -ReadRSAKey keyfile\n");
            printf( "       rpapicryputil -Encrypt keyfile intkeyfile algotype modetype padtype hmactype data\n");
            printf( "       rpapicryputil -Decrypt keyfile intkeyfile algotype modetype padtype hmactype\n");
            printf( "       rpapicryputil -EncryptFile keyfile intkeyfile inputfile outputfile algotype modetype padtype hmactype\n");
            printf( "       rpapicryputil -DecryptFile keyfile intkeyfile inputfile outputfile algotype modetype padtype hmactype\n");
            printf( "       rpapicryputil -RSAEncrypt keyfile padtype data\n");
            printf( "       rpapicryputil -RSADecrypt keyfile padtype\n");
            printf( "       rpapicryputil -Digest alg\n");
            printf( "       rpapicryputil -HMAC  alg \n");
            printf( "       rpapicryputil -SignXML keyfile rsa1024-sha256-pkcspad inputfile outputfile\n");
            printf( "       rpapicryputil -VerifyXML keyfile inputfile\n");
            printf( "       rpapicryputil -Sign keyfile rsa1024-sha256-pkcspad inputfile outputfile\n");
            printf( "       rpapicryputil -Verify keyfile rsa1024-sha256-pkcspad inputfile sigfile\n");
            printf( "       rpapicryputil -SignFile keyfile rsa1024-sha256-pkcspad inputfile outputfile\n");
            printf( "       rpapicryputil -VerifyFile keyfile rsa1024-sha256-pkcspad inputfile sigfile\n");
            printf( "       rpapicryputil -PKDF  keytype method password salt round\n");
            printf( "       rpapicryputil -Base64 action  input_string dir\n");
            printf( "       rpapicryputil -Random  numbits\n");
            printf( "       rpapicryputil -Base64Random  numbytes\n");
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
            break;
        }

        if(strcmp(av[i], "-GenSymKey")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: [AES128|AES256] \n");
                return 1;
            }
            szKeyType= av[i+1];
            iAction= GENSYMKEY;
            break;
        }
        if(strcmp(av[i], "-GenRSAKey")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: [RSA1024|RSA2048] \n");
                return 1;
            }
            szKeyType= av[i+1];
            iAction= GENRSAKEY;
            break;
        }

        if(strcmp(av[i], "-ReadAESKey")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: keyfile\n");
                return 1;
            }
            szKeyFile= av[i+1];
            iAction= READAESKEY;
            break;
        }

        if(strcmp(av[i], "-ReadRSAKey")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: keyfile\n");
                return 1;
            }
            szKeyFile= av[i+1];
            iAction= READRSAKEY;
            break;
        }

        if(strcmp(av[i], "-SignXML")==0) {
            if(an<(i+5)) {
                printf( "Too few arguments: key-file rsa1024-sha256-pkcspad input-file output-file\n");
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
            if(an<(i+5)) {
                printf( "Too few arguments: key-file rsa1024-sha256-pkcspad input-file output-file\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szAlgorithm= av[i+2];
            szInFile= av[i+3];
            szOutFile= av[i+4];
            iAction= SIGN;
            printf( "%s %s %s %s\n",szKeyFile, szAlgorithm,szInFile,szOutFile);
            break;
        }
        if(strcmp(av[i], "-SignFile")==0) {
            if(an<(i+5)) {
                printf( "Too few arguments: key-file rsa1024-sha256-pkcspad input-file output-file\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szAlgorithm= av[i+2];
            szInFile= av[i+3];
            szOutFile= av[i+4];
            printf( "%s %s %s %s\n",szKeyFile, szAlgorithm,szInFile,szOutFile);
            iAction= SIGNFILE;
            break;
        }
        if(strcmp(av[i], "-VerifyXML")==0) {
            if(an<(i+3)) {
                printf( "Too few arguments: key-file input-file\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szInFile= av[i+2];
            iAction= VERIFYXML;
            break;
        }
        if(strcmp(av[i], "-Verify")==0) {
            if(an<(i+5)) {
                printf( "Too few arguments:  keyfile rsa1024-sha256-pkcspad inputfile sigfile\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szAlgorithm= av[i+2];
            szInFile= av[i+3];
            szSigInFile= av[i+4];
            iAction= VERIFY;
            printf( "%s %s %s %s\n",szKeyFile, szAlgorithm,szInFile,szSigInFile);
            break;
        }
        if(strcmp(av[i], "-VerifyFile")==0) {
            if(an<(i+5)) {
                printf( "Too few arguments:  keyfile rsa1024-sha256-pkcspad inputfile sigfile\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szAlgorithm= av[i+2];
            szInFile= av[i+3];
            szSigInFile= av[i+4];
            printf( "%s %s %s %s\n",szKeyFile, szAlgorithm,szInFile,szSigInFile);
            iAction= VERIFYFILE;
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

            break;
        }

        if(strcmp(av[i], "-Encrypt")==0) {
            if(an<(i+8)) {
                printf( "Too few arguments: key-file intkeyfile algotype modetype padtype hmactype data\n");
                return 1;
            }

            //szOutFile= av[i+1];
            szKeyFile= av[i+1];
            szIntKeyFile= av[i+2];
            szAlgo= av[i+3];
            szMode= av[i+4];
            szPad= av[i+5];
            szHmac= av[i+6];
            szInData= av[i+7];
            iAction= ENCRYPT;
            if(an>(i+8) && strcmp(av[i+8],"gcm")==0)
                mode= CBCMODE;

	    printf( "an=%d \n", an);
	    printf( "i=%d \n", i);
	    printf( "%d \n", mode);
	    printf( "%s %s  %s %s %s %s  \n",szKeyFile,szIntKeyFile,szAlgo,szMode,szPad,szHmac);
	    //printf( "%s \n", av[i+6]);

            break;
	}

        if(strcmp(av[i], "-RSADecrypt")==0) {
            if(an<(i+2)) {
                printf( "Too few arguments: key-file padding\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szRSAPad= av[i+2];
            iAction= RSADECRYPT;
	    printf( "%s %s  \n",szKeyFile,szRSAPad);
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
            break;
        }

        if(strcmp(av[i], "-Decrypt")==0) {
            if(an<(i+7)) {
                printf( "Too few arguments: key-file intkeyfile algotype modetype padtype hmactype\n");
                return 1;
            }
            szKeyFile= av[i+1];
            szIntKeyFile= av[i+2];
            szAlgo= av[i+3];
            szMode= av[i+4];
            szPad= av[i+5];
            szHmac= av[i+6];
            iAction= DECRYPT;
            if(an>(i+7) && strcmp(av[i+7],"gcm")==0)
                mode= CBCMODE;

	    printf( "an=%d \n", an);
	    printf( "i=%d \n", i);
	    printf( "%s %s %s %s %s %s  \n", szKeyFile,szIntKeyFile,szAlgo,szMode,szPad,szHmac);
	    //printf( "%s \n", av[i+6]);
            break;
        }

	if(strcmp(av[i], "-Digest")==0) {
	    if(an<(i+2)) {
                printf("Too few arguments: alg\n");
                return 1;
            }
            iAction= DIGEST;
            szAlgorithm= av[i+1];
            //szAlgorithm= "SHA256";
            break;
        }
	if(strcmp(av[i], "-HMAC")==0) {
	    if(an<(i+2)) {
                printf("Too few arguments: key-file alg\n");
                return 1;
            }

            iAction= HMAC;
            //szKeyFile= av[i+1];
            szAlgorithm= av[i+1];
            //szInData = av[i+3]
            //szAlgorithm= "SHA-256";
            szAlgorithm= "HMAC";
            break;
        }

	if(strcmp(av[i], "-PKDF")==0) {
	    if(an<(i+6)) {
                printf("Too few arguments: keytype method password salt round\n");
                return 1;
            }

            iAction= PKDF;
            szKeyType= av[i+1];
            szMethod= av[i+2];
            szPassword= av[i+3];
            szSalt= av[i+4];
            iRounds= atoi(av[i+5]);
	    printf( "%s %s %s %s %d\n", szKeyType,szMethod,szPassword,szSalt, iRounds);
            break;
        }


	if(strcmp(av[i], "-Base64")==0) {
	    if(an<(i+4)) {
                printf("Too few arguments: action input_string [true|false]\n");
                return 1;
            }

            iAction= BASE64;
	    szEnDec= av[i+1];
            szInData= av[i+2];
            szDir= av[i+3];
	    printf( "%s %s %s\n", szEnDec, szInData,szDir);
            break;
        }
	if(strcmp(av[i], "-Random")==0) {
	    if(an<(i+2)) {
                printf("Too few arguments: numbits\n");
                return 1;
            }

            iAction= RANDOM;
            szInData= av[i+1];
	    printf( "%s \n", szInData);
            break;
        }
	if(strcmp(av[i], "-Base64Random")==0) {
	    if(an<(i+2)) {
                printf("Too few arguments: numbytes\n");
                return 1;
            }

            iAction= BASE64RANDOM;
            szInData= av[i+1];
	    printf( "%s \n", szInData);
            break;
        }
    }


    printf( "Command line options parsed\n");
    if(iAction==NOACTION) {
        printf( "Cant find option\n");
        return 1;
    }

    if(iAction==GENKEYFILE) {
        InitCryptoRand();
        InitBigNum();
        iRet= GenKeyFile(szKeyType, szOutFile);
        if(iRet==1)
            printf( "GenKeyFile returning successfully\n");
        else
            printf( "GenKeyFile returning unsuccessfully\n");
        CloseCryptoRand();
    }

    const char* reserved=NULL;
    u8              buf[32];
    SymKey symKeyOut;
    symKeyOut.data= buf;
    symKeyOut.len= 32; 

    if(iAction==GENSYMKEY) {
        InitCryptoRand();
        InitBigNum();

        iRet= GenSymKey(szKeyType, reserved, &symKeyOut);
        if(iRet==1) {
            PrintBytes_LR("AES key:", buf,symKeyOut.len);
            printf( "GenSymKey returning successfully\n");
	}
        else
            printf( "GenSymKey returning unsuccessfully\n");
        CloseCryptoRand();
    }

    //SymKey pubExp;
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

    if(iAction==GENRSAKEY) {
        InitCryptoRand();
        InitBigNum();
        iRet= GenRSAKey(szKeyType, reserved, pubExp, &rsaKeyOut);
        if(iRet==1) {
            printf( "GenRSAKey returning successfully\n");
            PrintBytes_LR("n :", rsaKeyOut.n.data,rsaKeyOut.n.len);
            PrintBytes_LR("e:", rsaKeyOut.e.data,rsaKeyOut.e.len);
            PrintBytes_LR("d:", rsaKeyOut.d.data,rsaKeyOut.d.len);
            PrintBytes_LR("p:", rsaKeyOut.p.data,rsaKeyOut.p.len);
            PrintBytes_LR("q:", rsaKeyOut.q.data,rsaKeyOut.q.len);
	}
        else
            printf( "GenRSAKey returning unsuccessfully\n");
        CloseCryptoRand();
    }


    if(iAction==READAESKEY) {
        u8              buf[32];
        SymKey symKeyOut;
        symKeyOut.data= buf;
        symKeyOut.len= 32;

        iRet= ReadAESKeyfromFile(szKeyFile, &symKeyOut);
        if(iRet==1) {
            printf( "ReadAesKey returning successfully\n");
            PrintBytes_LR("AES key:", buf,symKeyOut.len);
	}
        else
            printf( "ReadAesKey returning unsuccessfully\n");
    }

    if(iAction==READRSAKEY) {
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
            printf( "ReadAesKey returning successfully\n");
            PrintBytes_LR("n :", rsaKeyOut.n.data,rsaKeyOut.n.len);
            PrintBytes_LR("e:", rsaKeyOut.e.data,rsaKeyOut.e.len);
            PrintBytes_LR("d:", rsaKeyOut.d.data,rsaKeyOut.d.len);
            PrintBytes_LR("p:", rsaKeyOut.p.data,rsaKeyOut.p.len);
            PrintBytes_LR("q:", rsaKeyOut.q.data,rsaKeyOut.q.len);
	}
        else
            printf( "ReadAesKey returning unsuccessfully\n");
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
    if(iAction==SIGNFILE) {
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

        //char szIn[]="0011223344556677889900"; 
        //char szIn[]="000000000000000000000000000000000000000011111111111111111111111111111111111111122222222222222222222222222222222222222220123456789"; 
        byte bOut[512];

	//Data_buffer dataIn;
        Data_buffer sigOut;	

	//dataIn.data=(byte *)szIn;
	//dataIn.len=strlen(szIn);
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
        } else printf( "Error in reading key file\n");
        CloseCryptoRand();
        if(iRet==1)
            printf( "SignFile succeeded\n");
        else
            printf( "SignFile failed\n");
	
	int iWrite= open(szOutFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if(iWrite<0) {
		printf("Can't open write file to write signature\n");
		return -1;
    	}
	iRet= write(iWrite, sigOut.data, sigOut.len);
	close(iWrite);

    }
    if(iAction==SIGN) {
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

        char szIn[]="0011223344556677889900"; 
        //char szIn[]="000000000000000000000000000000000000000011111111111111111111111111111111111111122222222222222222222222222222222222222220123456789"; 
        byte bOut[512];

	Data_buffer dataIn;
        Data_buffer sigOut;	

	dataIn.data=(byte *)szIn;
	dataIn.len=strlen(szIn);
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
        
	    struct stat statBlock;
	    //if(stat(szInFile, &statBlock)<0) 
	    if(stat(szInFile, &statBlock)<0) {
		return false;
	    }
	    int fileSize= statBlock.st_size;
	    int iRead= open(szInFile, O_RDONLY);
	    if(iRead<0) {
	   	 printf("Can't open read file\n");
	    	return false;
	    }

	    if(read(iRead, dataIn.data, fileSize)<0) {
           	printf("bad read\n");
            	return false;
	    }
	    dataIn.len= fileSize;	
	    close(iRead);
	    iRet= Sign(szAlgorithm, &dataIn, &sigOut, &priKey);
        } else printf( "Error in reading key file\n");
        CloseCryptoRand();

        if(iRet==1)
            printf( "Sign succeeded\n");
        else
            printf( "Sign failed\n");
	
	int iWrite= open(szOutFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if(iWrite<0) {
		printf("Can't open write file to write signature\n");
		return -1;
    	}
	iRet= write(iWrite, sigOut.data, sigOut.len);
	close(iWrite);

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
    if(iAction==VERIFYFILE) {
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

        //char szIn[]="0011223344556677889900"; 
        //char szIn[]="000000000000000000000000000000000000000011111111111111111111111111111111111111122222222222222222222222222222222222222220123456789"; 
        byte sigBuf[512];
	//Data_buffer dataIn;
        Data_buffer sigIn;	

	//dataIn.data=(byte *)szIn;
	//dataIn.len=strlen(szIn);
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
	    //if(stat(szInFile, &statBlock)<0) 
	    if(stat(szSigInFile, &statBlock)<0) {
		return false;
	    }
	    int fileSize= statBlock.st_size;
	    int iRead= open(szSigInFile, O_RDONLY);
	    if(iRead<0) {
	   	 printf("Can't open read file\n");
	    	return false;
	    }

	    if(read(iRead, sigIn.data, fileSize)<0) {
           	printf("bad read\n");
            	return false;
	    }
	    sigIn.len= fileSize;	
	    close(iRead);
            iRet= VerifyFile(szAlgorithm, szInFile, &sigIn, &pubKey);
        }  else printf( "Error in reading key file\n");
        if(iRet==1)
            printf( "VerifyFile - Signature verifies\n");
        else
            printf( "VerifyFile - Signature fails\n");
        CloseCryptoRand();
    }
    if(iAction==VERIFY) {
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

        char szIn[]="0011223344556677889900"; 
        //char szIn[]="000000000000000000000000000000000000000011111111111111111111111111111111111111122222222222222222222222222222222222222220123456789"; 
        byte sigBuf[512];
	Data_buffer dataIn;
        Data_buffer sigIn;	

	dataIn.data=(byte *)szIn;
	dataIn.len=strlen(szIn);
	sigIn.data=sigBuf;
	sigIn.len=512;

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

	    struct stat statBlock;
	    if(stat(szInFile, &statBlock)<0) {
		return false;
	    }
	    int fileSize= statBlock.st_size;
	    int iRead= open(szInFile, O_RDONLY);
	    if(iRead<0) {
	   	 printf("Can't open read file\n");
	    	return false;
	    }
	    if(read(iRead, dataIn.data, fileSize)<0) {
           	printf("bad read\n");
            	return false;
	    }
	    dataIn.len= fileSize;	
	    close(iRead);
	    if(stat(szSigInFile, &statBlock)<0) {
	   	 printf("Can't stat signature file\n");
		return false;
	    }
	    fileSize= statBlock.st_size;
	    iRead= open(szSigInFile, O_RDONLY); 
	    if(iRead<0) {
	   	 printf("Can't open read file\n");
	    	return false;
	    }
	    if(read(iRead, sigIn.data, fileSize)<0) {
           	printf("bad read\n");
            	return false;
	    }
	    sigIn.len= fileSize;	
	    close(iRead);


            iRet= Verify(szAlgorithm, &dataIn, &sigIn, &pubKey);
        }  else printf( "Error in reading key file\n");
        if(iRet==1)
            printf( "Verify - Signature verifies\n");
        else
            printf( "Verify - Signature fails\n");
        CloseCryptoRand();
    }

    if(iAction==ENCRYPTFILE) {
        InitCryptoRand();
        InitBigNum();
        iRet= EncryptSymFile(ENCRYPTFILE, szKeyFile, szIntKeyFile, szInFile, szOutFile,szAlgo,szMode,szPad,szHmac );
        if(iRet==1)
            printf( "Encryptfile returned successfully\n");
        else
            printf( "Encryptfile returned unsuccessfully\n");
        CloseCryptoRand();
    }

    if(iAction==DECRYPTFILE) {
        InitCryptoRand();
        InitBigNum();
        iRet= EncryptSymFile(DECRYPTFILE, szKeyFile, szIntKeyFile, szInFile, szOutFile, szAlgo,szMode,szPad,szHmac);
        if(iRet==1)
            printf( "Decryptfile returned successfully\n");
        else
            printf( "Deccryptfile returned unsuccessfully\n");
        CloseCryptoRand();
    }

    if(iAction==ENCRYPT) {
        InitCryptoRand();
        InitBigNum();
        //byte szIn[100];
        char szIn[]="012345678900011223344556677889900"; 
        byte szOut[512];
    	Data_buffer dataIn;
        Data_buffer dataOut;	
    	dataIn.data=(byte *)szIn;
    	dataIn.len=strlen(szIn);
    	dataOut.data=szOut;
    	dataOut.len=512;

        dataIn.data= (byte *)szInData;
        dataIn.len= strlen(szInData);
        iRet= EncryptSym(ENCRYPT, szKeyFile, szIntKeyFile, &dataIn, &dataOut, szAlgo,szMode,szPad,szHmac);
        if(iRet==1)
            printf( "Encryp returned successfully\n");
        else
            printf( "Encryp returned unsuccessfully\n");
        CloseCryptoRand();
	int iWrite= open("./rpcryptmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if(iWrite<0) {
		printf("Can't open write file to write encrypted data\n");
		return -1;
    	}
	iRet= write(iWrite, dataOut.data, dataOut.len);
	close(iWrite);
    }

    if(iAction==DECRYPT) {
        InitCryptoRand();
        InitBigNum();
        char szIn[]="012345678900011223344556677889900"; 
        //byte szIn[100];
        byte szOut[512];
    	Data_buffer dataIn;
        Data_buffer dataOut;	
    	dataIn.data=(byte *)szIn;
    	dataIn.len=strlen(szIn);
    	dataOut.data=szOut;
    	dataOut.len=512;

	struct stat statBlock;
	//if(stat(szInFile, &statBlock)<0) 
	if(stat("./rpcryptmp", &statBlock)<0) {
	   printf("Can't stat file\n");
            CloseCryptoRand();
	    return false;
	 }
	int fileSize= statBlock.st_size;
	int iRead= open("./rpcryptmp", O_RDONLY);
	if(iRead<0) {
           CloseCryptoRand();
	   printf("Can't open read file\n");
	   return false;
	 }
	if(read(iRead, dataIn.data, fileSize)<0) {
            CloseCryptoRand();
	    close(iRead);
            printf("bad read\n");
            return false;
	}
	dataIn.len= fileSize;	
	close(iRead);

        if(szKeyFile==NULL) {
            printf( "Keyfile missing\n");
	}
        iRet= EncryptSym(DECRYPT, szKeyFile, szIntKeyFile, &dataIn, &dataOut, szAlgo,szMode,szPad,szHmac);
        if(iRet==1) {
            printf( "Decrypt returned successfully\n");
            PrintBytes("symKeyIn.data=",dataOut.data,dataOut.len);
        }
        else
            printf( "Decrypt returned unsuccessfully\n");
        CloseCryptoRand();
    }

    if(iAction==RSAENCRYPT) {
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

        char szIn[]="000000000000000000000000000000000000000011111111111111111111111111111111111111122222222222222222222222222222222222222220123456789"; 
        //byte szIn[100];
        byte szOut[512];
	Data_buffer dataIn;
        Data_buffer dataOut;	
	dataIn.data=(byte *)szIn;
	dataIn.len=strlen(szIn);
	dataOut.data=szOut;
	dataOut.len=512;

        dataIn.data= (byte *)szInData;
        dataIn.len= strlen(szInData);

        //Function to read an RSA key file from an XML file - ToDo
        // ReadRSAKeyfromFile(const char* szKeyFile, RSAKeyPair*  pRSAKey);
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
        if (iRet) {
            pubKey.n.len= rsaKeyOut.n.len;
            memcpy(pubKey.n.data,rsaKeyOut.n.data, rsaKeyOut.n.len);
            pubKey.e.len= rsaKeyOut.e.len;
            memcpy(pubKey.e.data,rsaKeyOut.e.data, rsaKeyOut.e.len);

            priKey.d.len= rsaKeyOut.d.len;
            memcpy(priKey.d.data,rsaKeyOut.d.data, rsaKeyOut.d.len);

            priKey.n.len= rsaKeyOut.n.len;
            memcpy(priKey.n.data,rsaKeyOut.n.data, rsaKeyOut.n.len);
            //Write a routine like this to test
            //iRet= RSAEncryptTest(szRSAPad, &dataIn, &dataOut, &pubKey, &priKey);
            iRet= RSAEncrypt(szRSAPad, &dataIn, &dataOut, &pubKey);
            if(iRet==1)  {
                printf( "RSAEncryp returned successfully\n");
                PrintBytes("dataIn.data",dataIn.data,dataIn.len);
                printf( "dataOut.len = %d\n", dataOut.len);
                PrintBytes("dataOut.data",dataOut.data,dataOut.len);
	        int iWrite= open("./rpcryptmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	         if(iWrite<0) {
        	    CloseCryptoRand();
		    printf("Can't open write file to write encrypted data\n");
		    return -1;
    	          }
	          iRet= write(iWrite, dataOut.data, dataOut.len);
	          close(iWrite);
	    }
            else
                printf( "RSAEncryp returned unsuccessfully\n");
        } else printf("Keys could not be populated\n");
        CloseCryptoRand();
    } 

    if(iAction==RSADECRYPT) {
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

        char szIn[]="0011223344556677889900"; 
        //byte szIn[100];
        byte szOut[512]={0};
	Data_buffer dataIn;
        Data_buffer dataOut;	
	dataIn.data=(byte *)szIn;
	dataIn.len=strlen(szIn);
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

            priKey.d.len= keyPair.d.len;
            memcpy(priKey.d.data,keyPair.d.data, keyPair.d.len);

            priKey.p.len= keyPair.p.len;
            memcpy(priKey.p.data,keyPair.p.data, keyPair.p.len);

            priKey.q.len= keyPair.q.len;
            memcpy(priKey.q.data,keyPair.q.data, keyPair.q.len);

	    printf("Key lengths n = %d d= %d p = %d q=%d\n", keyPair.n.len,keyPair.d.len,keyPair.p.len, keyPair.q.len);
	    struct stat statBlock;
	    //if(stat(szInFile, &statBlock)<0) 
	    if(stat("./rpcryptmp", &statBlock)<0) {
	   	printf("Can't stat file\n");
        	CloseCryptoRand();
		return false;
	    }
	    int fileSize= statBlock.st_size;
	    int iRead= open("./rpcryptmp", O_RDONLY);
	    if(iRead<0) {
        	CloseCryptoRand();
	   	 printf("Can't open read file\n");
	    	 return false;
	    }

	    if(read(iRead, dataIn.data, fileSize)<0) {
        	CloseCryptoRand();
           	printf("bad read\n");
	        close(iRead);
            	return false;
	    }

	    dataIn.len= fileSize;	
	    close(iRead);

            iRet= RSADecrypt(szRSAPad, &dataIn, &dataOut, &priKey);
            if(iRet==1) {
                printf( "RSADecryp returned successfully\n");
                printf( "dataOut len = %d\n",dataOut.len);
                //PrintBytes("symKeyIn.data=",dataOut.data,dataOut.len);
                // Stack Smashing 
                PrintBytes("dataOut.data=",dataOut.data,dataIn.len);
                printf( "dataOut = %s\n",dataOut.data);
                PrintBytes("dataOut.data=",dataOut.data,dataOut.len);
                //printf( "dataOut = %s\n",dataOut.data);
            }
            else
                printf( "RSADecryp returned unsuccessfully\n");
        } else printf( "Could not read Keys\n");
        CloseCryptoRand();
    }

    if(iAction==DIGEST) {
	    printf( "szAlgorithm = %s \n", szAlgorithm);
        int col=64;
    	//char* szDigestType="Sha1";
    	//const char* szDigestType="SHA-256";
        // There are 129 ascii chars i.e 130 bytes. Does not include the null.
        // 40 zeroes + 40 1's, 40 2's + 10 0-9 digits.
        char szIn[]="0000000000000000000000000000000000000000111111111111111111111111111111111111111122222222222222222222222222222222222222220123456789"; 
        Data_buffer digest;
        Data_buffer data;
        byte buf[256];
        //typedef void* Digest_context;
        Digest_context ctx;

       // int  size= SHA256DIGESTBYTESIZE;

        data.data=(byte *)szIn;
        data.len=strlen(szIn);
	digest.data= buf;
	digest.len= 256;
        printf("Input String\n%s\n",  szIn);
	if (strcmp(szAlgorithm, "SHA-256")==0) {
        printf("Known Sha-256:\n 6a15ce057360eb3aeef38e4fa5b7ec292a418e134cb92cca43991944c31940fa\n");
        //printf("Known Sha-1  :\n b9c754dfb8510126dd655d52967e8833fc8d75f0\n");

    	//InitDigest((char *)szDigestType, &ctx);
    	Sha256InitDigest((char *)szAlgorithm, &ctx);
    	Sha256UpdateDigest(&ctx, &data);
    	Sha256GetDigest(&ctx, &digest);
	Sha256CloseDigest(&ctx);

        printf("RP Crypto Sha-256 Hash/Digest:\n");
    	for (ui= 0; ui<digest.len; ui++) {
            printf("%02x", digest.data[ui]);
             if((ui%col)==(col-1))
                printf("\n");
         }

        }
	if (strcmp(szAlgorithm, "SHA-1")==0) {
        printf("Known Sha-1  :\n b9c754dfb8510126dd655d52967e8833fc8d75f0\n");

	digest.len= 256;
    	Sha1InitDigest((char *)szAlgorithm, &ctx);
    	Sha1UpdateDigest(&ctx, &data);
    	Sha1GetDigest(&ctx, &digest);
	Sha1CloseDigest(&ctx);

        printf("RP Crypto Sha-1 Hash/Digest:\n");
    	for (ui= 0; ui<digest.len; ui++) {
            printf("%02x", digest.data[ui]);
             if((ui%col)==(col-1))
                printf("\n");
         }
       }


        printf("\n");
        Data_buffer InData;
        Data_buffer Sha256Digest;
        byte sha256buf[256];

        InData.data=(byte *)szIn;
        InData.len=strlen(szIn);

	if (strcmp(szAlgorithm, "SHA-256")==0) {
    	Sha256Digest.data= sha256buf;
    	Sha256Digest.len= 256;

        printf("SHA-256 Hash:\n");
    	iRet= GetSha256Digest(&InData, &Sha256Digest);
        if (iRet == 1) {
    	for (ui= 0; ui<Sha256Digest.len; ui++) {
            printf("%02x", Sha256Digest.data[ui]);
           if((ui%col)==(col-1))
             printf("\n");
           }
        } else printf("GetSha256Digest failed\n");

        }

        Data_buffer Sha1Digest;
        byte sha1buf[256];

	if (strcmp(szAlgorithm, "SHA-1")==0) {
        printf("\nSHA-1 Hash:\n");
    	Sha1Digest.data= sha1buf;
    	Sha1Digest.len= 256;
    	iRet= GetSha1Digest(&InData, &Sha1Digest);
        
        if (iRet == 1) {
    	for (ui= 0; ui<Sha1Digest.len; ui++) {
            printf("%02x", Sha1Digest.data[ui]);
           if((ui%col)==(col-1))
             printf("\n");
           }
        } else printf("GetSha1Digest failed\n");
	}
        CloseCryptoRand();
      }

    if(iAction==HMAC) {

	const char* szMACType="HMAC";

        Data_buffer mac;
        Data_buffer data;
        byte bufd[1000];
        byte bufm[256];
        byte bufk[256];

        data.data= bufd;
        data.len= 1000;

	mac.data= bufm;
	mac.len=256;

        SymKey symKey;
        symKey.data= bufk;
        symKey.len= 32;

        /*
        int col=32;
        iRet= ReadAESKeyfromFile(szKeyFile, &symKey);
        if(iRet==1)
            printf( "ReadAesKey returning successfully\n");
        else {
            printf( "ReadAesKey returning unsuccessfully\n");
            CloseCryptoRand();
            return -1;
        }

        printf("Input String - %s \n", szIn);
        printf("Key Bytes\n");
	for (ui= 0; ui<symKey.len; ui++) {
               printf("%02x", symKey.data[ui]);
               if((ui%col)==(col-1))
               printf("\n");
         }
        printf("\nBigE\n");
	for (ui= 0; ui<symKey.len; ui++) {
               printf("%02x", symKey.data[symKey.len - ui - 1]);
               if((ui%col)==(col-1))
               printf("\n");
         }


        printf("HMAC Bytes\n");
	GetMAC((char *)szMACType, &symKey, &data, &mac);
	for (ui= 0; ui<mac.len; ui++) {
        	printf("%02x", mac.data[ui]);
        	if((ui%col)==(col-1))
        	printf("\n");
        }
        */

        const char szD[]="The quick brown fox jumps over the lazy dog"; 
       	int szDlen = strlen(szD); 
        const char szK[]="key";
        unsigned int szKlen= strlen(szK);

       
        memcpy(symKey.data,szK,3); 
        symKey.len= szKlen;
        memcpy(data.data,szD,szDlen);
        data.len= szDlen;
	GetMAC((char *)szMACType, &symKey, &data, &mac);
        PrintBytes("SHA-256 MAC:0x", (byte*)mac.data, mac.len);
        printf("The above should match this to pass:\nSHA-256 MAC:0xf7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8\n");

        char mac2[256];
        bool fRet = hmac_sha256((byte*)szD,szDlen,(byte*)szK,szKlen,(byte *)mac2);
        if (fRet == 1) PrintBytes("SHA-256 MAC:0x", (byte*)mac2, (int)32);
        else printf("hmac_sha256 returned errors\n");

        CloseCryptoRand();
    }
    if(iAction==PKDF) {
        InitCryptoRand();
        InitBigNum();
        char szPassl[]="Thereis200four_s"; 
        char szSaltl[]="SaltySalt"; 
        byte szOut[256];
        byte szSalt[256];
    	Data_buffer dataIn;
        Data_buffer ssymKeyOut;	
        Data_buffer salt;	
    	dataIn.data=(byte *)szPassl;
    	dataIn.len=strlen(szPassl);
    	ssymKeyOut.data=szOut;
    	ssymKeyOut.len=256;
    	salt.data=(byte *)szSaltl;
    	salt.len=strlen(szSaltl);
	if (szSalt==NULL);
        //iRet= EncryptSym(ENCRYPT, szKeyFile, &dataIn, &dataOut, szAlgo,szMode,szPad,szHmac);
	//int DeriveKey(char* szKeyType, char* szMethod, Data_buffer *password, Data_buffer *salt, SymKey *keyOut,u32 iterations)
	iRet= DeriveKey(szKeyType, szMethod, &dataIn, &salt, &ssymKeyOut,iRounds);
        if(iRet==1)
            printf( "DeriveKey returning successfully\n");
        else {
            printf( "DeriveKey returning unsuccessfully\n");
            CloseCryptoRand();
            return -1;
        }
	PrintBytes("The generated key is: ", ssymKeyOut.data,ssymKeyOut.len);
        CloseCryptoRand();
    }
    if(iAction==BASE64) {
        InitCryptoRand();
        InitBigNum();

        char szOut[1024];
	int piOutLen;
	int iInLen;
	bool fDir;
	bool iDir;
	if(strcmp(szDir,"true")==0) { fDir=true; iDir=1;}
	if(strcmp(szDir,"false")==0) { fDir=false; iDir=0; }

        if(fDir); if (iDir); 
    	iInLen= strlen(szInData);
	piOutLen=1024;

	if (strcmp(szEnDec,"En") == 0) {
	PrintBytes("Input  Bytes- ",(byte*)szInData,iInLen);
	printf("Input  String- %s\n",szInData,iInLen);
	iRet= ToBase64(iInLen,(byte *)szInData,&piOutLen,szOut/*,iDir*/);
        if(iRet==1)
            printf("ToBase64 returning successfully\n");
        else {
            printf( "ToBase64 returning unsuccessfully\n");
            CloseCryptoRand();
            return -1;
        }
	PrintBytes("ToBase64 Bytes- ",(byte*)szOut,piOutLen);
        szOut[piOutLen]=0;
	printf("ToBase64 String- %s length= %d\n",szOut,piOutLen);
       }

         
	if (strcmp(szEnDec,"Dec")==0) {
        iRet= FromBase64(iInLen,(const char *)szInData, &piOutLen, (unsigned char *)szOut/*, iDir*/);
	if(iRet==1) printf("FromBase64 returning successfully\n"); 
	else {
            printf( "FromBase64 returning unsuccessfully\n");
            CloseCryptoRand();
            return -1;
	}
	PrintBytes("FromBase64 Bytes- ",(byte *)szOut,piOutLen);
        szOut[piOutLen]= 0;
	printf("FromBase64 String- %s length= %d\n",(byte *)szOut,piOutLen);
	}

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
            bufBase64[ibufBase64]=0;
            printf("Base 64 Encoded String- %s length =%d\n", bufBase64, ibufBase64 );
	}
        else
            printf( "GetBase64Rand returning unsuccessfully\n");
        CloseCryptoRand();
    }


    printf("\n");
    RPUtilExit();

    return 0;
}


// -------------------------------------------------------------------------

