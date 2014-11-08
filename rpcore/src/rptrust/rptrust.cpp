
//  File: rptrust.cpp
//    Vinay Phegade 
//
//  Description: Reliance Point Trust establishment services 
//
//  Copyright (c) 2011, Intel Corporation. 
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
/*#include "rptrust.h"
#include "jlmUtility.h"
#include "rpapilocal.h"
#include "channelcoding.h"
#include <ctype.h>
#include "cert.h"
#include "quote.h"
#include "cryptoHelper.h"
#include "validateEvidence.h"
*/
#include "jlmTypes.h"
#include "channelcoding.h"
#include "rptrust.h"
#include "tcpchan.h"
#include "pyifc.h"



// ---------------------------------------------------------------------------------------

extern 	int g_rpchan_fd;

static void PrintBytes(const char* szMsg, byte* pbData, int iSize)
{
    int i;

    printf("\t%s: ", szMsg);
    for (i= 0; i<iSize; i++) {
	
			printf("%02x", pbData[i]);
    }
    printf("\n");
}

#if 0
int channel_open() {
    int fd = -1;

	fd =  ch_open(NULL, 0);


	if(fd < 0)
	{
        	fprintf(stdout, "Cannot connect: %s\n", strerror(errno));
        	return -1;
    }
    
  	ch_register(fd);  
    return fd;

}
#endif

int doTrustOp(int opcode, u32 *putype, int inSize, byte* inData, int* poutSize, byte* outData)
{
	int rc = -1;


	
	int         size= PARAMSIZE;
	byte        rgBuf[PARAMSIZE] = {0};
    char		*str_type;
    
    tcBuffer*   pReq= (tcBuffer*) rgBuf;

#ifdef TEST	
	printf("Function entered doTrustOp\n");
#endif
	if ( (poutSize == NULL) || (outData == NULL))
		return false;

	if(g_rpchan_fd <= 0)
	{
#ifdef TEST 
        	fprintf(stdout, "Channel  error : %s\n", strerror(errno));
#endif
        	return false;
    	}
    
    size = sizeof(tcBuffer);
    
    memset(outData, 0, *poutSize);
    
    switch (opcode){
		case VM2RP_ATTESTFOR:
			size = encodeVM2RP_ATTESTFOR(inSize, inData,  PARAMSIZE -size, &rgBuf[size]);
			break;
		case VM2RP_GETOSHASH:
			size = encodeVM2RP_GETOSHASH(*putype, PARAMSIZE-size,  &rgBuf[size]);
			break;
		case VM2RP_GETOSCERT:
			size = encodeVM2RP_GETOSCERT(PARAMSIZE-size,  &rgBuf[size]);
			break;
		case VM2RP_GETPROGHASH:
			size = encodeVM2RP_GETPROGHASH(0 /*pid*/, PARAMSIZE -size, &rgBuf[size]);
			break;
		case VM2RP_SEALFOR:
			size = encodeVM2RP_SEALFOR(inSize, inData,  PARAMSIZE -size, &rgBuf[size]);
			break;
		case VM2RP_UNSEALFOR:			
			size = encodeVM2RP_UNSEALFOR(inSize, inData,  PARAMSIZE -size, &rgBuf[size]);
			break;
        	/*case VM2RP_IS_MEASURED:
			size = encodeVM2RP_IS_MEASURED(inData, PARAMSIZE -size, &rgBuf[size]);
			break;*/
	        default:
			return false;
	}
	
    if(size<0) {
#ifdef TEST 
        fprintf(stdout, "getQuote: encodeVM2RP_ATTESTFOR failed\n");
#endif
        return false;
    }

#ifdef TEST 
	fprintf(stdout, ">>>> opcode %d inSize %d, size %d\n", opcode, inSize, size);
#endif

	pReq->m_procid= 0;
	pReq->m_reqID= opcode;
	pReq->m_ustatus= 0;
	pReq->m_origprocid= 0;
	pReq->m_reqSize= size;

//PrintBytes(" Encoded data - before write ", (byte*)rgBuf, size + sizeof(tcBuffer));

	rc = ch_write(g_rpchan_fd, rgBuf, size + sizeof(tcBuffer) );
	if (rc < 0){
		fprintf(stdout, "write error: %s\n", strerror(errno));
		goto fail;
	}


	size = 0;
	memset(rgBuf, 0, sizeof(rgBuf));
	
	size = ch_read(g_rpchan_fd, rgBuf+size, sizeof(rgBuf));
	if (size < 0){
		fprintf(stdout, "read error:%d  %s\n", errno, strerror(errno));
		goto fail;
	}
		

//PrintBytes(" Encoded data - after read ", (byte*)rgBuf, size );	

	size -= sizeof(tcBuffer);
	
	
	
	switch (opcode){
		case VM2RP_ATTESTFOR:
			rc = decodeRP2VM_ATTESTFOR(poutSize, outData, &rgBuf[sizeof(tcBuffer)]);
			fprintf(stdout, "VM2RP_ATTESTFOR: data size=%d\n", *poutSize);
			break;
		case VM2RP_GETOSHASH:
			size =  decodeRP2VM_GETOSHASH(&str_type, poutSize, outData, size, &rgBuf[sizeof(tcBuffer)]);
			break;
		case VM2RP_GETOSCERT:
			size =  decodeRP2VM_GETOSCERT(&str_type, poutSize, outData, size, &rgBuf[sizeof(tcBuffer)]);
			break;
		case VM2RP_GETPROGHASH: /*last 2 params not used */
			size =  decodeRP2VM_GETPROGHASH(&str_type, poutSize, outData, size, &rgBuf[sizeof(tcBuffer)]);
			break;
		case VM2RP_SEALFOR:
			rc = decodeRP2VM_SEALFOR(poutSize, outData, &rgBuf[sizeof(tcBuffer)]);
			break;
		case VM2RP_UNSEALFOR:
			rc = decodeRP2VM_UNSEALFOR(poutSize, outData, &rgBuf[sizeof(tcBuffer)]);
			break;
		/*case VM2RP_IS_MEASURED:
			rc = decodeRP2VM_IS_MEASURED(outData, &rgBuf[sizeof(tcBuffer)]);
			break;*/
	}
	
   if(!rc) {
#ifdef TEST 
        fprintf(stdout, "getQuote: gettcBuf for decodeRP2VM_ATTESTFORfailed\n");
#endif
        return false;
    }
	
    
    size = *poutSize ;

//    fpritf(stdout, ">>>> data returned by server size %d\n", size);
//    PrintBytes(" Decoded data - after read ", (byte*)outData, size );
fail:

    return true;
}


