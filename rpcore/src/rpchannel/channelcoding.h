//
//  File: buffercoding.h
//  Description: encode/decode buffers between app/OS and tcService/OS
//
//  Copyright (c) 2012, John Manferdelli.  All rights reserved.
//     Some contributions Copyright (c) 2012, Intel Corporation. 
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


// ------------------------------------------------------------------------------


#ifndef _CHANNELCODING_H__
#define _CHANNELCODING_H__



#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#ifndef byte
typedef unsigned char byte;
#endif

/*
int   encodeTCSERVICEGETPOLICYKEYFROMOS(uint32_t keyType, int size, 
                                    const byte* key, int bufsize, byte* buf);
bool  decodeTCSERVICEGETPOLICYKEYFROMOS(uint32_t* pkeyType, int* psize, 
                                    byte* key, const byte* buf);
bool  encodeTCSERVICEGETPOLICYKEYFROMOS(uint32_t keyType, int size, 
                                        const byte* key, byte* buf);
bool  decodeTCSERVICEGETPOLICYKEYFROMOS(uint32_t* pkeyType, int* psize,
                                        byte* key, const byte* buf);
                                        * */
int   encodeVM2RP_GETRPHOSTQUOTE(int size, const byte* data, int bufsize, byte* buf);
bool  decodeVM2RP_GETRPHOSTQUOTE(int* psize, byte* data, const byte* buf);
int   encodeRP2VM_GETRPHOSTQUOTE(int size, const byte* data, int bufsize, byte* buf);
bool  decodeRP2VM_GETRPHOSTQUOTE(int* psize, byte* data, const byte* buf);

//Get TPM Quote
int   encodeVM2RP_GETTPMQUOTE(int size, const byte* data, int bufsize, byte* buf);
bool  decodeVM2RP_GETTPMQUOTE(int* psize, byte* data, const byte* buf);
int   encodeRP2VM_GETTPMQUOTE(int size, const byte* data, int bufsize, byte* buf);
bool  decodeRP2VM_GETTPMQUOTE(int* psize, byte* data, const byte* buf);


int   encodeVM2RP_RPHOSTQUOTERESPONSE(int size, const byte* data, int bufsize, byte* buf);
bool  decodeVM2RP_RPHOSTQUOTERESPONSE(int* psize, byte* data, const byte* buf);
int   encodeRP2VM_RPHOSTQUOTERESPONSE(int size, const byte* data, int bufsize, byte* buf);
bool  decodeRP2VM_RPHOSTQUOTERESPONSE(int* psize, byte* data, const byte* buf);


int   encodeVM2RP_SEALFOR(int sealsize, const byte* seal, int bufsize, byte* buf);
bool  decodeVM2RP_SEALFOR(int* psealedsize, byte* sealed, const byte* buf);

int   encodeRP2VM_SEALFOR(int sealedsize, const byte* sealed, int bufsize, byte* buf);
bool  decodeRP2VM_SEALFOR(int* psealedsize, byte* sealed, const byte* buf);

int   encodeVM2RP_UNSEALFOR(int sealedsize, const byte* sealed, int bufsize, byte* buf);
bool  decodeVM2RP_UNSEALFOR(int* psealsize, byte* seal, const byte* buf);

int   encodeRP2VM_UNSEALFOR( int sealsize, const byte* seal, int bufsize, byte* buf);
bool  decodeRP2VM_UNSEALFOR(int* punsealsize, byte* unsealed, const byte* buf);

int  encodeVM2RP_ATTESTFOR(int toattestsize, const byte* toattest, int bufsize, byte* buf);
bool decodeVM2RP_ATTESTFOR(int* ptoattestsize, byte* toattest, const byte* buf);

int  encodeRP2VM_ATTESTFOR(int attestsize, const byte* attested, int bufsize, byte* buf);
bool decodeRP2VM_ATTESTFOR(int* pattestsize, byte* attested, const byte* buf);

//start/measure VM
int encodeVM2RP_STARTAPP(const char* file, int nargs, char** args, int bufsize, byte* buf);
bool decodeVM2RP_STARTAPP(char** psz, int* pnargs, char**, const byte* buf);

int   encodeRP2VM_STARTAPP(int procid, int sizebuf, byte* buf);
bool  decodeRP2VM_STARTAPP(int* pprocid, const byte* buf);

//terminate app/vm
int  encodeVM2RP_TERMINATEAPP(int size, const byte* data, int bufsize, byte* buf);
bool decodeVM2RP_TERMINATEAPP(int* psize, byte* data, const byte* buf);

