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


int encodeVM2RP_SETUUID(const char* rpid, const char* uuid, const char* vdi_id, int bufsize, byte* buf);
bool  decodeVM2RP_SETUUID(char** psz, int* pnargs, char** args, const byte* buf);
int  encodeRP2VM_SETUUID(int result, byte* buf);
bool  decodeRP2VM_SETUUID(int* presult, const byte* buf);
// get rpid
bool  decodeRP2VM_GETRPID(int *psize, byte* data, const byte* buf);
int encodeRP2VM_GETRPID(int size,byte *data, int bufsize, byte *buf);
// get vm meta
bool decodeRP2VM_GETVMMETA(int * psize, byte *data, const byte * buf);
int encodeRP2VM_GETVMMETA(int numofMetadata, byte * metadata[], int bufsize, byte *buf);

int encodeRP2VM_ISVERIFIED(int size, byte *data, int bufsize, byte * buf);
bool decodeRP2VM_ISVERIFIED(int * psize, byte *data, const byte * buf);

int encodeRP2VM_GETVMREPORT(int size, byte *data, int bufsize, byte * buf);
bool decodeRP2VM_GETVMREPORT(char** psz, int* pnargs, char** args, const byte* buf);

#endif



// ------------------------------------------------------------------------------


