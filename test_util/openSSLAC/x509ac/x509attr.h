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

#ifndef _attr_h_
#define _attr_h_


#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/asn1t.h>

#ifdef __cplusplus
extern "C" 
{
#endif

typedef struct SvceAuthInfo_st {
	GENERAL_NAME *service;
	GENERAL_NAME *ident;
	ASN1_OCTET_STRING *authInfo;
} SvceAuthInfo;


typedef struct IetfAttrSyntax_st {
	GENERAL_NAMES *policyAuthority;
	int type; 
	union{ 
		ASN1_OCTET_STRING *octets;
		ASN1_OBJECT *oid;
		ASN1_UTF8STRING *string;
	}values;
} IetfAttrSyntax;

typedef struct RoleSyntax_st {
	GENERAL_NAMES *roleAuthority;
	GENERAL_NAME  *roleName;
} RoleSyntax;

DECLARE_ASN1_ITEM(SvceAuthInfo)
DECLARE_ASN1_FUNCTIONS(SvceAuthInfo)
DECLARE_ASN1_FUNCTIONS(SvceAuthInfo)
DECLARE_ASN1_ITEM(IetfAttrSyntax)
DECLARE_ASN1_FUNCTIONS(IetfAttrSyntax)
DECLARE_ASN1_ITEM(RoleSyntax)
DECLARE_ASN1_FUNCTIONS(RoleSyntax)

typedef struct SecurityCategory_st {
	ASN1_OBJECT    *type;
	ASN1_TYPE     *value;
} SecurityCategory;

typedef struct Clearance_st {
	ASN1_OBJECT *policyId;
	ASN1_BIT_STRING *ClassList;
	SecurityCategory *securityCategories;
}Clearance;

DECLARE_ASN1_ITEM(Clearance)
DECLARE_ASN1_FUNCTIONS(Clearance)
DECLARE_ASN1_ITEM(SecurityCategory)
DECLARE_ASN1_FUNCTIONS(SecurityCategory)

#ifdef __cplusplus
}
#endif

#endif


