//
//  File: rpapi.h
//      Vinay Phegade
//      
//
//  Description: RP Crypto APIs 
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

#ifndef __RPAPI_H__
#define __RPAPI_H__


/*
	This file describes library API to RP client. The functionality is divided into,
	1) Crypto API
		- Key generation
		- Digest
		- Signature
		- Encryption/Decryption
	2) Attestation and verification
	3) Trust Establishment
	4) Secure storage
	5) Utility functions
*/

/*
	AR: Describe buffer alloc/free convention.


*/


//typedefs
typedef unsigned char       byte;
typedef unsigned int        uint;
typedef unsigned char       u8;
typedef unsigned            u32;
typedef long long unsigned  u64;

#define BIGSYMKEYSIZE        64
#define NBITSINBYTE           8

#define BLOCK        1
#define STREAM       2
#define ASYMMETRIC   3

#define NOPAD        0
#define PKCSPAD      1
#define SYMPAD       2

#define RSANOPADDING         0
#define RSAPKCS1PADDING      1
#define RSAPKCS1OAEPPADDING  2

#define NOHASH       0
#define SHA256HASH   1
#define SHA1HASH     2
#define SHA512HASH   3
#define SHA384HASH   4
#define MD5HASH      5

#define NOMODE       0
#define ECBMODE      1
#define CBCMODE      2
#define CTRMODE      3
#define GCMMODE      4

#define NOALG        0
#define AES128       1
#define AES256       2
#define RSA1024      6
#define RSA2048      7

#define NOHMAC       0
#define HMACSHA256   1

#define NOKEYTYPE        0
#define AESKEYTYPE       1
#define RSAKEYTYPE       2


#define AES128BYTEBLOCKSIZE        16
#define AES128BYTEKEYSIZE          16
#define AES256BYTEBLOCKSIZE        16
#define AES256BYTEKEYSIZE          32

#define RSA1024BYTEKEYSIZE        128
#define RSA2048BYTEKEYSIZE        256

#define RSA1024BYTEBLOCKSIZE      128
#define RSA2048BYTEBLOCKSIZE      256

#define SHA1BLOCKBYTESIZE          64
#define SHA1DIGESTBYTESIZE         20
#define SHA256BLOCKBYTESIZE        64
#define SHA256DIGESTBYTESIZE       32
#define SHA512BLOCKBYTESIZE        64
#define SHA512DIGESTBYTESIZE       64


#define SMALLKEYSIZE            128
#define BIGKEYSIZE              512
#define KEYNAMEBUFSIZE          128
#define KEYTYPEBUFSIZE          128
#define BIGSYMKEYSIZE            64
#define SMALLSYMKEYSIZE          32
#define BIGBLOCKSIZE             32

#define AES128CBCSYMPADHMACSHA256  (AES128|(CBCMODE<<8)|(SYMPAD<<16)|(HMACSHA256<<24))
#define AES128GCM                  (AES128 | (GCMMODE<<8))

#define NOENCRYPT        	  0
#define DEFAULTENCRYPT   	  1

//
// Crypto API
//

	
// supported key types
	

/*
	This function is used to generate keys.
	- Symmetric keys, e.g. AES, are generated using random data acquired from RNG
	- An RSA public key consists of a modulus n, which is the product of two positive prime 
        integers p and q (i.e., n = pq), and a public key exponent e.  Thus, the RSA public key
        is the pair of values (n, e) and is utilized to verify digital signatures.  The size of
        an RSA key pair is typically considered to be the length of the modulus n in bits (nlen).
*/

typedef struct _data_buf {		
	unsigned int	len;
	byte		*data;
} Data_buffer;

typedef struct _data_buf SymKey;

typedef struct _rsa_key_pair {
	Data_buffer	n;	//modulus
	Data_buffer	e;	//public exponent
	Data_buffer	d;  	//private exponent 
	Data_buffer	p;	//prime 1
	Data_buffer	q;	//prime 2
} RSAKeyPair;

typedef struct _rsa_public_key {
	Data_buffer	n;	//modulus
	Data_buffer	e;	//public exponent
} RSAPublicKey;

typedef struct _rsa_private_key{
	Data_buffer	n;  //modulus
	Data_buffer	d;  //private exponent 
	Data_buffer	p;  //prime 1
	Data_buffer	q;  //prime 2
} RSAPrivateKey;

#ifdef __cplusplus
extern "C" {
#endif
int GenSymKey(const char* szKeyType, const char* szReserved, SymKey *keyOut);
int GenRSAKey(const char* szKeyType,  const char* szReserved, u64 pubExp, RSAKeyPair *keyOut);
int GenKeyFile(const char* szKeyType, const char* szOutFile);
#ifdef __cplusplus
}
#endif


