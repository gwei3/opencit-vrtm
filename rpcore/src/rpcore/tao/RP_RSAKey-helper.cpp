//  File: RSAKey-helper.cpp
//      Huaiyu Liu, Andrew Reinders
//
//  Description:  RSA Helper functions to create openssl RSA keys and convert to 
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
//#include <time.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include "bignum.h"
#include "keys.h"
#include "mpFunctions.h"
#include "logging.h"


#include <iostream>
#include <fstream>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include "RP_RSAKey-helper.h"

#define ENTRIES 6
#define fatal(msg) fatal_error(__FILE__, __LINE__, msg)

// Parameter settings for this cert
//
#define RSA_KEY_SIZE (1024)
#define ENTRIES 6
//#define MAXLEN 80
//#define CONFIG_FILE "/etc/rp_cert_config" 


using namespace std;

////// Constants ///////////
//value for exponent, "e"
//The exponent is an odd number, typically 3, 17 or 65537. 
const int g_keyExp = 65537; //(1ULL<<16)+1ULL; 

////////////////////////////////
// Utility functions below
////////////////////////////////
extern void kpb(const char* szMsg, byte* pbData, int iSize);
extern void initBigNum();
 
// Function opensslRSAGenerateKeyPair -- calling openssl library to 
// create RSA keys of the given size
// Ref: // https://www.openssl.org/docs/crypto/RSA_generate_key.html 
// Note: caller of opensslRSAGenerateKeyPair is responsible to free 
// the memory of "rsa"
//
// TO double check: The pseudo-random number generator must be seeded 
// prior to calling RSA_generate_key_ex()?
// According to https://www.openssl.org/docs/crypto/RAND_add.html:
// On systems that provide /dev/urandom, the randomness device is used 
// to seed the PRNG transparently. However, on all other systems, the 
// application is responsible for seeding the PRNG by calling RAND_add(), 
// RAND_egd(3) or RAND_load_file(3).
//RSA * opensslRSAGenerateKeyPair(int kBits)
void opensslRSAGenerateKeyPair(int kBits, RSA *opensslRsaKey)
{
	bool created = false;
	//according to openssl docs online, RSA_generate_key is deprecated.
	//need to use  RSA_generate_key_ex instead.
	//RSA *rsaKey = RSA_new();
	BIGNUM *bnE = BN_new();
	
	//set value for exponent e
	if(BN_set_word(bnE, g_keyExp)){
		//create the key pair
		if(!RSA_generate_key_ex(opensslRsaKey, kBits, bnE, NULL))
			created = false;
		else
			created = true;
	}
	//Clean up the big number
	if(bnE)
		BN_free (bnE);
		
	//clean up on failure of creating keypair
	if(!created)
	{
		RSA_free(opensslRsaKey);
		opensslRsaKey = NULL;
	}
	//return rsaKey;
	return;
}

// In: RSA * rsaKey -- see "struct rsa_st" in openssl/rsa.h
// Out: char *pemKey -- the rsa key in PEM format
char * opensslRSAtoPEM(RSA * rsaKey)
{
	int klen;
	char *pemKey;

	if(rsaKey == NULL)
		return NULL;

	/* To convert to the PEM form: */
	BIO *bio = BIO_new(BIO_s_mem());
	PEM_write_bio_RSAPrivateKey(bio, rsaKey, NULL, NULL, 0, NULL, NULL);

	klen = BIO_pending(bio);
	//add null for termination
	int eleSize = sizeof(char);
	pemKey = (char*)calloc(klen+eleSize, eleSize); 
	BIO_read(bio, pemKey, klen);

	BIO_free_all(bio);
	return pemKey;
}

