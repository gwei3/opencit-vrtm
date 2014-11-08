/* Written by Dr Stephen N Henson (shenson@bigfoot.com) for the OpenSSL
 * project.
 */
/* ====================================================================
 * Copyright (c) 1999-2002 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    licensing@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

/* Prototype attribute certificate code */

/* WARNING WARNING WARNING: this module is higly experimental and subject
 * to change. Use entirely at your own risk.
 */



#ifndef _x509ac_h_
#define _x509ac_h_

#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/asn1t.h>

#ifdef __cplusplus
extern "C" 
{
#endif

typedef struct X509AC_OBJECT_DIGESTINFO_st {
	ASN1_ENUMERATED *type;
	ASN1_OBJECT *othertype;
	X509_ALGOR *algor;
	ASN1_BIT_STRING *digest;
} X509AC_OBJECT_DIGESTINFO;

typedef struct X509AC_ISSUER_SERIAL_st {
	GENERAL_NAMES *issuer;
	ASN1_INTEGER *serial;
	ASN1_BIT_STRING *issuerUniqueID;
} X509AC_ISSUER_SERIAL;

typedef struct X509AC_V2FORM_st {
	GENERAL_NAMES *issuer;
	X509AC_ISSUER_SERIAL *baseCertID;
	X509AC_OBJECT_DIGESTINFO *digest;
} X509AC_V2FORM;

typedef struct X509AC_ISSUER_st {
	int type;
	union {
		GENERAL_NAMES *v1Form;
		X509AC_V2FORM *v2Form;
	} d;
} X509AC_ISSUER;


typedef struct X509AC_HOLDER_st {
	X509AC_ISSUER_SERIAL *baseCertID;
	GENERAL_NAMES *entity;
	X509AC_OBJECT_DIGESTINFO *objectDigestInfo;
} X509AC_HOLDER;

typedef struct X509AC_VAL_st {
	ASN1_GENERALIZEDTIME *notBefore;
	ASN1_GENERALIZEDTIME *notAfter;
} X509AC_VAL;

typedef struct X509AC_INFO_st {
	ASN1_INTEGER *version;
	X509AC_HOLDER *holder;
	X509AC_ISSUER *issuer;
	X509_ALGOR *algor;
	ASN1_INTEGER *serial;
	X509AC_VAL *validity;
	STACK_OF(X509_ATTRIBUTE) *attributes;
	//X509_ATTRIBUTE *attributes;
	ASN1_BIT_STRING *issuerUniqueID;
	STACK_OF(X509_EXTENSION) *extensions;
} X509AC_INFO;

typedef struct X509AC_st {
	X509AC_INFO *info;
	X509_ALGOR *algor;
	ASN1_BIT_STRING *signature;
} X509AC;

/* added by markus lorch */
DECLARE_ASN1_ITEM(X509AC)
DECLARE_ASN1_FUNCTIONS(X509AC)
DECLARE_ASN1_ITEM(X509AC_INFO)
DECLARE_ASN1_FUNCTIONS(X509AC_INFO)
DECLARE_ASN1_ITEM(X509AC_ISSUER_SERIAL)
DECLARE_ASN1_FUNCTIONS(X509AC_ISSUER_SERIAL)
DECLARE_ASN1_ITEM(X509AC_ISSUER)
DECLARE_ASN1_FUNCTIONS(X509AC_ISSUER)
DECLARE_ASN1_ITEM(X509AC_HOLDER)
DECLARE_ASN1_FUNCTIONS(X509AC_HOLDER)
DECLARE_ASN1_ITEM(X509AC_V2FORM)
DECLARE_ASN1_FUNCTIONS(X509AC_V2FORM)
DECLARE_ASN1_ITEM(X509AC_OBJECT_DIGESTINFO)
DECLARE_ASN1_FUNCTIONS(X509AC_OBJECT_DIGESTINFO)
#ifdef __cplusplus
}
#endif

#endif

