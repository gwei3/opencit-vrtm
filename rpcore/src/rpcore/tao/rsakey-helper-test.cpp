//  Description:  Test : RSA Helper functions to create openssl RSA keys and convert to 
//				  the RSAKey data structure used in RPCore
//
//  Copyright (c) 2014, Intel Corporation. All rights reserved.
//  Some contributions (c) John Manferdelli.  All rights reserved.
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
/////////
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
#define INT32_MAX       (2147483647)
#endif

////////
#include <string.h>

#include <iostream>
#include <fstream>

////////
#include <string.h>
//#include <time.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include "jlmcrypto.h"
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include "RP_RSAKey-helper.h"
using namespace std;

void test_pem_key(char *pemKey)
{
	ofstream myfile;
        myfile.open ("/tmp/private.pem");
        myfile << pemKey;
        myfile.close();
        system("echo intelrp > /tmp/txt");
        system("openssl rsa -in /tmp/private.pem -out /tmp/public.pem -outform PEM -pubout");
        system("openssl rsautl -encrypt -inkey /tmp/public.pem -pubin -in /tmp/txt -out /tmp/txt_enc");
        system("openssl rsautl -decrypt -inkey /tmp/private.pem -in /tmp/txt_enc -out /tmp/txt_dec");
        printf("\n Openssl pem file verification: \n\t RSA PRIVATE PEM KEY: /tmp/private.pem \n\t Plain Text File: /tmp/txt \n\t Encrypted File: /tmp/txt_enc \n\t Decrypted Text File: /tmp/txt_dec ");
}

void test_openssl_keypair(RSA *opensslRsaKey)
{
	char plainText[] = "Pravin";
        unsigned char  encrypted[2048]={};
        unsigned char decrypted[2048]={};
        int padding = RSA_NO_PADDING;
        //int padding = RSA_PKCS1_PADDING;
        printf("\n\nEncrypting with openssl function:");
        printf("\n \tPlaintext: %s",plainText);
        int encrypted_length= public_encrypt(plainText,strlen(plainText),opensslRsaKey,encrypted, padding);
        if(encrypted_length == -1){
                printf("Public Encrypt failed ");
        }
        printf("\n \tEncrypted length =%d",encrypted_length);
        int decrypted_length = private_decrypt(encrypted,encrypted_length,opensslRsaKey,decrypted, padding);
        if(decrypted_length == -1){
                printf("Private Decrypt failed ");
                }
        printf("\n\nDecryption with openssl function:");
        printf("\n \tDecrypted Text =%s",decrypted);
        printf("\n \tDecrypted Length =%d",decrypted_length);

}

void test_rpcore_keypair(RSAKey *rpKey)
{
	RSAKey *pKey=new RSAKey();
        pKey=rpKey;

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
	

    char plainText[] = "Pravin";
    const char* szRSAPad="RSA_NO_PADDING";
    byte szEn[2048];
    Data_buffer dataIn;
    Data_buffer Encrypted;
    dataIn.data= (byte *)plainText;
    dataIn.len=strlen(plainText);
    printf("\n \tPlaintext: %s",plainText);
    Encrypted.data= szEn;
    Encrypted.len= 2048;

    pubKey.n.len=pKey->m_iByteSizeM;
    memcpy(pubKey.n.data,(u8*)pKey->m_pbnM->m_pValue, pKey->m_iByteSizeM);

    pubKey.e.len=pKey->m_iByteSizeE;
    memcpy(pubKey.e.data,(u8*)pKey->m_pbnE->m_pValue, pKey->m_iByteSizeE);

    int iRet= RSAEncrypt(szRSAPad, &dataIn, &Encrypted, &pubKey);
    if (iRet !=1 ) {
        #ifdef TEST
        fprintf(g_logFile, "RSAEncrypt iRet= %d\n", iRet);
        fprintf(g_logFile, "Failed in RSAEncrypt\n");
                #endif

    }
	

    //Creating RPCore private key structure
    byte szDec[2048];
    Data_buffer Decrypted;
    Decrypted.len= 2048;
    Decrypted.data= szDec;
    priKey.n.len= pKey->m_iByteSizeM;
    memcpy(priKey.n.data,(u8*)pKey->m_pbnM->m_pValue, pKey->m_iByteSizeM);
    priKey.d.len= pKey->m_iByteSizeD;
    memcpy(priKey.d.data,(u8*)pKey->m_pbnD->m_pValue, pKey->m_iByteSizeD);
    priKey.p.len=  pKey->m_iByteSizeP;
    memcpy(priKey.p.data,(u8*)pKey->m_pbnP->m_pValue, pKey->m_iByteSizeP);
    priKey.q.len= pKey->m_iByteSizeQ;
    memcpy(priKey.q.data,(u8*)pKey->m_pbnQ->m_pValue, pKey->m_iByteSizeQ);
    printf("\n Calling RSADecrypt function of RP Crypto API for decryption \n\n ");
    iRet = RSADecrypt(szRSAPad, &Encrypted, &Decrypted, &priKey);
    if (iRet !=1 ) {
#ifdef TEST
        fprintf(g_logFile, "RSADecrypt iRet= %d\n", iRet);
        fprintf(g_logFile, "Failed in RSADecrypt\n");
#endif
    }
    if (memcmp(plainText, Decrypted.data, strlen(plainText)) != 0) {
        printf("\n Failed in Enc/Decr\n");
    }
    else{
        printf("\n \tDecrypted Data : %s", Decrypted.data);
        printf("\n \tDecrypted Length =%d\n",Decrypted.len);
        printf("\n Encrypted - Decryption Successful \n");
    }


}