// In: RSA * rsaKey -- see "struct rsa_st" in openssl/rsa.h
// Out: char *pemKey -- the rsa key in PEM format
char * opensslRSAtoPubPEM(RSA * rsaKey)
{
        int klen;
        char *pemKey;

        if(rsaKey == NULL)
                return NULL;

        /* To convert to the PEM form: */
        BIO *bio = BIO_new(BIO_s_mem());
        PEM_write_bio_RSAPublicKey(bio, rsaKey);

        klen = BIO_pending(bio);
        //add null for termination
        int eleSize = sizeof(char);
        pemKey = (char*)calloc(klen+eleSize, eleSize);
        BIO_read(bio, pemKey, klen);

        BIO_free_all(bio);
        return pemKey;
}

//Andrew's code for data structure converstion,
//In: big num "in" as in openssl's BIGNUM definition (bn.h)
// typedef struct bignum_st
//	{
//		BN_ULONG *d;	/* Pointer to an array of 'BN_BITS2' bit chunks. */
//		int top;	/* Index of last used d +1. */
//		/* The next are internal book keeping for bn_expand. */
//		int dmax;	/* Size of the d array. */
//		int neg;	/* one if the number is negative */
//		int flags;
//	} BIGNUM;
//Out: bnum "out" as defined in fileProxy/Code/jlmbignum/bignum.h
//Return: the number of bytes in the bignum
int sslBN2bnum (BIGNUM* in, bnum* out) {
    int len, i, bufsize;
    unsigned char* tempbuf;
    len = BN_num_bytes(in);
    bufsize = ((len+7)/8)*8;

	//calling calloc instead of malloc to set memory to zero
    tempbuf = (unsigned char*)calloc(bufsize, sizeof(unsigned char));
    //for (i = 0; i < bufsize; i ++) tempbuf[i]=0;

    BN_bn2bin(in, tempbuf + (bufsize-len)); //returns big-endian, 4-byte-aligned binary representation
    
    //debug
    BN_print_fp(stdout, in);
   
#ifdef RSA_TEST
    printf("\nbufsize = %d\n", bufsize);
#endif 

    //out = new bnum(bufsize>>3); // bnum is 8-byte words
    //HL: I think we shall call bnum with bufsize, insteaed of bufsize>>3(or bufize/8)
    //maybe we don't need to the the fancy math below, but simply call 
    //revmemcpy function  -- this works well for little endian machines,
    //like Intel machines. But Andrew's code below in theory work for both
    //big endian/little endian machines.
    
    //revmemcpy((byte*)out->m_pValue, tempbuf, bufsize);
    
    // i -> bufsize - j - 1
    // bufsize - i - 1 -> j     
    for (i = 0; i < bufsize; ++i) {
        (out->m_pValue)[(bufsize-i-1)/8] += ((u64) tempbuf[i]) << (((bufsize-i-1)%8)*8); //for each byte, shift the byte over by how many
    }
   
#ifdef RSA_TEST    
    kpb("out ", (byte*)out->m_pValue, bufsize); 
    printf("end of out \n");
#endif
    
    if (in->neg) out->mpNegate();
    free(tempbuf);
    return bufsize;
}


//Andrew's code for data structure converstion,
//convert parameters M,P,Q,E,D 
// In: RSA* in, in openssl data structure
// Out: RSAKey* out), as defined in fileProxy/Code/jlmcrypto/keys.h
void sslRSA2RPRSA (RSA* in, RSAKey* out) { 	
	//out = new RSAKey(); 
	int bnSize=0;
	//printf("calling sslBN2bnum for n, m_iByteSizeM = %d\n", out->m_iByteSizeM);
	bnSize=sslBN2bnum(in->n, out->m_pbnM); 
	out->m_iByteSizeM = bnSize;
	
#ifdef RSA_TEST
	kpb("M ", (byte*)out->m_pbnM->m_pValue, out->m_iByteSizeM);
#endif	
	
	//printf("calling sslBN2bnum for P\n");
	bnSize=sslBN2bnum(in->p, out->m_pbnP); 
	out->m_iByteSizeP = bnSize;
	
	//printf("calling sslBN2bnum for Q\n");
	bnSize=sslBN2bnum(in->q, out->m_pbnQ); 
	out->m_iByteSizeQ = bnSize;
	
	//printf("calling sslBN2bnum for E\n");
	bnSize=sslBN2bnum(in->e, out->m_pbnE); 
	out->m_iByteSizeE = bnSize;
	
	//printf("calling sslBN2bnum for D\n");
	bnSize=sslBN2bnum(in->d, out->m_pbnD);
	out->m_iByteSizeD = bnSize; 
	//todo: compute fast parameters  -- done in RSAGenerateKeyPair_OpenSSL	
}