int doTrustOp_IS_MEASURED(int opcode, char* inData, int* poutSize, int* outData)
{
	int rc = -1;
	int         size= PARAMSIZE;
	byte        rgBuf[PARAMSIZE] = {0};
    
    tcBuffer*   pReq= (tcBuffer*) rgBuf;

#ifdef TEST	
	printf("Function entered doTrustOp\n");
#endif
	if ( (poutSize == NULL) || (outData == NULL))
		return false;

	if(g_rpchan_fd <= 0)
	{
#ifdef TEST 
        	fprintf(stdout, "Channel  error : %s\n", strerror(errno));
#endif
        	return false;
    	}
    
    size = sizeof(tcBuffer);
    
    memset(outData, 0, *poutSize);
    
    const char * cindata = (const char *)inData;
    size = encodeVM2RP_IS_MEASURED(cindata, PARAMSIZE -size, &rgBuf[size]);
	
    if(size<0) {
#ifdef TEST 
        fprintf(stdout, "getQuote: encodeVM2RP_ATTESTFOR failed\n");
#endif
        return false;
    }

#ifdef TEST 
	fprintf(stdout, ">>>> opcode %d \n", opcode);
#endif

	pReq->m_procid= 0;
	pReq->m_reqID= opcode;
	pReq->m_ustatus= 0;
	pReq->m_origprocid= 0;
	pReq->m_reqSize= size;

//PrintBytes(" Encoded data - before write ", (byte*)rgBuf, size + sizeof(tcBuffer));

	rc = ch_write(g_rpchan_fd, rgBuf, size + sizeof(tcBuffer) );
	if (rc < 0){
		fprintf(stdout, "write error: %s\n", strerror(errno));
		goto fail;
	}


	size = 0;
	memset(rgBuf, 0, sizeof(rgBuf));
	
	size = ch_read(g_rpchan_fd, rgBuf+size, sizeof(rgBuf));
	if (size < 0){
		fprintf(stdout, "read error:%d  %s\n", errno, strerror(errno));
		goto fail;
	}
		

//PrintBytes(" Encoded data - after read ", (byte*)rgBuf, size );	

	size -= sizeof(tcBuffer);
	rc = decodeRP2VM_IS_MEASURED(outData, &rgBuf[sizeof(tcBuffer)]);
	
	
   if(!rc) {
#ifdef TEST 
        fprintf(stdout, "getQuote: gettcBuf for decodeRP2VM_ATTESTFORfailed\n");
#endif
        return false;
    }
	
    
    size = *poutSize ;

//    fpritf(stdout, ">>>> data returned by server size %d\n", size);
//    PrintBytes(" Decoded data - after read ", (byte*)outData, size );
fail:

    return true;
}


