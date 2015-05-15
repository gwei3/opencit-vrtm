//
//  File: buffercoding.cpp
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


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "channelcoding.h"
#include "pyifc.h"

// ------------------------------------------------------------------------------

#define RPC2BUF 	"xmlrpc_buf"
#define RPC2ARGS	"xmlrpc_args"

 int g_max_uuid = 48;
 int g_sz_uuid = 36;
 
static void bin2ascii(int iSize, const byte* pbData, char* szMsg)
{
    int i;

    for (i= 0; i<iSize; i++) {
	
			sprintf(&szMsg[2*i], "%02x", pbData[i]);
    }
    
    szMsg[2*iSize] = '\0';
}

static void ascii2bin(const char* szMsg, int *iSize, byte* pbData)
{
    int i = 0;
    int len = strlen(szMsg);
	*iSize = 0;
    for (i= 0; i < len; i = i+2) {
	
			sscanf(&szMsg[i], "%02x", &pbData[i/2]);
			(*iSize)++;
    }
    
}



int encodeVM2RP_STARTAPP(const char* file, int nargs, char** args, 
                                   int bufsize, byte* buf)
{
	memset(buf, 0, bufsize);
   return args_to_xmlrpc((char*)file, nargs, args, bufsize, buf);
}


bool  decodeVM2RP_STARTAPP(char** psz, int* pnargs, 
                                     char** args, const byte* buf)
{
 	int status = xmlrpc_to_args (psz, pnargs, args, buf);
    return (status > 0);
}


int  encodeRP2VM_STARTAPP(int procid, byte* buf)
{
    if(buf==NULL)
        return false;
    *((int*) buf)= procid;
    return true;
}


bool  decodeRP2VM_STARTAPP(int* pprocid, const byte* buf)
{
    if(buf==NULL)
        return false;
    *pprocid= *((int*) buf);
    return true;
}


int encodeVM2RP_SETUUID(const char* rpid, const char* uuid, const char* vdi_id, 
                                   int bufsize, byte* buf)
{
   const char* args[3];
   memset(buf, 0, bufsize);
   
   args[0] = rpid;
   args[1] = uuid;
   args[2] = vdi_id;
   return args_to_xmlrpc((char*)"set_vm_uuid", 3, (char**)args, bufsize, buf);
}


bool  decodeVM2RP_SETUUID(char** psz, int* pnargs, 
                                     char** args, const byte* buf)
{
 	int status = xmlrpc_to_args (psz, pnargs, args, buf);
    return (status > 0);
}


int  encodeRP2VM_SETUUID(int result, byte* buf)
{
    if(buf==NULL)
        return false;
    *((int*) buf)= result;
    return true;
}


bool  decodeRP2VM_SETUUID(int* presult, const byte* buf)
{
    if(buf==NULL)
        return false;
    *presult = *((int*) buf);
    return true;
}


//
int  encodeVM2RP_TERMINATEAPP(int size, const byte* data, int bufsize, byte* buf)
{
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_call", "delete_vm", size, data, bufsize, buf);
}


bool  decodeVM2RP_TERMINATEAPP(int* psize, byte* data, const byte* buf)
{
 	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int  encodeRP2VM_TERMINATEAPP(int size, const byte* data, int bufsize, byte* buf)
{
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_response", "", size, data, bufsize, buf);
}


bool  decodeRP2VM_TERMINATEAPP(int* psize, byte* data, const byte* buf)
{
	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


/***********************encoding and decoding functions for GETRPID and GETVMMETA****************/
bool  decodeRP2VM_GETRPID(int *psize, byte* data, const byte* buf)
{
	#ifdef TEST
    fprintf(stdout, "decodeRP2VM_GETRPID called\n");
    #endif
    int status = xmlrpc_to_cbuf(RPC2BUF,psize,data,buf);
    return (status > 0);
}

int encodeRP2VM_GETRPID(int size,byte *data, int bufsize, byte *buf)
{
	memset(buf,0,bufsize);
	return  cbuf_to_xmlrpc("encode_response", "", size, data, bufsize, buf);
}


bool decodeRP2VM_GETVMMETA(int * psize, byte *data, const byte * buf)
{
	#ifdef TEST
    fprintf(stdout, "decodeRP2VM_GETVMMETA called\n");
    #endif
    int status = xmlrpc_to_cbuf(RPC2BUF,psize,data,buf);
        return (status > 0);
}

int encodeRP2VM_GETVMMETA(int numofMetadata, byte * metadata[], int bufsize, byte *buf)
{

	//return args_to_xmlrpc((char*)"is_measured", 1, (char**)args, bufsize, buf);
	memset(buf,0,bufsize);
	//return cbuf_to_xmlrpc("encode_response","",size,data,bufsize,buf);
	return args_to_xmlrpc((char*)"get_vmmeta", numofMetadata, (char**)metadata, bufsize, buf);
}

bool decodeRP2VM_ISVERIFIED(int * psize, byte *data, const byte * buf)
{
	#ifdef TEST
    fprintf(stdout, "decodeRP2VM_ISVERIFIED called\n");
    #endif
      int status = xmlrpc_to_cbuf(RPC2BUF,psize,data,buf);
        return (status > 0);
}

int encodeRP2VM_ISVERIFIED(int size, byte *data, int bufsize, byte * buf)
{
	memset(buf,0,bufsize);
	return  cbuf_to_xmlrpc("encode_response", "", size, data, bufsize, buf);
}

bool decodeRP2VM_GETVMREPORT( char ** psz, int * pnargs,
                                      char** args, const byte* buf)
{
       #ifdef TEST
    fprintf(stdout, "decodeRP2VM_GETVMREPORT called\n");
    #endif
 int status = xmlrpc_to_args (psz, pnargs, args, buf);
 //   int status = xmlrpc_to_cbuf(RPC2BUF,psize,data,buf);
        return (status > 0);
}

int encodeRP2VM_GETVMREPORT(int size, byte *data, int bufsize, byte * buf)
{
       memset(buf,0,bufsize);
       return  cbuf_to_xmlrpc("encode_response", "", size, data, bufsize, buf);
}


// ------------------------------------------------------------------------------