/////////////////////////////////////////////////////////////////////
// The main function RSAGenerateKeyPair_OpenSSL
// In: the size of the keys to be generated
// Out: opensslRsaKey -- the created key pair as in openssl's RSA 
//		data structure
// Out: pKey -- the data structure as efined in 
//		fileProxy/Code/jlmcrypto/keys.h, with the same key parameters 
//		as in opensslRsaKey
// Caller of RSAGenerateKeyPair_OpenSSL need to free opensslRsaKey 
//		and pKey
/////////////////////////////////////////////////////////////////////

bool RSAGenerateKeyPair_OpenSSL(int keySize, RSA *opensslRsaKey, RSAKey *pKey)
{
	int     ikeyByteSize= 0;
    int     ikeyu64Size= 0;
    initBigNum(); 
    if(keySize==1024) {
        ikeyu64Size= 16;
        ikeyByteSize= 128;
    }
    else if(keySize==2048) {
        ikeyu64Size= 32;
        ikeyByteSize= 256;
    }
    else
        return false;
        
	//Step 1: calling openssl library to create RSA key, in openssl's format
	//opensslRsaKey = (RSA *)malloc(sizeof(RSA *));

	//opensslRsaKey = opensslRSAGenerateKeyPair(keySize);	
	//RSA *rsaKey = RSA_new();
	//opensslRsaKey = RSA_new();
	opensslRSAGenerateKeyPair(keySize, opensslRsaKey);
	//opensslRsaKey = rsaKey;
	if (opensslRsaKey == NULL)
	{
		pKey = NULL;
		return false;
	}
	
	//Step 2: convert and copy parameters in openssl rsakey to the RSAKey 
	//used in RPCore's code base
	
	//Initialization
	//RSAKey*  pKey= new RSAKey();
    if(keySize==1024) {
        pKey->m_ukeyType= RSAKEYTYPE;
        pKey->m_uAlgorithm= RSA1024;
        pKey->m_ikeySize= 1024;
    }
    else if(keySize==2048) {
        pKey->m_ukeyType= RSAKEYTYPE;
        pKey->m_uAlgorithm= RSA2048;
        pKey->m_ikeySize= 2048;
    }
    pKey->m_rgkeyName[0]= 0;
    pKey->m_ikeyNameSize= 0;
    
    //TODO: the sizes probably shall not be set here, but set while
    //calling sslRSA2RPRSA
    pKey->m_iByteSizeM= ikeyByteSize;
    pKey->m_iByteSizeD= ikeyByteSize;
    pKey->m_iByteSizeE= 4*sizeof(u64);
    pKey->m_iByteSizeP= ikeyByteSize/2;
    pKey->m_iByteSizeQ= ikeyByteSize/2;
    
    pKey->m_pbnM= new bnum(ikeyu64Size);
    pKey->m_pbnP= new bnum(ikeyu64Size/2);
    pKey->m_pbnQ= new bnum(ikeyu64Size/2);
    pKey->m_pbnE= new bnum(4);
    pKey->m_pbnD= new bnum(ikeyu64Size);
    
    pKey->m_pbnPM1= new bnum(ikeyu64Size/2);
    pKey->m_pbnQM1= new bnum(ikeyu64Size/2);
    
    //Convert and copy parameters M, P, Q, E, D from the openssl rsakey
	sslRSA2RPRSA(opensslRsaKey, pKey);
	
#ifdef RSA_TEST
	//debug
	printf("after sslRSA2RPRSA, pKey sizes: m_iByteSizeM=%d, m_iByteSizeD=%d, m_iByteSizeE=%d, m_iByteSizeP=%d, m_iByteSizeQ=%d\n", 
	pKey->m_iByteSizeM, pKey->m_iByteSizeD, pKey->m_iByteSizeE, pKey->m_iByteSizeP, pKey->m_iByteSizeQ);
#endif

	char * serializedPubKey = pKey->SerializePublictoString();
	
#ifdef RSA_TEST	
	printf("after sslRSA2RPRSA, serialized pub key in RP format: \n %s\n", serializedPubKey);
#endif
	
	//Populate other fields in the RSAKey data structure
	memcpy(pKey->m_rgbM,(byte*)pKey->m_pbnM->m_pValue, pKey->m_iByteSizeM);
    memcpy(pKey->m_rgbP,(byte*)pKey->m_pbnP->m_pValue, pKey->m_iByteSizeP);
    memcpy(pKey->m_rgbQ,(byte*)pKey->m_pbnQ->m_pValue, pKey->m_iByteSizeQ);
    memcpy(pKey->m_rgbE,(byte*)pKey->m_pbnE->m_pValue, pKey->m_iByteSizeE);
    memcpy(pKey->m_rgbD,(byte*)pKey->m_pbnD->m_pValue, pKey->m_iByteSizeD);


	bnum       bnDP(128);
    bnum       bnDQ(128);
    bnum       bnPM1(128);
    bnum       bnQM1(128);
    
	if(!mpRSACalculateFastRSAParameters(*(pKey->m_pbnE), *(pKey->m_pbnP), *(pKey->m_pbnQ), bnPM1, bnDP, bnQM1, bnDQ)) {
    //if(!mpRSACalculateFastRSAParameters(bnE, bnP, bnQ, bnPM1, bnDP, bnQM1, bnDQ)) {
        fprintf(g_logFile, "Cant generate fast rsa parameters\n");
    }
    else {
        pKey->m_iByteSizeDP= ikeyByteSize/2;
        pKey->m_iByteSizeDQ= ikeyByteSize/2;
        pKey->m_pbnDP= new bnum(ikeyu64Size/2);
        pKey->m_pbnDQ= new bnum(ikeyu64Size/2);

        memcpy(pKey->m_pbnDP->m_pValue,(byte*)bnDP.m_pValue, pKey->m_iByteSizeDP);
        memcpy(pKey->m_pbnDQ->m_pValue,(byte*)bnDQ.m_pValue, pKey->m_iByteSizeDQ);

        memcpy(pKey->m_rgbDP,(byte*)bnDP.m_pValue, pKey->m_iByteSizeDP);
        memcpy(pKey->m_rgbDQ,(byte*)bnDQ.m_pValue, pKey->m_iByteSizeDQ);
        memcpy(pKey->m_rgbDP,(byte*)bnDP.m_pValue, pKey->m_iByteSizeDP);
        memcpy(pKey->m_rgbDQ,(byte*)bnDQ.m_pValue, pKey->m_iByteSizeDQ);

        if(pKey->m_iByteSizeDP>0 &&  pKey->m_iByteSizeDQ>0 && 
                   pKey->m_pbnPM1!=NULL && pKey->m_pbnQM1!=NULL) {
            mpSub(*(pKey->m_pbnP), g_bnOne, *(pKey->m_pbnPM1));
            mpSub(*(pKey->m_pbnQ), g_bnOne, *(pKey->m_pbnQM1));
        }

    }
	
	//free(opensslRsaKey);
	return pKey;
} 


