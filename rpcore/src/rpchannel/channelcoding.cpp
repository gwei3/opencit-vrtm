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

// Todo: security problem with overflow here
int encodeVM2RP_SEALFOR(int sealsize, const byte* seal, int bufsize, 
                                  byte* buf)
{
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_call", "seal_for",  sealsize, seal, bufsize, buf);
}


bool  decodeVM2RP_SEALFOR(int* psize, byte* data, const byte* buf)
{
	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int encodeRP2VM_SEALFOR(int sealedsize, const byte* sealed, int bufsize, byte* buf)
{
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_response", "", sealedsize, sealed, bufsize, buf);
}


bool  decodeRP2VM_SEALFOR(int* psize, byte* data, const byte* buf)
{
 	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int encodeVM2RP_UNSEALFOR(int sealedsize, const byte* sealed, int bufsize, byte* buf)
{
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_call", "unseal_for", sealedsize, sealed, bufsize, buf);
}


bool decodeVM2RP_UNSEALFOR(int* psize, byte* data, const byte* buf)
{
	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int encodeRP2VM_UNSEALFOR( int unsealsize, const byte* unseal, int bufsize, byte* buf)
{
   memset(buf, 0, bufsize);
   return 	cbuf_to_xmlrpc("encode_response", "", unsealsize, unseal, bufsize, buf);
}


bool  decodeRP2VM_UNSEALFOR(int* psize, 
                                        byte* data, const byte* buf)
{
    int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int encodeVM2RP_ATTESTFOR(int toattestsize, const byte* toattest, 
                                      int bufsize, byte* buf)
{
	memset(buf, 0, bufsize);
	int n_bytes = cbuf_to_xmlrpc("encode_call", "attest_for", toattestsize, toattest, bufsize, buf);
	return n_bytes;
}


bool  decodeVM2RP_ATTESTFOR(int* psize, byte* data, const byte* buf)
{	
	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int  encodeRP2VM_ATTESTFOR(int attestsize, const byte* attested, int bufsize, byte* buf)
{
	memset(buf, 0, bufsize);
	 return cbuf_to_xmlrpc("encode_response", "", attestsize, attested, bufsize, buf);
}


bool  decodeRP2VM_ATTESTFOR(int* pattestsize, byte* attested, const byte* buf)
{
    
    int status = xmlrpc_to_cbuf (RPC2BUF, pattestsize,  attested, buf);
    return (status > 0);
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

//
// Todo: security problem with overflow here

int encodeVM2RP_CHECK_VM_VDI(const char* uuid, const char* vdi_id,
                                   int bufsize, byte* buf)
{
   const char* args[3];
   memset(buf, 0, bufsize);

   args[0] = uuid;
   args[1] = vdi_id;
   return args_to_xmlrpc((char*)"check_vm_vdi", 2, (char**)args, bufsize, buf);
}


bool  decodeVM2RP_CHECK_VM_VDI(char** psz, int* pnargs,
                                     char** args, const byte* buf)
{
    int status = xmlrpc_to_args (psz, pnargs, args, buf);
   //printf("status: %d", status);
    return (status > 0);
}


int  encodeRP2VM_CHECK_VM_VDI(int result, byte* buf)
{
    if(buf==NULL)
        return false;
    *((int*) buf)= result;
    return true;
}


bool  decodeRP2VM_CHECK_VM_VDI(int* presult, const byte* buf)
{
    if(buf==NULL)
        return false;
    *presult = *((int*) buf);
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


int encodeVM2RP_GETOSHASH(int pid, int bufsize, byte* buf)
{
	byte fake = 0x20;
    memset(buf, 0, bufsize);
    int n = cbuf_to_xmlrpc("encode_call", "get_os_hash", 1, &fake, bufsize, buf);
    
    return n;
}


bool  decodeVM2RP_GETOSHASH(int* psize, byte* data, const byte* buf)
{
    int n= 0;

   
    int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);

}

int encodeRP2VM_GETOSHASH(char* hashType, int size, const byte* hash, 
                                          int bufsize, byte* buf)
{
/* xmlrpc response returns only one element
   const char* args[2];
   char strhash[128] = {0};
      
   if (size > 128)
		return -1;
		
   memset(buf, 0, bufsize);
   args[0] = hashType;

   bin2ascii(size, hash, strhash);
   
   args[1] = strhash;
   
   int n_bytes = args_to_xmlrpc_response((char*)"", 2, (char**)args, bufsize, buf);
     return n_bytes;    
     * * */
   	memset(buf, 0, bufsize);
	return cbuf_to_xmlrpc("encode_response", "", size, hash, bufsize, buf);
}


bool  decodeRP2VM_GETOSHASH(char** phashType, int* psize, byte* hash, int bufsize, const byte* buf)
{
  /*  char* av[5];
    int an;
    char *psz = NULL;
    
   	int status = xmlrpc_to_args (&psz, &an, av, buf);
   
    if((status > 0) && (an > 1))
    {
		if (psz)
			free(psz);
			
		//extract parameters av[0]:type av[1]:value
		*phashType = av[0];
		ascii2bin(av[1], psize, hash);
		
		free(av[1]);
		
		return true;
	}
	
	
	return false;*/
	
	*phashType = NULL;
    int status = xmlrpc_to_cbuf (RPC2BUF, psize,  hash, buf);
    return (status > 0);
}



int encodeRP2VM_GETPROGHASH(char* hashType, int size, const byte* hash, 
                                          int bufsize, byte* buf)
{
	return encodeRP2VM_GETOSHASH(hashType, size, hash, bufsize, buf);
}


bool  decodeRP2VM_GETPROGHASH(char** phashType, int* psize, byte* hash, int bufsize, const byte* buf)
{
	return decodeRP2VM_GETOSHASH(phashType, psize, hash, bufsize, buf);
}


int encodeVM2RP_GETPROGHASH(int pid, int bufsize, byte* buf)
{
	byte fake = 0x20;
    memset(buf, 0, bufsize);
    int n = cbuf_to_xmlrpc("encode_call", "get_prog_hash",  1, &fake, bufsize, buf);
    return n;
}


bool  decodeVM2RP_GETPROGHASH(int* psize, byte* data, const byte* buf)
{
 
    int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);

}

int  encodeVM2RP_GETOSCREDS(uint32_t credType, int size, const byte* cred, 
                                       byte* buf)
{
    int n= 0;

    memcpy(&buf[n], (byte*)&credType, sizeof(uint32_t));
    n+= sizeof(uint32_t);
    memcpy(&buf[n], &size, sizeof(int));
    n+= sizeof(int);
    memcpy(&buf[n], cred, size);
    return true;
}


bool  decodeVM2RP_GETOSCREDS(uint32_t* pcredType, int* psize, byte* cred, 
                                       const byte* buf)
{
    int n= 0;

    memcpy(pcredType, buf, sizeof(uint32_t));
    n+= sizeof(uint32_t);
    memcpy(psize, &buf[n], sizeof(int));
    n+= sizeof(int);
    memcpy(cred, &buf[n], *psize);
    return true;
}


int encodeRP2VM_GETOSCREDS(uint32_t credType, int size, const byte* cred, 
                                      int bufsize, byte* buf)
{
    int n= 0;

    if(bufsize<(sizeof(uint32_t)+sizeof(int)+size))
        return -1;
    memcpy(&buf[n], (byte*)&credType, sizeof(uint32_t));
    n+= sizeof(uint32_t);
    memcpy(&buf[n], &size, sizeof(int));
    n+= sizeof(int);
    memcpy(&buf[n], cred, size);
    n+= size;
    return n;
}


bool  decodeRP2VM_GETOSCREDS(uint32_t* pcredType, int* psize, 
                                             byte* cred, const byte* buf)
{
    int n= 0;

    memcpy(pcredType, buf, sizeof(uint32_t));
    n+= sizeof(uint32_t);
    memcpy(psize, &buf[n], sizeof(int));
    n+= sizeof(int);
    memcpy(cred, &buf[n], *psize);
    return true;
}

int  encodeVM2RP_GETOSCERT(int size, byte* buf)
{
    byte fake = 0x20;
	memset(buf, 0, size);
    int n = cbuf_to_xmlrpc("encode_call", "get_host_cert", 1, &fake, size, buf);
    return n;

}


bool  decodeVM2RP_GETOSCERT(int* psize, byte* data, const byte* buf)
{
   	
    int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int encodeRP2VM_GETOSCERT(char* type, int size, const byte* certchain,  int bufsize, byte* buf)
{
    //RPC encoding is same as encodeRP2VM_GETOSHASH
    return encodeRP2VM_GETOSHASH(type, size, certchain, bufsize, buf);
}


bool  decodeRP2VM_GETOSCERT(char** pcert_type, int* psize,  byte* certchain, int bufsize, const byte* buf)
{
 
    return decodeRP2VM_GETOSHASH(pcert_type, psize, certchain, bufsize, buf);
}

// ------------------------------------------------------------------------------
//MH start of GETAIKCERT xmlrpc conversion

int  encodeVM2RP_GETAIKCERT(int size, byte* buf)
{
    byte fake = 0x20;
        memset(buf, 0, size);
    int n = cbuf_to_xmlrpc("encode_call", "get_aik_cert", 1, &fake, size, buf);
    return n;

}


bool  decodeVM2RP_GETAIKCERT(int* psize, byte* data, const byte* buf)
{

    int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int encodeRP2VM_GETAIKCERT(char* type, int size, const byte* certchain,  int bufsize, byte* buf)
{
    memset(buf, 0, bufsize);
    return cbuf_to_xmlrpc("encode_response", "", size, certchain, bufsize, buf);
}


bool  decodeRP2VM_GETAIKCERT(char** pcert_type, int* psize,  byte* certchain, int bufsize, const byte* buf)
{
    *pcert_type = NULL;
    int status = xmlrpc_to_cbuf (RPC2BUF, psize, certchain, buf);
    return (status > 0);
}
//MH end of GETAIKCERT xmlrpc conversion
// ------------------------------------------------------------------------------
//MH start of GETTPMQUOTE xmlrpc conversion
int encodeVM2RP_GETTPMQUOTE(int size, const byte* data, int bufsize,
                                  byte* buf)
{
    memset(buf, 0, bufsize);
    return  cbuf_to_xmlrpc("encode_call", "get_tpm_quote", size, data, bufsize, buf);
}


bool  decodeVM2RP_GETTPMQUOTE(int* psize, byte* data, const byte* buf)
{
    int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int encodeRP2VM_GETTPMQUOTE(int size, const byte* data, int bufsize, byte* buf)
{
    memset(buf, 0, bufsize);
    return  cbuf_to_xmlrpc("encode_response", "", size, data, bufsize, buf);
}


bool  decodeRP2VM_GETTPMQUOTE(int* psize, byte* data, const byte* buf)
{
    #ifdef TEST
    fprintf(stdout, "decodeRP2VM_GETTPMQUOTE called\n");
    #endif
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

//MH end of GETTPMQUOTE xmlrpc conversion

// ------------------------------------------------------------------------------



int encodeVM2RP_SETFILE(int size, const byte* data, int bufsize, byte* buf)
{
	#ifdef TEST
	fprintf(stdout, "encodeVM2RP_SETFILE called\n");
	#endif
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_call", "setfile", size, data, bufsize, buf);
}


bool  decodeVM2RP_SETFILE(int* psize, byte* data, const byte* buf)
{
 	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int encodeRP2VM_SETFILE(int size, const byte* data, int bufsize, byte* buf)
{
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_response", "", size, data, bufsize, buf);
 }


bool  decodeRP2VM_SETFILE(int* psize, byte* data, const byte* buf)
{
	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


// ------------------------------------------------------------------------------

int encodeVM2RP_GETRPHOSTQUOTE(int size, const byte* data, int bufsize, 
                                  byte* buf)
{
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_call", "get_rp_host_quote", size, data, bufsize, buf);
}


bool  decodeVM2RP_GETRPHOSTQUOTE(int* psize, byte* data, const byte* buf)
{
 	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int encodeRP2VM_GETRPHOSTQUOTE(int size, const byte* data, int bufsize, byte* buf)
{
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_response", "", size, data, bufsize, buf);
}


bool  decodeRP2VM_GETRPHOSTQUOTE(int* psize, byte* data, const byte* buf)
{
		#ifdef TEST
	fprintf(stdout, "decodeRP2VM_GETRPHOSTQUOTE called\n");
	#endif
	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


// ------------------------------------------------------------------------------

int encodeVM2RP_RPHOSTQUOTERESPONSE(int size, const byte* data, int bufsize, 
                                  byte* buf)
{
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_call", "host_quote_response", size, data, bufsize, buf);

}


bool  decodeVM2RP_RPHOSTQUOTERESPONSE(int* psize, byte* data, const byte* buf)
{
	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int encodeRP2VM_RPHOSTQUOTERESPONSE(int size, const byte* data, int bufsize, byte* buf)
{
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_response", "", size, data, bufsize, buf);
}


bool  decodeRP2VM_RPHOSTQUOTERESPONSE(int* psize, byte* data, const byte* buf)
{
	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}

//
/*
int  encodeVM2RP_IS_MEASURED(int size, const byte* data, int bufsize, byte* buf)
{
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_call", "is_measured",  size, data, bufsize, buf);
}


bool  decodeVM2RP_IS_MEASURED(int* psize, byte* data, const byte* buf)
{
	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


int  encodeRP2VM_IS_MEASURED(const char response, int bufsize, byte* buf)
{
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_response", "", sizeof(char),(byte*) &response, bufsize, buf);
}


bool  decodeRP2VM_IS_MEASURED(int * psize, byte *response, const byte* buf)
{
 	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  response, buf);
    return (status > 0);
}
*/

int encodeVM2RP_IS_MEASURED(const char* uuid, int bufsize, byte* buf)
{
   const char* args[3];
   memset(buf, 0, bufsize);

   args[0] = uuid;
   return args_to_xmlrpc((char*)"is_measured", 1, (char**)args, bufsize, buf);
}


bool decodeVM2RP_IS_MEASURED(char** psz, int* pnargs,
                                     char** args, const byte* buf)
{
        int status = xmlrpc_to_args (psz, pnargs, args, buf);
    return (status > 0);
}


int encodeRP2VM_IS_MEASURED(int result, byte* buf)
{
    if(buf==NULL)
        return false;
    *((int*) buf)= result;
    return true;
}


bool decodeRP2VM_IS_MEASURED(int* presult, const byte* buf)
{
    if(buf==NULL)
        return false;
    *presult = *((int*) buf);
    return true;
}




// ------------------------------------------------------------------------------


