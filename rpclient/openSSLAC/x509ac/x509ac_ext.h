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

#ifndef _x509ac_ext_h_
#define _x509ac_ext_h_


#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/asn1t.h>
#include "x509ac.h"

#ifdef __cplusplus
extern "C" 
{
#endif

	/*
	This field is defined as follows.
	targetingInformation  EXTENSION  ::= {
	SYNTAX  			SEQUENCE SIZE (1..MAX) OF Targets
	IDENTIFIED BY	 	id-ce-targetInformation }
	Targets	::=		SEQUENCE SIZE (1..MAX) OF Target
	Target	:	:=	CHOICE {
	targetName			[0]		GeneralName,
	targetGroup		[1]		GeneralName,
	targetCert			[2]		TargetCert }
	TargetCert	::=	SEQUENCE {
	targetCertificate		IssuerSerial,
	targetName			GeneralName OPTIONAL,
	certDigestInfo		ObjectDigestInfo OPTIONAL }
	The targetName component, if present, provides the name of target servers/services 
	for which the containing attribute certificate is targeted. 
	The targetGroup component, if present, provides the name of a target group for 
	which the containing attribute certificate is targeted. How the membership of a 
	target within a targetGroup is determined is outside the scope of this Specification.
	The targetCert component, if present, identifies target servers/services by reference 
	to their certificate.
	*/

	typedef struct TargetCert_st{
		X509AC_ISSUER_SERIAL *targetCertificate;
		GENERAL_NAME *targetName;
		X509AC_OBJECT_DIGESTINFO *certDigestInfo;
	}TargetCert;


	typedef struct Target_st {
		int type;
		union{
			GENERAL_NAME *targetName; 
			GENERAL_NAME *targetGroup;
			TargetCert *targetCert;
		}d;
	} Target;

	
	typedef struct targetingInformation_st {
		Target *targets;
	} targetingInformation;




#ifdef __cplusplus
}
#endif

#endif