int public_encrypt(char * data,int data_len, RSA* rsa, unsigned char *encrypted, int padding)
{
    unsigned char udata[2048/8];
    memcpy(udata, data, sizeof(data));
    int maxSize = RSA_size(rsa);
    int result = RSA_public_encrypt(maxSize,udata,encrypted,rsa,padding);
    return result;
}
int private_decrypt(unsigned char * enc_data,int data_len, RSA* rsa, unsigned char *decrypted, int padding)
{
    int  result = RSA_private_decrypt(data_len,enc_data,decrypted,rsa,padding);
    return result;
}
void fatal_error(const char *file, int line, const char *msg)
{
    fprintf(stderr, "**FATAL** %s:%i %s\n", file, line, msg);
    ERR_print_errors_fp(stderr);
    exit(-1);
}
/*
struct sample_parameters
{
  char countryName[MAXLEN];
  char stateOrProvinceName[MAXLEN];
  char localityName[MAXLEN];
  char organizationName[MAXLEN];
  char organizationalUnitName[MAXLEN];
  char commonName[MAXLEN];
  char ca_cert_chain_path[MAXLEN];
}
sample_parameters;
*/
void init_parameters (struct sample_parameters * parms)
{
  strncpy (parms->countryName, "IN", MAXLEN);
  strncpy (parms->stateOrProvinceName, "MH", MAXLEN);
  strncpy (parms->localityName, "PUNE", MAXLEN);
  strncpy (parms->organizationName, "GSL", MAXLEN);
  strncpy (parms->organizationalUnitName, "IDM", MAXLEN);
  strncpy (parms->commonName, "GSLAB", MAXLEN);
  strncpy (parms->ca_cert_chain_path, "", MAXLEN);
}

