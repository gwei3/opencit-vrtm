//
//  File: rpapilocal.h
//     
//
//  Description:  Modes and Padding
//
//  Copyright (c) 2011, Intel Corporation. Some contributions 
//    (c) John Manferdelli.  All rights reserved.
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

#ifndef __RPAPILOCAL_H__
#define __RPAPILOCAL_H__

// These are from rpapi.h. They need to be kept in sync.

#define RSANOPADDING         0
#define RSAPKCS1PADDING      1
#define RSAPKCS1OAEPPADDING  2

//typedef unsigned char       byte;
typedef unsigned int        uint;
//typedef unsigned char       u8;
//typedef unsigned            u32

typedef struct _data_buf {		
	unsigned int	len;
	byte		*data;
}Data_buffer;

typedef struct _data_buf SymKey;

typedef struct _rsa_key_pair {
	Data_buffer	n;	//modulus
	Data_buffer	e;	//public exponent
	Data_buffer	d; 	 //private exponent 
	Data_buffer	p;	//prime 1
	Data_buffer	q;	//prime 2
}RSAKeyPair;

typedef struct _rsa_public_key {
	Data_buffer	n;	//modulus
	Data_buffer	e;	//public exponent
}RSAPublicKey;

typedef struct _rsa_private_key{
	Data_buffer	n;	//modulus
	Data_buffer	d;  	//private exponent 
	Data_buffer	p;  	//prime 1
	Data_buffer	q;  	//prime 2
}RSAPrivateKey;

typedef void* Digest_context;

//Callable by C functions
extern "C" int GenSymKey(const char* szKeyType, const char* szReserved, SymKey *keyOut);
extern "C" int GenRSAKey(const char* szKeyType,  const char* szReserved, u64 pubExp, RSAKeyPair *keyOut);
extern "C" int GenKeyFile(const char* szKeyType, const char* szOutFile);
extern "C" int DeriveKey(const char* szKeyType, const char* szMethod, Data_buffer *password, Data_buffer *salt, SymKey *keyOut,u32 iterations);
extern "C" int GetPublicKeyFromCert(const char* szCertificate, RSAPublicKey* pubKey);
extern "C" int Sha256InitDigest(char* szDigestType, Digest_context *ctx);
extern "C" int Sha256UpdateDigest(Digest_context *ctx, Data_buffer *data);
extern "C" int Sha256GetDigest(Digest_context *ctx,  Data_buffer *digest);
extern "C" int Sha256CloseDigest(Digest_context *ctx);
extern "C" int Sha1InitDigest(char* szDigestType, Digest_context *ctx);
extern "C" int Sha1UpdateDigest(Digest_context *ctx, Data_buffer *data);
extern "C" int Sha1GetDigest(Digest_context *ctx,  Data_buffer *digest);
extern "C" int Sha1CloseDigest(Digest_context *ctx);
extern "C" int GetSha256Digest(Data_buffer *In,Data_buffer *Digest);
extern "C" int GetSha1Digest(Data_buffer *In,Data_buffer *Digest);
extern "C" int GetMAC(char* szMACType, SymKey *key, Data_buffer *digest, Data_buffer *mac);
extern "C" int Sign(const char* szAlgorithm, Data_buffer *digestIn, Data_buffer *sigOut, RSAPrivateKey* privKey);
extern "C" int Verify(const char* szAlgorithm, Data_buffer *digestIn, Data_buffer *sigIn, RSAPublicKey* pubKey);
extern "C" int SignFile(const char* szAlgorithm, const char* szInFileName, Data_buffer *sigOut, RSAPrivateKey* privKey);
extern "C" int VerifyFile(const char* szAlgorithm, const char* szInFileName, Data_buffer *sigIn, RSAPublicKey* pubKey);
extern "C" int Encrypt(SymKey* key, SymKey* intKey, Data_buffer * dataIn, Data_buffer *dataOut, 
			uint alg, uint mode,uint pad, uint mac);
extern "C" int Decrypt(SymKey* key, SymKey* intKey, Data_buffer * dataIn, Data_buffer *dataOut,
			uint alg, uint mode,uint pad, uint mac);
extern "C" int EncryptFile(SymKey* key, SymKey* intKey, const char* szInFileName, const char* szOutFileName,
                        uint alg, uint mode,uint pad, uint mac);
extern "C" int DecryptFile(SymKey* key, SymKey* intKey, const char* szInFileName, const char* szOutFileName, 
                        uint alg, uint mode,uint pad, uint mac);
extern "C" int RSAEncrypt(const char* szPKPadding, Data_buffer * dataIn, Data_buffer *dataOut, RSAPublicKey* pubKey);
extern "C" int RSAEncryptTest(const char* szPKPadding, Data_buffer * dataIn, Data_buffer *dataOut, RSAPublicKey* pubKey, RSAPrivateKey* privKey);
extern "C" int RSADecrypt(const char* szPKPadding, Data_buffer * dataIn, Data_buffer *dataOut, RSAPrivateKey* privKey);
extern "C" int InitCryptoRand();
extern "C" int GetCryptoRandom(int iNumBits, byte* buf);
extern "C" int CloseCryptoRand();
extern "C" int ToBase64(int inLen, const byte* pbIn, int* poutLen, char* szOut);
extern "C" int FromBase64(int inLen, const char* szIn, int* poutLen, unsigned char* puOut);
extern "C" int GetBase64Rand(int iBytes, byte* puR, int* pOutSize, char* szOut);
extern "C" int RPLibInit();
extern "C" int RPLibExit();
extern "C" void InitBigNum();
//extern "C" int GenerateQuote(Data_buffer *nonce, Data_buffer * challengeIn, Data_buffer *quoteOut);
//extern "C" int VerifyQuote( Data_buffer * quoteIn,  const char* szCertificate, int iCertFlag);
//extern "C" int EstablishSessionKey (const char* szRemoteMachine, const char* method, int* nuKeysOut, Data_buffer **keys);
//extern "C" int Seal(Data_buffer * secretIn, Data_buffer *dataOut);
//extern "C" int UnSeal(Data_buffer * dataIn, Data_buffer *secretOut);
//extern "C" int Attest(Data_buffer * dataIn, Data_buffer *dataOut);



#endif
// -------------------------------------------------------------------------------------


