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

#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/asn1t.h>
#include "x509ac.h"

#ifdef __cplusplus
extern "C" 
{
#endif
/* Prototype attribute certificate code */

/* WARNING WARNING WARNING: this module is higly experimental and subject
 * to change. Use entirely at your own risk.
 */


/* ASN1 module */

ASN1_SEQUENCE(X509AC_OBJECT_DIGESTINFO) = {
	ASN1_SIMPLE(X509AC_OBJECT_DIGESTINFO, type, ASN1_ENUMERATED),
	ASN1_OPT(X509AC_OBJECT_DIGESTINFO, othertype, ASN1_OBJECT),
	ASN1_SIMPLE(X509AC_OBJECT_DIGESTINFO, algor, X509_ALGOR),
	ASN1_SIMPLE(X509AC_OBJECT_DIGESTINFO, digest, ASN1_BIT_STRING)
} ASN1_SEQUENCE_END(X509AC_OBJECT_DIGESTINFO)

ASN1_SEQUENCE(X509AC_ISSUER_SERIAL) = {
	ASN1_SEQUENCE_OF(X509AC_ISSUER_SERIAL, issuer, GENERAL_NAME),
	ASN1_SIMPLE(X509AC_ISSUER_SERIAL, serial, ASN1_INTEGER),
	ASN1_OPT(X509AC_ISSUER_SERIAL, issuerUniqueID, ASN1_BIT_STRING)
} ASN1_SEQUENCE_END(X509AC_ISSUER_SERIAL)

ASN1_SEQUENCE(X509AC_V2FORM) = {
	ASN1_SEQUENCE_OF_OPT(X509AC_V2FORM, issuer, GENERAL_NAME),
	ASN1_IMP_OPT(X509AC_V2FORM, baseCertID, X509AC_ISSUER_SERIAL, 0),
	ASN1_IMP_OPT(X509AC_V2FORM, digest, X509AC_OBJECT_DIGESTINFO, 1)
} ASN1_SEQUENCE_END(X509AC_V2FORM)

ASN1_CHOICE(X509AC_ISSUER) = {
	ASN1_SEQUENCE_OF(X509AC_ISSUER, d.v1Form, GENERAL_NAME),
	ASN1_IMP(X509AC_ISSUER, d.v2Form, X509AC_V2FORM, 0)
} ASN1_CHOICE_END(X509AC_ISSUER)

ASN1_SEQUENCE(X509AC_HOLDER) = {
//	ASN1_IMP_OPT(X509AC_HOLDER, baseCertID, X509AC_ISSUER_SERIAL, 0),
	ASN1_IMP_OPT(X509AC_HOLDER, baseCertID, X509AC_ISSUER_SERIAL, 0),
	ASN1_IMP_SEQUENCE_OF_OPT(X509AC_HOLDER, entity, GENERAL_NAME, 1),
	ASN1_IMP_OPT(X509AC_HOLDER, objectDigestInfo, X509AC_OBJECT_DIGESTINFO, 2)
} ASN1_SEQUENCE_END(X509AC_HOLDER)

ASN1_SEQUENCE(X509AC_VAL) = {
	ASN1_SIMPLE(X509AC_VAL, notBefore, ASN1_GENERALIZEDTIME),
	ASN1_SIMPLE(X509AC_VAL, notAfter, ASN1_GENERALIZEDTIME)
} ASN1_SEQUENCE_END(X509AC_VAL)

ASN1_SEQUENCE(X509AC_INFO) = {
//	ASN1_OPT(X509AC_INFO, version, ASN1_INTEGER),
	ASN1_SIMPLE(X509AC_INFO, version, ASN1_INTEGER),
	ASN1_SIMPLE(X509AC_INFO, holder, X509AC_HOLDER),
	ASN1_SIMPLE(X509AC_INFO, issuer, X509AC_ISSUER),
	ASN1_SIMPLE(X509AC_INFO, algor, X509_ALGOR),
	ASN1_SIMPLE(X509AC_INFO, serial, ASN1_INTEGER),
	ASN1_SIMPLE(X509AC_INFO, validity, X509AC_VAL),
//	ASN1_SEQUENCE_OF_OPT(X509AC_INFO, attributes, X509_ATTRIBUTE),
	ASN1_SEQUENCE_OF(X509AC_INFO, attributes, X509_ATTRIBUTE),
	ASN1_OPT(X509AC_INFO, issuerUniqueID, ASN1_BIT_STRING),
	ASN1_SEQUENCE_OF_OPT(X509AC_INFO, extensions, X509_EXTENSION)
} ASN1_SEQUENCE_END(X509AC_INFO)

ASN1_SEQUENCE(X509AC) = {
	ASN1_SIMPLE(X509AC, info, X509AC_INFO),
	ASN1_SIMPLE(X509AC, algor, X509_ALGOR),
	ASN1_SIMPLE(X509AC, signature, ASN1_BIT_STRING)
} ASN1_SEQUENCE_END(X509AC)

IMPLEMENT_ASN1_FUNCTIONS(X509AC_OBJECT_DIGESTINFO)
IMPLEMENT_ASN1_FUNCTIONS(X509AC_ISSUER_SERIAL)
IMPLEMENT_ASN1_FUNCTIONS(X509AC_V2FORM)
IMPLEMENT_ASN1_FUNCTIONS(X509AC_HOLDER)
IMPLEMENT_ASN1_FUNCTIONS(X509AC_ISSUER)
IMPLEMENT_ASN1_FUNCTIONS(X509AC_INFO)
IMPLEMENT_ASN1_FUNCTIONS(X509AC)

IMPLEMENT_ASN1_DUP_FUNCTION(X509AC_OBJECT_DIGESTINFO)
IMPLEMENT_ASN1_DUP_FUNCTION(X509AC_ISSUER_SERIAL)
IMPLEMENT_ASN1_DUP_FUNCTION(X509AC_V2FORM)
IMPLEMENT_ASN1_DUP_FUNCTION(X509AC_HOLDER)
IMPLEMENT_ASN1_DUP_FUNCTION(X509AC_ISSUER)
IMPLEMENT_ASN1_DUP_FUNCTION(X509AC_INFO)
IMPLEMENT_ASN1_DUP_FUNCTION(X509AC)

#ifdef __cplusplus
}
#endif


#ifdef TEST

int main()
{
	BIO *in;
	X509AC *ac;
	ERR_load_crypto_strings();
	in = BIO_new_fp(stdin, BIO_NOCLOSE);
	ac = ASN1_item_d2i_bio(ASN1_ITEM_rptr(X509AC), in, NULL);
	fprintf(stderr, "AC is %lx\n", ac);
	ERR_print_errors_fp(stderr);
}

#endif
