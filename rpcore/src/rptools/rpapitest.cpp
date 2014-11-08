//
//  File: rpapitest.cpp
//
//  Description: rp crypto api wrappers test driver.
//               Currently will  have all the test cases here
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
#include "rpapitest.h"
#include <iostream>
#include <cstdio>

//const char*   szAlgo= NULL;
//const char*   szMode= NULL;
//const char*   szPad= NULL;
//const char*   szRSAPad= NULL;
//const char*   szHmac= NULL;

//const char*   szInFile= NULL;
const char*   szKeyType= NULL;
const char*   szOutFile= NULL;
//const char*   szAlgorithm= NULL;
//const char*   szKeyFile= NULL;
int    iRet;
//int    iTestCase=1;;
unsigned int    col= 32;
unsigned int    i;

void PrintBytes(byte* pData, int iSize)
{
    for (int i= 0; i<iSize; i++) {
        printf("%02x", pData[i]);
    }
    printf("\n");
}

bool RPTest1()
{

    //T1:
    printf( "Key Generation in a file------------------------------------------------------------------\n");
    szKeyType= "AES128";
    szOutFile= "F1";
    iRet= GenKeyFile(szKeyType, szOutFile);
    if(iRet==1)
         printf( "GenKeyFile returning successfully\n");
    else {
		printf( "GenKeyFile returning unsuccessfully\n");
		return 0;
	}

    printf( "\n");
    return 1;
}
bool RPTest2()
{

    //T2:
    printf( "\n");
    printf( "AES Key Generation in a buffer----------------------------------------------------------\n");
    //Sym Key Generation in buffer
    const char* reserved=NULL;
    u8              buf[32];
    SymKey symKeyOut;
    symKeyOut.data= buf;

    szKeyType= "AES128";
    iRet= GenSymKey(szKeyType, reserved, &symKeyOut);
    if(iRet==1)
         printf( "GenSymKey returning successfully\n");
    else {
		printf( "GenKeyFile returning unsuccessfully\n");
		return 0;
	}

    printf("Printing AES Key Bytes\n");
    for (i= 0; i<symKeyOut.len; i++) {
        printf("%02x", symKeyOut.data[i]);
        if((i%col)==(col-1))
            printf("\n");
    }
    return 1;
}

bool RPTest3()
{
    //Tn:
return 0;
}

void TestSymKeyGen()
{
const char* szKeyType=NULL;
const char* szReserved=NULL;
u8     buf[32];
SymKey keyOut;
keyOut.data= buf;

int iRet;
iRet = GenSymKey(szKeyType, szReserved, &keyOut);
if(iRet==1) {
	PrintBytes(buf, AES128BYTEKEYSIZE);
	printf( "GenSymKey returning successfully\n");
 }
 else
	printf( "GenSymKey returning unsuccessfully\n");

}

void TestUnimplementedAPIs()
{
const char *szCertificate=NULL;
const char *szCertificates=NULL;
const char *szRemoteMachine=NULL;
const char *method=NULL;
RSAPublicKey pubKey;
Data_buffer  challengeIn, quoteOut;
Data_buffer quoteIn;
int nuKeysOut;
Data_buffer *keys;
Data_buffer secretIn, dataOut;
Data_buffer dataIn,secretOut, nonce;
int iRet;

//int GetPublicKeyFromCert(const char* szCertificate, RSAPublicKey* pubKey);
//int GenerateQuote(Data_buffer * challengeIn, Data_buffer *quoteOut);
//int VerifyQuote( Data_buffer * quoteIn,  const char* szCertificates);
//int EstablishSessionKey (const char* szRemoteMachine, const char* method, int* nuKeysOut, Data_buffer **keys);
//int Seal(Data_buffer * secretIn, Data_buffer *dataOut);
//int UnSeal(Data_buffer * dataIn, Data_buffer *secretOut);
/*
iRet= GetPublicKeyFromCert(szCertificate,  &pubKey);
if(iRet==-99) printf("Unimplemnetd GetPublicKeyFromCert Passed iRet= %d\n",iRet);
iRet= GenerateQuote(&nonce, &challengeIn, &quoteOut);
if(iRet==-99) printf("Unimplemnetd GenerateQuote Passed iRet= %d\n",iRet);
iRet= VerifyQuote(&quoteIn,  szCertificates, 0);
if(iRet==-99) printf("Unimplemnetd VerifyQuote Passed iRet= %d\n",iRet);
iRet=  EstablishSessionKey (szRemoteMachine, method, &nuKeysOut, &keys);
if(iRet==-99) printf("Unimplemnetd EstablishSessionKey Passed iRet= %d\n",iRet);
iRet= Seal(&secretIn, &dataOut);
if(iRet==-99) printf("Unimplemnetd Seal Passed iRet= %d\n",iRet);
iRet= UnSeal(&dataIn, &secretOut);
if(iRet==-99) printf("Unimplemnetd UnSeal Passed iRet= %d\n",iRet);
* */
}

int main(int an, char** av)
{

   
    // Main driver for testing 
    if(RPLibInit() != 1) {
	std::cout<< "RP lib initialization failure\n";
        return -1;
    }
    InitCryptoRand();
    InitBigNum();

    //Read the state table to be executed if tests can be structred this way.
    //ToDo Follow established test patterns if possible.
    // Placeholder for a Test case to be called.

    //TestUnimplementedAPIs();
    TestSymKeyGen();
    CloseCryptoRand();
    RPLibExit();

}
