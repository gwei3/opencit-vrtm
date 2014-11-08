//  File: linuxHostsupport.cpp
//      John Manferdelli
//  Description:  Support for Linux host as trusted OS
//
//  Copyright (c) 2012, John Manferdelli
//  Some contributions copyright (c) 2012, Intel Corporation
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

#include "jlmTypes.h"
#include "logging.h"
#include "jlmcrypto.h"
#include "jlmUtility.h"
#include "modesandpadding.h"
#include "sha256.h"
#include "tao.h"
#include "bignum.h"
#include "mpFunctions.h"
#ifndef NEWANDREORGANIZED
#include "rsaHelper.h"
#else
#include "cryptoHelper.h"
#endif
#include "trustedKeyNego.h"
#include "tcIO.h"
#include "rp_api_code.h"
#include "channelcoding.h"
#include "tcService.h"
#include <string.h>
#include <time.h>


// -------------------------------------------------------------------------
extern tcChannel   g_reqChannel;

//
//   Service support from LINUX
//


// request channel to device driver
//tcChannel   g_reqChannel;
int         g_myPid= -1;

#if 0 //moved to rpinterface
bool initLinuxService(const char* name)
{
#ifdef TCTEST
    if(name!=NULL)
        fprintf(g_logFile, "initLinuxService started %s\n", name);
    else
        fprintf(g_logFile, "initLinuxService started no childname\n");
#endif

    g_myPid= getpid();
    if(!g_reqChannel.OpenBuf(TCDEVICEDRIVER, 0, name ,0)) {
        fprintf(g_logFile, "initLinuxService: OpenBuf returned false \n");
        return false;
    }

#ifdef TCTEST
    fprintf(g_logFile, "initLinuxService returns true\n");
#endif
    return true;
}


bool closeLinuxService()
{
    g_reqChannel.CloseBuf();
    return true;
}
#endif 


bool getEntropyfromDeviceDriver(int size, byte* pKey)
{
    return false;
}


bool getprogramNamefromDeviceDriver(int* pSize, const char* szName)
{
    // this is done elsewhere
    return true;
}


bool getpolicykeyfromDeviceDriver(u32* pkeyType, int* pSize, byte* pKey)
{
#if 0
    u32         ustatus;
    u32         ureq;
    int         procid;
    int         origprocid;
    int         size= PARAMSIZE;
    byte        rgBuf[PARAMSIZE];

    if(!g_reqChannel.sendtcBuf(g_myPid, TCSERVICEGETRPQUOTEFROMAPP, 0, 
                               g_myPid, size, rgBuf)) {
        fprintf(g_logFile, "getpolicykeyfromDeviceDriver: sendtcBuf for encodeTCSERVICEPOLICYKEYFROMAPP failed\n");
        return false;
    }
    size= PARAMSIZE;
    if(!g_reqChannel.gettcBuf(&procid, &ureq, &ustatus, &origprocid, &size, rgBuf)) {
        fprintf(g_logFile, "getpolicykeyfromDeviceDriver: gettcBuf for encodeTCSERVICEPOLICYKEYFROMAPP failed\n");
        return false;
    }
    if(!decodeTCSERVICEGETPOLICYKEYFROMOS(pkeyType, pSize, pKey, rgBuf)) {
        fprintf(g_logFile, "getpolicykeyfromDeviceDriver: gettcBuf for decodeTCSERVICEPOLICYKEYFROMAPP failed\n");
        return false;
    }
#ifdef TEST1
    PrintBytes("Policy key: ", pKey, *pSize);
#endif
    fprintf(g_logFile, "getpolicykeyfromDeviceDriverOS parent returns true\n");
#else
	//asm("int $0x3");
	*pSize = 0;
	pKey = '\0';
#endif    
    return true;
}


bool getOSMeasurementfromDeviceDriver(u32* phashType, int* pSize, byte* pHash)
{
    u32         ustatus;
    u32         ureq;
    int         procid;
    int         origprocid;
    int         size= 0;
    byte        rgBuf[PARAMSIZE];
	char* 		str_hash_type = NULL;
	
#ifdef TEST
    fprintf(g_logFile, "getOSMeasurementfromDeviceDriver\n");
#endif
    if(!g_reqChannel.sendtcBuf(g_myPid, VM2RP_GETOSHASH, 0, 
                               g_myPid, size, rgBuf)) {
        fprintf(g_logFile, "getOSMeasurementfromDeviceDriver: sendtcBuf for encodeVM2RP_SEALFOR failed\n");
        return false;
    }
    size= PARAMSIZE;
    if(!g_reqChannel.gettcBuf(&procid, &ureq, &ustatus, &origprocid, &size, rgBuf)) {
        fprintf(g_logFile, "getOSMeasurementfromDeviceDriver: gettcBuf for encodeVM2RP_SEALFOR failed\n");
        return false;
    }
    if(!decodeRP2VM_GETOSHASH(&str_hash_type, pSize, pHash, size, rgBuf)) {
        fprintf(g_logFile, "getOSMeasurementfromDeviceDriver: gettcBuf for decodeVM2RP_SEALFOR failed\n");
        return false;
    }
    //v:for time-being hardcode
    *phashType = HASHTYPETPM;
    if (str_hash_type)
		free(str_hash_type);
		
#ifdef TEST
    PrintBytes("OS hash: ", pHash, *pSize);
#endif
    fprintf(g_logFile, "getOSMeasurementfromDeviceDriver OS parent returns true\n");
    return true;
}


