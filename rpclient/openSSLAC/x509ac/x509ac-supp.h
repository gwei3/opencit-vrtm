/*****************************************************************
*
* This code has been developed at:
*************************************
* Pervasive Computing Laboratory
*************************************
* Telematic Engineering Dept.
* Carlos III university
* Contact:
*		Daniel Díaz Sanchez
*		Florina Almenarez
*		Andrés Marín
*************************************		
* Mail:	dds[@_@]it.uc3m.es
* Web: http://www.it.uc3m.es/dds
* Blog: http://rubinstein.gast.it.uc3m.es/research/dds
* Team: http://www.it.uc3m.es/pervasive
**********************************************************
* This code is released under the terms of OpenSSL license
* http://www.openssl.org
*****************************************************************/

#ifndef X509AC_SUPP
#define X509AC_SUPP

#include <openssl/x509.h>
#include "x509ac.h"

#ifdef __cplusplus
extern "C" 
{
#endif
	/* defines for debugging and error reporting */

#define ONELINELEN 128

	void handle_error (const char *file, int lineno, const char *msg);

#define int_error(msg) handle_error(__FILE__, __LINE__, msg)

	/* should be in pem/pem.h */
#define PEM_STRING_X509AC               "CERTIFICATE"
#define PEM_read_X509AC(fp,x,cb,u) (X509AC *)PEM_ASN1_read( \
	(char *(*)())d2i_X509AC,PEM_STRING_X509AC,fp,(char **)x,cb,u)
#define	PEM_write_X509AC(fp,x) \
	PEM_ASN1_write((int (*)())i2d_X509AC,\
	PEM_STRING_X509AC,fp,(char *)x,NULL,NULL,0,NULL,NULL)

	/* functions in X509AC-supp.c */
	X509AC_ISSUER_SERIAL* X509_get_basecertID(X509 *x);
	X509_NAME *X509AC_get_issuer_name(X509AC *a);
	X509_NAME *X509AC_get_holder_entity_name(X509AC *a);
	X509AC_ISSUER_SERIAL *X509AC_get_holder_baseCertID(X509AC *a);
	ASN1_BIT_STRING *X509AC_get_holder_objectDigestInfo(X509AC *a);
	X509AC_ISSUER_SERIAL *X509AC_get_issuer_baseCertID(X509AC *a);
	ASN1_BIT_STRING *X509AC_get_issuer_objectDigestInfo(X509AC *a);
	long X509AC_get_version(X509AC *a);
	int X509AC_set_version(X509AC *a, long version);
	int X509AC_set_holder_name(X509AC* a, X509_NAME *name);
	int X509AC_set_holder_entity_name(X509AC* a, X509_NAME *name);
	int X509AC_set_holder_serialNumber(X509AC *x, ASN1_INTEGER *serial);
	int X509AC_set_holder_baseCertID(X509AC* a, X509AC_ISSUER_SERIAL *bci);
	int X509AC_set_holder_objectDigestInfo(X509AC *a, X509AC_OBJECT_DIGESTINFO *odig);
	int X509AC_set_issuer_baseCertID(X509AC* a, X509AC_ISSUER_SERIAL *bci);
	int X509AC_set_issuer_name(X509AC* a, X509_NAME *name);
	int X509AC_set_GENERAL_NAME_name(GENERAL_NAMES *gens, X509_NAME *name);
	int X509AC_set_baseCertID_name(X509AC_ISSUER_SERIAL *bci, X509_NAME *name);
	int X509AC_set_baseCertID_serial(X509AC_ISSUER_SERIAL *bci, ASN1_INTEGER *serial);
	int X509AC_set_baseCertID_issuerUniqueID(X509AC_ISSUER_SERIAL *bci, ASN1_BIT_STRING *uid);
	X509_ATTRIBUTE * X509AC_get_attr( X509AC *a, int idx );
	int X509AC_add_attribute_by_NID(X509AC *a, int nid, int atrtype, void *value);
	int X509AC_add_attribute(X509AC *a, X509_ATTRIBUTE *attr);
	int X509AC_sign_rsa(X509AC *a, RSA *rsa, EVP_MD *md);
	int X509AC_sign_pkey(X509AC *a, EVP_PKEY *pkey, EVP_MD *md);
	ASN1_TYPE *X509AC_ATTRIBUTE_get0_type(X509_ATTRIBUTE *attr, int idx);
	void *X509AC_ATTRIBUTE_get0_data(X509_ATTRIBUTE *attr, int idx,
		int atrtype, void *data);
	int X509AC_add_X509_ATTRIBUTE(X509AC *a, X509_ATTRIBUTE *attr);
	int X509AC_get_attributecount(X509AC *a);
	int X509AC_X509_NAME_dup(X509_NAME **xn, X509_NAME *name);
	int X509AC_verify_cert(X509_STORE_CTX * verify_ctx, X509AC * ac);
	void X509AC_print(X509AC *ac);
	int GENERAL_NAMES_pprinter(FILE *out, GENERAL_NAMES *gens);
	int GENERAL_NAME_pprinter(FILE *out, GENERAL_NAME *gen);
	int X509AC_add_extension(X509AC *a, X509_EXTENSION *ex, int loc);
#ifdef __cplusplus
}
#endif

#endif
