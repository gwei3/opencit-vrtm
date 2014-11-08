
//  File: rptrust.h
//    Vinay Phegade 
//
//  Description: Reliance Point Trust establishment services 
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


// ------------------------------------------------------------------------
#ifndef _RPTRUST_H_
#define _RPTRUST_H_ 

#include "rp_api_code.h"

#define PARAMSIZE 8192
#ifndef byte
typedef unsigned char byte;
#endif

#ifdef USE_DRV_CHANNEL
struct tcBuffer {
    int                 m_procid;
    unsigned int        m_reqID;
    unsigned int        m_reqSize;
    unsigned int 		m_ustatus;
    int                 m_origprocid;
};
typedef struct tcBuffer tcBuffer;
#endif

//void 	PrintBytes(const char* szMsg, byte* pbData, int iSize);
int 	doTrustOp(int opcode, unsigned int* type, int inSize, byte* inData, int* poutSize, byte* outData);
int     doTrustOp_IS_MEASURED(int opcode, char* inData, int* poutSize, int* outData);

int Seal(int secinLen, byte * secretIn, int *dataLen, byte * dataOut);
int UnSeal(int dataLen, byte * dataIn, int *secoutLen, byte * secretOut);
int Attest(int inLen, byte * dataIn, int *outLen, byte *dataOut);
int getHostedComponentMeasurement(unsigned *type, int *outLen, byte *dataOut);
int getHostMeasurement(unsigned *type, int *outLen, byte *dataOut);
int getHostCertChain(int *outLen, byte *dataOut);
int channel_open();
bool isMeasured(char * uuid);
#endif
