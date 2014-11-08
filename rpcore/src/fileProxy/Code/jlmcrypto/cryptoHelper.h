//
//  File: cryptoHelper.h
//      John Manferdelli
//
//  Description:  crypto helper function definitions
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


// --------------------------------------------------------------------------


#ifndef _CRYPTOHELPER__H
#define _CRYPTOHELPER__H

#include "jlmTypes.h"
#include "jlmUtility.h"
#include "keys.h"
#include "jlmcrypto.h"
#include "tinyxml.h"


#define USEPUBLIC      1
#define USEPRIVATE     2
#define USEPRIVATEFAST 3


bool        RSADecrypt_i(RSAKey& key, int sizein, byte* in, int* psizeout, 
                       byte* out, bool fFast=false);
bool        RSAEncrypt_i(RSAKey& key, int sizein, byte* in, int* psizeout, byte* out);
bool        RSASign(RSAKey& key, int hashType, byte* hash, 
                                       int* psizeout, byte* out);
bool        RSAVerify(RSAKey& key, int hashType, byte* hash, byte* in);
bool        RSASeal(RSAKey& key, u32 keyUse, int sizein, byte* in, int* psizeout, 
                    byte* out);
bool        RSAUnseal(RSAKey& key, u32 keyUse, int sizein, byte* in, int* psizeout, 
                      byte* out);

RSAKey*     RSAGenerateKeyPair(int keySize);
RSAKey*     RSAKeyfromKeyInfoNode(TiXmlNode* pNode);
char*       RSAKeyInfofromKey(RSAKey& key);
char*       RSAPublicKeyInfofromKey(RSAKey& key);
RSAKey*     RSAKeyfromkeyInfo(const char* szKeyInfo);

int         timeCompare(struct tm& time1, struct tm& time2);
char*       stringtimefromtimeInfo(struct tm* timeinfo);
struct tm*  timeNow();
bool        timeInfofromstring(const char* szTime, struct tm& thetime);
bool        checktimeinInterval(tm& time, tm& begin, tm& end);

#ifdef TEST
void        printTime(struct tm* timeinfo);
#endif 

bool        sameRSAKey(RSAKey* pKey1, RSAKey* pKey2);
KeyInfo*    ReadKeyfromFile_i(const char* szKeyFile);

int         maxbytesfromBase64string(int nc);
int         maxcharsinBase64stringfrombytes(int nb);
bool        base64frombytes(int nb, byte* in, int* pnc, char* out);
bool        bytesfrombase64(char* in, int* pnb, byte* out);

char*	    XMLCanonicalizedString(const char* szXML);


bool        XMLenclosingtypefromelements(const char* tag, int numAttr, 
                                   const char** attrName, const char** attrValues, 
                                   int numElts, const char** elts, 
                                   int* psize, char* buf);

bool        VerifyRSASha1SignaturefromSignedInfoandKey(RSAKey& key, 
                                                  char* szsignedInfo, 
                                                  char* szSigValue);

bool        VerifyRSASha256SignaturefromSignedInfoandKey(RSAKey& key, 
                                                  char* szsignedInfo, 
                                                  char* szSigValue);

char*       XMLRSASha256SignaturefromSignedInfoandKey(RSAKey& key, 
                                                const char* szsignedInfo);
char*       XMLRSASha256SignaturefromSignedInfoNodeandKey(RSAKey& key, 
                                                    TiXmlNode* signedInfo);

char*       constructXMLRSASha256SignaturefromSignedInfoandKey(RSAKey& key,
                                                const char* id,
                                                const char* szsignedInfo);
#endif


// -----------------------------------------------------------------------------