int  encodeRP2VM_TERMINATEAPP(int size, const byte* data, int bufsize, byte* buf);
bool  decodeRP2VM_TERMINATEAPP(int* psize, byte* data, const byte* buf);

//prog or VM hash
int encodeVM2RP_GETPROGHASH(int pid, int bufsize, byte* buf);
bool decodeVM2RP_GETPROGHASH(int* psize, byte* data, const byte* buf);
int encodeRP2VM_GETPROGHASH (char* hashtype, int size, const byte* hash, int bufsize, byte* buf);
bool decodeRP2VM_GETPROGHASH(char** phashtype, int* psize, byte* hash, int bufsize, const byte* buf);


//os hash - hash of the host system including hypervisor, dom0/host, etc.
int encodeVM2RP_GETOSHASH(int pid, int bufsize, byte* buf);
bool decodeVM2RP_GETOSHASH(int* psize, byte* data, const byte* buf);
int   encodeRP2VM_GETOSHASH(char* hashtype, int size, const byte* hash, int bufsize, byte* buf);
bool  decodeRP2VM_GETOSHASH(char** phashType, int* psize,  byte* hash, int bufsize, const byte* buf);
                 
//os cred                  
int  encodeVM2RP_GETOSCREDS(uint32_t credType, int size, const byte* cred, byte* buf);                                   
bool  decodeVM2RP_GETOSCREDS(uint32_t* pcredType, int* psize, byte* cred,  const byte* buf);
int   encodeRP2VM_GETOSCREDS(uint32_t credType, int size, const byte* cred,  int bufsize, byte* buf);
bool  decodeRP2VM_GETOSCREDS(uint32_t* pcredType, int* psize,  byte* cred, const byte* buf);

//os cert - certificate chain                                
int  encodeVM2RP_GETOSCERT(int size, byte* buf);
bool decodeVM2RP_GETOSCERT(int* psize, byte* data, const byte* buf);
int encodeRP2VM_GETOSCERT(char* type, int size, const byte* certchain,  int bufsize, byte* buf);
bool decodeRP2VM_GETOSCERT(char** pcert_type, int* psize,  byte* certchain, int bufsize, const byte* buf);

// get AIK cert
int  encodeVM2RP_GETAIKCERT(int size, byte* buf);
bool decodeVM2RP_GETAIKCERT(int* psize, byte* data, const byte* buf);
int encodeRP2VM_GETAIKCERT(char* type, int size, const byte* certchain,  int bufsize, byte* buf);
bool decodeRP2VM_GETAIKCERT(char** pcert_type, int* psize,  byte* certchain, int bufsize, const byte* buf);

                                

int   encodeVM2RP_SETFILE(int size, const byte* data, int bufsize, byte* buf);
bool  decodeVM2RP_SETFILE(int* psize, byte* data, const byte* buf);
int   encodeRP2VM_SETFILE(int size, const byte* data, int bufsize, byte* buf);
bool  decodeRP2VM_SETFILE(int* psize, byte* data, const byte* buf);

int encodeVM2RP_SETUUID(const char* rpid, const char* uuid, const char* vdi_id, int bufsize, byte* buf);
bool  decodeVM2RP_SETUUID(char** psz, int* pnargs, char** args, const byte* buf);
int  encodeRP2VM_SETUUID(int result, byte* buf);
bool  decodeRP2VM_SETUUID(int* presult, const byte* buf);

int encodeVM2RP_CHECK_VM_VDI(const char* uuid, const char* vdi_id, int bufsize, byte* buf);
bool decodeVM2RP_CHECK_VM_VDI(char** psz, int* pnargs, char** args, const byte* buf);
int encodeRP2VM_CHECK_VM_VDI(int result, byte* buf);
bool decodeRP2VM_CHECK_VM_VDI(int result, byte* buf);

int encodeVM2RP_IS_MEASURED(const char* uuid, int bufsize, byte* buf);
bool decodeVM2RP_IS_MEASURED(char** psz, int* pnargs, char** args, const byte* buf);
int encodeRP2VM_IS_MEASURED(int result, byte* buf);
bool decodeRP2VM_IS_MEASURED(int* presult, const byte* buf);
/*
int  encodeVM2RP_IS_MEASURED(int size, const byte* data, int bufsize, byte* buf);
bool  decodeVM2RP_IS_MEASURED(int* psize, byte* data, const byte* buf);
int  encodeRP2VM_IS_MEASURED(const char response, int bufsize, byte* buf);
bool  decodeRP2VM_IS_MEASURED(int* psize, byte *response, const byte* buf);
*/
#endif


// ------------------------------------------------------------------------------