/*
	This function is used to derive symmetric key from the another key e.g. password
	Key derivation functions (KDF) derive key material from another source of entropy 
	while preserving the entropy of the input and being one-way
*/
//int DeriveKey(char* szKeyType, char* szMethod, Data_buffer *keyIn, SymKey *keyOut);
#ifdef __cplusplus
extern "C" {
#endif
int DeriveKey(const char* szKeyType, const char* szMethod, Data_buffer *password, Data_buffer *salt, SymKey *keyOut,u32 iterations);
int GetPublicKeyFromCert(const char* szCertificate, RSAPublicKey* pubKey);
#ifdef __cplusplus
}
#endif
//
// Message digests/hashes
//
typedef void* Digest_context;

#ifdef __cplusplus
extern "C" {
#endif
int Sha256InitDigest(char* szDigestType, Digest_context *ctx);
int Sha256UpdateDigest(Digest_context *ctx, Data_buffer *data);
int Sha256GetDigest(Digest_context *ctx,  Data_buffer *digest);
int Sha256CloseDigest(Digest_context *ctx);
int Sha1InitDigest(char* szDigestType, Digest_context *ctx);
int Sha1UpdateDigest(Digest_context *ctx, Data_buffer *data);
int Sha1GetDigest(Digest_context *ctx,  Data_buffer *digest);
int Sha1CloseDigest(Digest_context *ctx);
	
int GetSha256Digest(Data_buffer *In,Data_buffer *Digest);
int GetSha1Digest(Data_buffer *In,Data_buffer *Digest);

//
// MAC - message authentication code
//
int GetMAC(char* szMACType, SymKey *key, Data_buffer *digest, Data_buffer *mac);
	
//
// Sign and verification
//

int Sign(const char* szAlgorithm, Data_buffer *digestIn, Data_buffer *sigOut, RSAPrivateKey* privKey);
int Verify(const char* szAlgorithm, Data_buffer *digestIn, Data_buffer *sigIn, RSAPublicKey* pubKey);
int SignFile(const char* szAlgorithm, const char* szInFileName, Data_buffer *sigOut, RSAPrivateKey* privKey);
int VerifyFile(const char* szAlgorithm, const char* szInFileName, Data_buffer *sigIn, RSAPublicKey* pubKey);

//
// Encryption/Decryption
//

int Encrypt(SymKey* key, SymKey* intKey,  Data_buffer * dataIn, Data_buffer *dataOut, 
                        uint mode, uint alg, uint pad, uint mac);
int Decrypt(SymKey* key, SymKey* intKey, Data_buffer * dataIn, Data_buffer *dataOut,
                    uint mode, uint alg, uint pad, uint mac);
int EncryptFile(SymKey* key, SymKey* intKey, const char* szInFileName, const char* szOutFileName,
                        uint mode, uint alg, uint pad, uint mac);
int DecryptFile(SymKey* key, SymKey* intKey, const char* szInFileName, const char* szOutFileName, 
                        uint mode, uint alg, uint pad, uint mac);
int RSAEncrypt(const char* szPKPadding, Data_buffer * dataIn, Data_buffer *dataOut, RSAPublicKey* pubKey);
int RSADecrypt(const char* szPKPadding, Data_buffer * dataIn, Data_buffer *dataOut, RSAPrivateKey* privKey);

//
// Random number generator
//
int InitCryptoRand();
int GetCryptoRandom(int iNumBits, byte* buf);
int CloseCryptoRand();

//
// Utility functions
//

int ToBase64(int inLen, const byte* pbIn, int* poutLen, char* szOut);
int FromBase64(int inLen, const char* szIn, int* poutLen, unsigned char* puOut);
int GetBase64Rand(int iBytes, byte* puR, int* pOutSize, char* szOut);

//
// Crypto Lib init/exit functions
//
//
int RPLibInit();
int RPLibExit();

// Big number initialization
void InitBigNum();

#if 0
// Identity attestation and verification
//

/*
Trusted Execution Environment (TEE) identify is defined as the hash of the code and data measured
by HW at the time TEE instance launch.  Attestation is this identity singed by private key held
in HW root of trust provider.

In case of TXT, The TPM is updated with a list of all the software measurements committed to the 
PCRs, and can then sign them with a private key known only by the TPM. Thus, a trusted client can
prove to a third party what software was launched in TEE.
	
*/

int GenerateQuote(Data_buffer *nonce, Data_buffer * challengeIn, Data_buffer *quoteOut);
int VerifyQuote( Data_buffer * quoteIn,  const char* szCertificate, int iCertFlag);
//
// Remote trust establishment: Establish shared secret with remote software program, after 
// verifying its identity 
//

int EstablishSessionKey (const char* szRemoteMachine, const char* method, int* nuKeysOut, Data_buffer **keys);

//
// Secure data storage
//

/*
During runtime of TEE, data can be protected by keeping it within the boundary (TCB).  To store
the secrets in persistent storage the data is encrypted by TEE in a way that it can be decrypted
by the same TEE identity on that machine.  Sealed data can be stored in the non-secure persistent
storage.

*/
int Seal(Data_buffer * secretIn, Data_buffer *dataOut);
int UnSeal(Data_buffer * dataIn, Data_buffer *secretOut);
int Attest(Data_buffer * dataIn, Data_buffer *dataOut);
#endif
#ifdef __cplusplus
}
#endif
#endif


// -------------------------------------------------------------------------------------


