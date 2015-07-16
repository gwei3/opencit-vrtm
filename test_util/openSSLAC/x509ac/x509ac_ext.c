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
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/asn1t.h>
#include "x509ac_ext.h"

#ifdef __cplusplus
extern "C" 
{
#endif


	ASN1_CHOICE(Target) = {
		ASN1_IMP(Target, d.targetName, GENERAL_NAME,0),
		ASN1_IMP(Target, d.targetGroup, GENERAL_NAME,1),
		ASN1_IMP(Target, d.targetCert, X509AC_ISSUER_SERIAL,2)
	} ASN1_CHOICE_END(Target)

	ASN1_SEQUENCE(targetingInformation) = {
		ASN1_SEQUENCE_OF(targetingInformation,targets,Target)
	}ASN1_SEQUENCE_END(targetingInformation)


#ifdef __cplusplus
}
#endif