char *trim (char * s)
{
  /* Initialize start, end pointers */
  char *s1 = s, *s2 = &s[strlen (s) - 1];

  /* Trim and delimit right side */
  while ( (isspace (*s2)) && (s2 >= s1) )
    s2--;
  *(s2+1) = '\0';

  /* Trim left side */
  while ( (isspace (*s1)) && (s1 < s2) )
    s1++;
  /* Copy finished string */
  strcpy (s, s1);
  return s;
}

void parse_config (struct sample_parameters * parms)
{
  char *s, buff[256];
  FILE *fp = fopen (CONFIG_FILE, "r");
  if (fp == NULL)
  {
    return;
  }

  /* Read next line */
  while ((s = fgets (buff, sizeof buff, fp)) != NULL)
  {
    /* Skip blank lines and comments */
    if (buff[0] == '\n' || buff[0] == '#')
      continue;

    /* Parse name/value pair from line */
    char name[MAXLEN], value[MAXLEN];
    s = strtok (buff, "=");
    if (s==NULL)
      continue;
    else
      strncpy (name, s, MAXLEN);
    s = strtok (NULL, "=");
    if (s==NULL)
      continue;
    else
      strncpy (value, s, MAXLEN);
    trim (value);

    /* Copy into correct entry in parameters struct */
    if (strcmp(name, "countryName")==0)
      strncpy (parms->countryName, value, MAXLEN);
    else if (strcmp(name, "stateOrProvinceName")==0)
      strncpy (parms->stateOrProvinceName, value, MAXLEN);
    else if (strcmp(name, "localityName")==0)
      strncpy (parms->localityName, value, MAXLEN);
    else if (strcmp(name, "organizationName")==0)
      strncpy (parms->organizationName, value, MAXLEN);
    else if (strcmp(name, "organizationalUnitName")==0)
      strncpy (parms->organizationalUnitName, value, MAXLEN);
    else if (strcmp(name, "commonName")==0)
      strncpy (parms->commonName, value, MAXLEN);
    else if (strcmp(name, "ca_cert_chain_path")==0)
      strncpy (parms->ca_cert_chain_path, value, MAXLEN);
    else
      printf ("WARNING: %s/%s: Unknown name/value pair!\n",
        name, value);
  }

  /* Close file */
  fclose (fp);
}