bool getHostedMeasurementfromDeviceDriver(int childproc, u32* phashType, 
                                          int* pSize, byte* pHash)
{
    u32         ustatus;
    u32         ureq;
    int         procid;
    int         origprocid;
    int         size= PARAMSIZE;
    byte        rgBuf[PARAMSIZE];
    char* 		str_hash_type = NULL;

#ifdef TEST
    fprintf(g_logFile, "getHostedMeasurementfromDeviceDriver\n");
#endif
    // Does my pid have valid hash?
    size= encodeVM2RP_GETPROGHASH(childproc, 1024, rgBuf);
    if(!g_reqChannel.sendtcBuf(g_myPid, VM2RP_GETPROGHASH,
                        0, g_myPid, size, rgBuf)) {
        fprintf(g_logFile, "getHostedMeasurementfromDeviceDriver: sendtcBuf for VM2RP_GETPROGHASH failed\n");
        return false;
    }
    size= PARAMSIZE;
    if(!g_reqChannel.gettcBuf(&procid, &ureq, &ustatus, &origprocid, &size, rgBuf)) {
        fprintf(g_logFile, "getHostedMeasurementfromDeviceDriver: gettcBuf for VM2RP_GETPROGHASH failed\n");
    }
    size= PARAMSIZE;
    if(!decodeRP2VM_GETPROGHASH(&str_hash_type, pSize, pHash, 1024, rgBuf)) {
        fprintf(g_logFile, "getHostedMeasurementfromDeviceDriver: cant decode os hash\n");
        return false;
    }
   //v:for time-being hardcode
    *phashType = HASHTYPEHOSTEDPROGRAM;
    if (str_hash_type)
		free(str_hash_type);
		
#ifdef TEST
    PrintBytes("getHostedMeasurementfromDeviceDriver prog hash: ", pHash, *pSize);
#endif
    return true;
}


bool startAppfromDeviceDriver(const char* szexecFile, int* ppid)
{
    u32         ustatus;
    u32         ureq;
    int         procid;
    int         origprocid;
    int         size= PARAMSIZE;
    byte        rgBuf[PARAMSIZE];
    int         an = 0;
    char**      av = NULL;

    size= encodeVM2RP_STARTAPP(szexecFile, an, av, PARAMSIZE, rgBuf);
    if(size<0) {
        fprintf(g_logFile, "startProcessLinuxService: encodeVM2RP_STARTAPP failed\n");
        return false;
    }
    if(!g_reqChannel.sendtcBuf(g_myPid, VM2RP_STARTAPP, 0, g_myPid, size, rgBuf)) {
        fprintf(g_logFile, "startProcessLinuxService: sendtcBuf for VM2RP_STARTAPP failed\n");
        return false;
    }
    size= PARAMSIZE;
    if(!g_reqChannel.gettcBuf(&procid, &ureq, &ustatus, &origprocid, &size, rgBuf)) {
        fprintf(g_logFile, "startProcessLinuxService: gettcBuf for VM2RP_STARTAPP failed\n");
        return false;
    }
    if(!decodeRP2VM_STARTAPP(ppid, rgBuf)) {
        fprintf(g_logFile, "startProcessLinuxService: cant decode childproc\n");
        return false;
    }
#ifdef TEST
    fprintf(g_logFile, "startProcessLinuxService: Process created: %d by servicepid %d\n", 
            *ppid, g_myPid);
#endif
    return true;
}


bool sealfromDeviceDriver(int inSize, byte* inData, int* poutSize, byte* outData)
{
    u32         ustatus;
    u32         ureq;
    int         procid;
    int         origprocid;
    int         size= 0;
    byte        rgBuf[PARAMSIZE];

#ifdef TEST
    fprintf(g_logFile, "sealwithLinuxService(%d)\n", inSize);
#endif
    memset(outData, 0, *poutSize);
    size= encodeVM2RP_SEALFOR(inSize, inData, PARAMSIZE, rgBuf);
    if(size<0) {
        fprintf(g_logFile, "sealwithLinuxService: encodeVM2RP_SEALFOR failed\n");
        return false;
    }
    if(!g_reqChannel.sendtcBuf(getpid(), VM2RP_SEALFOR, 0, 
                               getpid(), size, rgBuf)) {
        fprintf(g_logFile, "sealwithLinuxService: sendtcBuf for encodeVM2RP_SEALFOR failed\n");
        return false;
    }
    size= PARAMSIZE;
    if(!g_reqChannel.gettcBuf(&procid, &ureq, &ustatus, &origprocid, &size, rgBuf)) {
        fprintf(g_logFile, "sealwithLinuxService: gettcBuf for encodeVM2RP_SEALFOR failed\n");
        return false;
    }
    if(!decodeRP2VM_SEALFOR(poutSize, outData, rgBuf)) {
        fprintf(g_logFile, "sealwithLinuxService: gettcBuf for decodeVM2RP_SEALFOR failed\n");
        return false;
    }
#ifdef TCTEST1
    PrintBytes("To seal: ", inData, inSize);
    PrintBytes("Sealed :", outData, *poutSize);
#endif
    return true;
}


