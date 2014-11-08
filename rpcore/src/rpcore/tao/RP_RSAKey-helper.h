#include "jlmTypes.h"
#include "jlmcrypto.h"
#include <sys/types.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
using namespace std;

#define MAXLEN 80
#define CONFIG_FILE "/tmp/rptmp/config/rp_cert_config"

struct sample_parameters
{
  char countryName[MAXLEN];
  char stateOrProvinceName[MAXLEN];
  char localityName[MAXLEN];
  char organizationName[MAXLEN];
  char organizationalUnitName[MAXLEN];
  char commonName[MAXLEN];
  char ca_cert_chain_path[MAXLEN];
};


bool RSAGenerateKeyPair_OpenSSL(int keySize, RSA *opensslRsaKey, RSAKey *pKey);
void opensslRSAGenerateKeyPair(int kBits, RSA *opensslRsaKey);
char * create_csr(RSA *rsakey);
int public_encrypt(char * data,int data_len, RSA* rsa, unsigned char *encrypted, int padding);
int private_decrypt(unsigned char * enc_data,int data_len, RSA* rsa, unsigned char *decrypted, int padding);
char * opensslRSAtoPEM(RSA * rsaKey);
void init_parameters (struct sample_parameters * parms);
void parse_config (struct sample_parameters * parms);