char * create_csr(RSA *rsakey)
{
  struct entry
  {
    char *key;
    char *value;
  };

  //reading subject from configuration file to create csr 
  struct sample_parameters parms;

  //printf ("Initializing parameters to default values...\n");
  init_parameters (&parms);

  //printf ("Reading config file...\n");
  parse_config (&parms);

  //printf ("Final values:\n");
  //printf (" countryName : %s, stateOrProvinceName : %s, localityName : %s, organizationName : %s, organizationalUnitName : %s, commonName : %s \n ",
  //parms.countryName, parms.stateOrProvinceName, parms.localityName, parms.organizationName, parms.organizationalUnitName, parms.commonName);

  struct entry entries[ENTRIES] =
  {
    { "countryName", parms.countryName },
    { "stateOrProvinceName", parms.stateOrProvinceName },
    { "localityName", parms.localityName },
    { "organizationName", parms.organizationName },
    { "organizationalUnitName", parms.organizationalUnitName },
    { "commonName", parms.commonName },
  };

    int i;
    X509_REQ *req;
    X509_NAME *subj;
    EVP_PKEY *pkey;
    EVP_MD *digest;
    //FILE *fp;
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    if (!(pkey = EVP_PKEY_new())){
        fatal("Could not create EVP object");
	return false;
    }
    if (!(EVP_PKEY_set1_RSA(pkey, rsakey))){
        fatal("Could not assign RSA key to EVP object");
	return false;
    }
    // create request object
    //
    if (!(req = X509_REQ_new())){
        fatal("Failed to create X509_REQ object");
	return false;
    }
    X509_REQ_set_pubkey(req, pkey);

    // create and fill in subject object
    //
    if (!(subj = X509_NAME_new())){
        fatal("Failed to create X509_NAME object");
	return false;
    }

    for (i = 0; i < ENTRIES; i++)
    {
        int nid;                  // ASN numeric identifier
        X509_NAME_ENTRY *ent;

        if ((nid = OBJ_txt2nid(entries[i].key)) == NID_undef)
        {
            fprintf(stderr, "Error finding NID for %s\n", entries[i].key);
            fatal("Error on lookup");
        }
        if (!(ent = X509_NAME_ENTRY_create_by_NID(NULL, nid, MBSTRING_ASC,
            (unsigned char*)entries[i].value, - 1)))
            fatal("Error creating Name entry from NID");

        if (X509_NAME_add_entry(subj, ent, -1, 0) != 1)
            fatal("Error adding entry to Name");
	 }
    if (X509_REQ_set_subject_name(req, subj) != 1){
        fatal("Error adding subject to request");
	return false;
    }

    // request is filled in and contains our generated public key;
    // now sign it
    //
    digest = (EVP_MD *)EVP_sha1();

    if (!(X509_REQ_sign(req, pkey, digest))){
        fatal("Error signing request");
	return false;		
    }
    // write output files
    //
    //if (!(fp = fopen(csr_file, "w")))
    //    fatal("Error writing to request file");

    BIO *bio = BIO_new(BIO_s_mem());
    char *pem = NULL;

    if(PEM_write_bio_X509_REQ(bio,req) != 0){
	pem = (char *) malloc(bio->num_write + 1);
    	if (NULL == pem) {
        	BIO_free(bio);
		free(pem);
	        return false;    
    	}
	memset(pem, 0, bio->num_write + 1);
	BIO_read(bio, pem, bio->num_write);
	BIO_free(bio);
	printf("PEM:%s", pem);
	//int csr_length = strlen(pem);
	//len = &csr_length;
    }
    else{
	fatal("Error in PEM_write_bio_X509_REQ");
    }
    /*
    if (PEM_write_X509_REQ(fp, req) != 1)
        fatal("Error while writing request");
    fclose(fp);*/

    /*
    if (!(fp = fopen(private_key, "w")))
        fatal("Error writing to private key file");
    if (PEM_write_PrivateKey(fp, pkey, NULL, NULL, 0, 0, NULL) != 1)
        fatal("Error while writing private key");
    fclose(fp);
	*/

    EVP_PKEY_free(pkey);
    X509_REQ_free(req);
    BIO_free(bio);
    return pem;
}