void test_openssl_rpcore_key(RSA *opensslRsaKey,RSAKey *rpKey)
{
	 //Encrypting Text with openssl pubkey and decrypting with openssl private key
        char plainText[] = "Pravin";
        unsigned char  encrypted[2048]={};
        int padding = RSA_NO_PADDING;
        //int padding = RSA_PKCS1_PADDING;
        printf("\n\nEncrypting with openssl function:");
        printf("\n \tPlaintext: %s",plainText);
        int encrypted_length= public_encrypt(plainText,strlen(plainText),opensslRsaKey,encrypted, padding);
        if(encrypted_length == -1){
                printf("Public Encrypt failed ");
        }
	printf("\n\n Now Trying to decrypt with RPCore formatted private key:");

        RSAKey *pKey=new RSAKey();
        pKey=rpKey;
        RSAPrivateKey priKey;

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

	Data_buffer Encrypted;
    byte szEn[2048];
    Encrypted.data= szEn;
    Encrypted.len= 2048;
    memcpy(Encrypted.data, encrypted, sizeof(encrypted));
    Encrypted.len=encrypted_length;

    //Creating RPCore private key structure
    const char* szRSAPad="RSA_NO_PADDING";
    byte szDec[2048];
    Data_buffer Decrypted;
    Decrypted.len= 2048;
    Decrypted.data= szDec;
    priKey.n.len= pKey->m_iByteSizeM;
    memcpy(priKey.n.data,(u8*)pKey->m_pbnM->m_pValue, pKey->m_iByteSizeM);
    priKey.d.len= pKey->m_iByteSizeD;
    memcpy(priKey.d.data,(u8*)pKey->m_pbnD->m_pValue, pKey->m_iByteSizeD);
    priKey.p.len=  pKey->m_iByteSizeP;
    memcpy(priKey.p.data,(u8*)pKey->m_pbnP->m_pValue, pKey->m_iByteSizeP);
    priKey.q.len= pKey->m_iByteSizeQ;
    memcpy(priKey.q.data,(u8*)pKey->m_pbnQ->m_pValue, pKey->m_iByteSizeQ);
    printf("\n Calling RSADecrypt function of RP Crypto API for decryption \n\n ");
    int iRet;
    iRet = RSADecrypt(szRSAPad, &Encrypted, &Decrypted, &priKey);
    if (iRet !=1 ) {
#ifdef TEST
        fprintf(g_logFile, "RSADecrypt iRet= %d\n", iRet);
        fprintf(g_logFile, "Failed in RSADecrypt\n");
#endif
    }
    if (memcmp(plainText, Decrypted.data, strlen(plainText)) != 0) {
        printf("\n Failed in Enc/Decr\n");
    }
    else{
        printf("\n \tDecrypted Data : %s", Decrypted.data);
        printf("\n \tDecrypted Length =%d\n",Decrypted.len);
        printf("\n Encrypted - Decryption Successful \n");
    }


}
int main(int an, char** av)
{

	RSA * opensslRsaKey=RSA_new();
	RSAKey*  rpKey= new RSAKey();	
	RSAGenerateKeyPair_OpenSSL(1024, opensslRsaKey, rpKey);
	create_csr(opensslRsaKey, "/tmp/sample.csr","/tmp/sample_private.key");
	char * pemKey = opensslRSAtoPEM(opensslRsaKey);
	if(pemKey != NULL){
		printf("\n This is RSA PRIVATE PEM KEY \n");
		printf(pemKey);
	}
	printf("\n***********************Testing PEM KEY*********************************\n");
	test_pem_key(pemKey);
	printf("\n***********************Testing opensslRsaKey KEY*********************************\n");
	test_openssl_keypair(opensslRsaKey);
	printf("\n***********************Testing RPCORE KEYS*********************************\n");
	test_rpcore_keypair(rpKey);
	printf("\n***********************Testing OPENSSL-RPCORE KEYS*********************************\n");
	test_openssl_rpcore_key(opensslRsaKey,rpKey);
	free(opensslRsaKey);
}