bool unsealfromDeviceDriver(int inSize, byte* inData, int* poutSize, byte* outData)
{
    u32         ustatus;
    u32         ureq;
    int         procid;
    int         origprocid;
    int         size= 0;
    byte        rgBuf[PARAMSIZE];

#ifdef TEST
    fprintf(g_logFile, "unsealwithLinuxService(%d)\n", inSize);
#endif
    memset(outData, 0, *poutSize);
    size= encodeVM2RP_UNSEALFOR(inSize, inData, PARAMSIZE, rgBuf);
    if(size<0) {
        fprintf(g_logFile, "unsealwithLinuxService: encodeVM2RP_UNSEALFOR failed\n");
        return false;
    }
    if(!g_reqChannel.sendtcBuf(getpid(), VM2RP_UNSEALFOR, 0, getpid(), size, rgBuf)) {
        fprintf(g_logFile, "unsealwithLinuxService: sendtcBuf for VM2RP_UNSEALFOR failed\n");
        return false;
    }
    size= PARAMSIZE;
    if(!g_reqChannel.gettcBuf(&procid, &ureq, &ustatus, &origprocid, &size, rgBuf)) {
        fprintf(g_logFile, "unsealwithLinuxService: gettcBuf for VM2RP_UNSEALFOR failed\n");
        return false;
    }
    if(!decodeRP2VM_UNSEALFOR(poutSize, outData, rgBuf)) {
        fprintf(g_logFile, "unsealwithLinuxService: gettcBuf for decodeRP2VM_SEALFOR failed\n");
        return false;
    }
#ifdef TCTEST1
    PrintBytes("To unseal: ", inData, inSize);
    PrintBytes("Unsealed : ", outData, *poutSize);
#endif
    return true;
}


bool quotefromDeviceDriver(int inSize, byte* inData, int* poutSize, byte* outData)
{
    u32         ustatus;
    u32         ureq;
    int         procid;
    int         origprocid;
    int         size= 0;
    byte        rgBuf[PARAMSIZE];

#ifdef TEST
    fprintf(g_logFile, "quotewithLinuxService(%d, %d)\n", inSize, *poutSize);
#endif
    memset(outData, 0, *poutSize);
    size= encodeVM2RP_ATTESTFOR(inSize, inData, PARAMSIZE, rgBuf);
    if(size<0) {
        fprintf(g_logFile, "quotewithLinuxService: encodeVM2RP_ATTESTFOR failed\n");
        return false;
    }
#ifdef TEST1
    fprintf(g_logFile, "quotewithLinuxService: sending %d\n", inSize);
    PrintBytes(" req buffer: ", inData, inSize); 
    fflush(g_logFile);
#endif
    if(!g_reqChannel.sendtcBuf(g_myPid, VM2RP_ATTESTFOR, 0, procid, size, rgBuf)) {
        fprintf(g_logFile, "quotewithLinuxService: sendtcBuf for VM2RP_ATTESTFOR failed\n");
        return false;
    }
    size= PARAMSIZE;
    if(!g_reqChannel.gettcBuf(&procid, &ureq, &ustatus, &origprocid, &size, rgBuf)) {
        fprintf(g_logFile, "quotewithLinuxService: gettcBuf for VM2RP_ATTESTFOR failed\n");
        return false;
    }
#ifdef TEST1
    fprintf(g_logFile, "quotewithLinuxService: rgBufsize is %d, status: %d\n", 
            size, ustatus);
#endif
    if(!decodeRP2VM_ATTESTFOR(poutSize, outData, rgBuf)) {
        fprintf(g_logFile, "quotewithLinuxService: gettcBuf for decodeRP2VM_ATTESTFORfailed\n");
        return false;
    }
#ifdef TEST1
    fprintf(g_logFile, "quotewithLinuxService: outsize is %d\n", *poutSize);
    PrintBytes("To quote: ", inData, inSize);
    PrintBytes("Quoted : ", outData, *poutSize);
    fflush(g_logFile);
#endif
    return true;
}


// -------------------------------------------------------------------------