/*

//
// v: this was duplicate functionality
//
* 
int doTrustOpX(int opcode, int inSize, byte* inData, int* poutSize, byte* outData) {

        int g_rpchan_fd;
        int rc = -1;
        int i = 0;
        int size= 0;
        byte rgBuf[PARAMSIZE] = {0};
        tcBuffer* pReq= (tcBuffer *)rgBuf;
        
        g_rpchan_fd = open("/dev/chandu", O_RDWR); 
        if(g_rpchan_fd < 0)
        {
            fprintf(stdout, "Open error chandu: %s\n", strerror(errno));
            return -10;
        }

       size = sizeof(tcBuffer);
       memset(outData, 0, *poutSize);
	switch (opcode){
	case VM2RP_ATTESTFOR:
            size = encodeVM2RP_ATTESTFOR(inSize, inData, PARAMSIZE - size, &rgBuf[size]);
             break;
	case VM2RP_SEALFOR:
            size = encodeVM2RP_SEALFOR(inSize, inData, PARAMSIZE - size, &rgBuf[size]);
            break;
	case VM2RP_UNSEALFOR:
	    PrintBytes(" Input param checking before encode", (byte*)inData, inSize);
            size = encodeVM2RP_UNSEALFOR(inSize, inData, PARAMSIZE - size, &rgBuf[size]);
            break;
	default:
            size= -1;
            break;
	}

       if(size < 0) {
            fprintf(stdout, "getQuote: encodeVM2RP_ATTESTFOR failed\n");
            close (g_rpchan_fd);
            return -11;
       }
       pReq->m_procid= 0;
       pReq->m_reqID= opcode;
       pReq->m_ustatus= 0;
       pReq->m_origprocid= 0;
       pReq->m_reqSize= size;

       rc = write(g_rpchan_fd, rgBuf, size + sizeof(tcBuffer) );
       if (rc < 0){
           fprintf(stdout, "write error: %s\n", strerror(errno));
           goto fail;
       }
again:
       rc = read(g_rpchan_fd, rgBuf, sizeof(rgBuf));
       if (rc < 0){
           if (errno == 11) {
               sleep(1);
               goto again;
           }

           fprintf(stdout, "read error:%d  %s\n", errno, strerror(errno));
           goto fail;
       }
       size = rc;
       size-= sizeof(tcBuffer);
       switch (opcode){
            case VM2RP_ATTESTFOR:
                    rc = decodeRP2VM_ATTESTFOR(poutSize, outData, &rgBuf[sizeof(tcBuffer)]);
                    break;
            case VM2RP_SEALFOR:
                    rc = decodeRP2VM_SEALFOR(poutSize, outData, &rgBuf[sizeof(tcBuffer)]);
                    break;
            case VM2RP_UNSEALFOR:
                    rc = decodeRP2VM_UNSEALFOR(poutSize, outData, &rgBuf[sizeof(tcBuffer)]);
                    break;
	    default:
                rc= 0;
                break;
       }

       if(!rc) {
           fprintf(stdout, "getQuote: gettcBuf for decodeRP2VM_ATTESTFORfailed\n");
           close (g_rpchan_fd);
           return -12;
       }
       size = *poutSize;
fail:
       if ( g_rpchan_fd >= 0)
           close (g_rpchan_fd);
	return rc;
}

*/


bool isMeasured(char * uuid)
{
	int iRet;
	int isMeasured;
	int length = sizeof(int);
	if(uuid == NULL)  {
            return -1;
	}
	iRet= doTrustOp_IS_MEASURED(VM2RP_IS_MEASURED, uuid, &length, &isMeasured);
	return iRet;
}

int Seal(int secretlen, byte * secret,  int* pdataoutLen, byte *dataout)
{
	int iRet;
        u32 uType = 0;
	if((secret == NULL) || (dataout == NULL) || (pdataoutLen == NULL) ) {
            return -1;
	}
	if((secretlen<= 0) )  {
            return -2;
        }

	iRet= doTrustOp(VM2RP_SEALFOR, &uType, secretlen, secret,
                         (int *)pdataoutLen, dataout );
	return iRet;
}

int UnSeal(int datalen, byte *data, int *psecretlen, byte * secret)
{
	int iRet;
        u32 uType = 0;

	if((data == NULL) || (secret == NULL) || (psecretlen == NULL)) {
            return -1;
	}
	if((datalen <= 0) )  {
            return -2;
    }
	iRet= doTrustOp(VM2RP_UNSEALFOR, &uType, datalen, data,
                        (int *)psecretlen, secret);
	return iRet;
}


