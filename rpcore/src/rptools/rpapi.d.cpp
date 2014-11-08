
//
//  File: rpapi.h
//      
//     
//
//  Description: RP crypto data 
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

#ifndef _RPAPI_
#define _RPAPI_

const char	*szAlgoTypes [] = {"NOALG", "AES128", "AES256", "RSA1024","RSA2048",0};
const char	*szModeTypes [] = {"NOMODE", "ECBMODE", "CBCMODE", "CTRMODE","GCMMODE", 0};
const char	*szHmacTypes [] = {"NOHMAC", "HMACSHA256", 0};
const char	*szPadTypes [] = {"NOPAD", "PKCSPAD", "SYMPAD", 0};
const char	*szHashTypes [] = {"NOHASH", "SHA256HASH", "SHA1HASH", "SHA512HASH","SHA384HASH","MD5HASH", 0};

const char	*szKeyTypes [] = {"AES128", "AES256", "RSA1024", "RSA2048", 0};
const char	*szDigestType [] = {"SHA-1", "SHA-256", 0};
const char	*szMACType [] = {"HMAC", 0};
const char	*szSignatureAlgs[]= {"rsa2048-sha256-pkcspad", "rsa1024-sha256-pkcspad", 0 };
const char	*szPKPadding[]= {"RSA_PKCS1_PADDING", "RSA_PKCS1_OAEP_PADDING", "RSA_NO_PADDING", 0 };

#endif


// -------------------------------------------------------------------------------------