int Attest(int datainlen, byte *datain, int* pdataoutlen, byte *dataout)
{
	int iRet;
        u32 uType = 0;

	if((datain == NULL) || (dataout == NULL) || (pdataoutlen == NULL)) {
            return -1;
	}
	if(datainlen <= 0 )  {
            return -2;
        }
	iRet= doTrustOp(VM2RP_ATTESTFOR, &uType, datainlen, datain,
                        (int *)pdataoutlen, dataout);
	return iRet;
}

int getHostedComponentMeasurement(unsigned *ptype, int *pdataoutlen, byte *dataout)
{
	int iRet;
	u32 uType = 0;

	if((ptype == NULL) || (dataout == NULL) || (pdataoutlen == NULL)) {
            return -1;
	}
	
        
	iRet= doTrustOp(VM2RP_GETPROGHASH, ptype, 0, NULL, (int *)pdataoutlen, dataout);

	return iRet;
}

int getHostMeasurement(unsigned *ptype, int *pdataoutlen, byte *dataout)
{
	int iRet;
	u32 uType = 0;

	if((ptype == NULL) || (dataout == NULL) || (pdataoutlen == NULL)) {
            return -1;
	}
	
        
	iRet= doTrustOp(VM2RP_GETOSHASH, ptype, 0, NULL, (int *)pdataoutlen, dataout);

	return iRet;
}

int getHostCertChain (int *pdataoutlen, byte *dataout)
{
	int iRet;
	if((dataout == NULL) || (pdataoutlen == NULL)) {
            return -1;
	}
	
	// Get the certificate chain from RPCore
    iRet= doTrustOp(VM2RP_GETOSCERT, 0, 0, NULL, pdataoutlen, (byte*)dataout);
    
    return iRet;
}

#if 0
int main(int argc, char** argv){


	if (init_pyifc("rppy_ifc") < 0 ) {
		printf("init_pyifc returned error \n");
	}

	/******* gen quote *******************************/
	char * c = "Challenge";
	char *q = "Quote";
	int size = 5;
	int n = 12345;
	byte* a = (byte*)&n;
	GenQuote(size, a, c, &q);
	fprintf(stdout, "%s", q);

	/******* verify quote *******************************/
	char* start = strstr(q, "<ds:KeyInfo");
	char* end = strstr(q, "ds:KeyInfo>");
	int len = end-start+strlen("ds:KeyInfo>");
	char szroot[1024];
	memcpy(szroot, start, len);
	szroot[len] = '\n';
	szroot[len+1] = '\0';
	fprintf(stdout, "keyinfo: \n\n%s\n", szroot);
	VerifyQuote(q, szroot);

	char buf[1024*4] = {0};
	char sealed[1024] = {0};

	int i = 1024*4;
	int dlen = 1024;	
	char * indata = "Please tell who am I?";
	
	u32 utype = 0;

	i = 1024*4;
	/******* attest *******************************/
	doTrustOp(VM2RP_ATTESTFOR, &utype, strlen(indata), (byte*)indata, &i, (byte*)buf);
	PrintBytes(" Attestation ", (byte*)buf, i );


/*
	PrintBytes("Sealing secret ", (byte*)indata, strlen(indata));
	doTrustOp(VM2RP_SEALFOR, &utype, strlen(indata), (byte*)indata, &dlen, (byte*)sealed);
	PrintBytes(" Sealed data ", (byte*)sealed, dlen);
	i = 1024*4;
	doTrustOp(VM2RP_UNSEALFOR, &utype, dlen, (byte*)sealed, &i, (byte*)buf);
	
	PrintBytes(" Unsealed data ", (byte*)buf, i );
	fprintf(stdout, "\tOriginal %s  \n\tunsealed %s\n", indata, buf);

	i = 1024*4;
	doTrustOp(VM2RP_GETPROGHASH, &utype, strlen(indata), (byte*)indata, &i, (byte*)buf);
	PrintBytes(" VM hash ", (byte*)buf, i );
	
	i = 1024*4;
	doTrustOp(VM2RP_ATTESTFOR, &utype, strlen(indata), (byte*)indata, &i, (byte*)buf);
	PrintBytes(" Attestation ", (byte*)buf, i );

	i = 1024*4;
	doTrustOp(VM2RP_GETOSCREDS, &utype, strlen(indata), (byte*)indata, &i, (byte*)buf);
	fprintf(stdout, "Evidence: \n%s\n", buf);
	
	i = 1024*4;
	doTrustOp(VM2RP_GETOSCERT, &utype, strlen(indata), (byte*)indata, &i, (byte*)buf);
	fprintf(stdout, "Host cert: \n%s\n", buf);
*/	
	return 0;
}
#endif
