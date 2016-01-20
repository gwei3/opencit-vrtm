
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#endif
#include "channelcoding.h"
#include "parser.h"
#include "logging.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __linux__
#include "safe_lib.h"
#endif
#ifdef __cplusplus
}
#endif

// ------------------------------------------------------------------------------

#define RPC2BUF 	"xmlrpc_buf"
#define RPC2ARGS	"xmlrpc_args"

 int g_max_uuid = 48;
 int g_sz_uuid = 36;
 

int encodeVM2RP_STARTAPP(const char* file, int nargs, char** args, 
                                   int bufsize, byte* buf)
{
	LOG_TRACE("Encode Start App request");
	memset(buf, 0, bufsize);
   return args_to_xmlrpc((char*)file, nargs, args, bufsize, buf);
}


bool  decodeVM2RP_STARTAPP(char** psz, int* pnargs, 
                                     char** args, const byte* buf)
{
	LOG_TRACE("Decode Start App request");
 	int status = xmlrpc_to_args (psz, pnargs, args, buf);
    return (status > 0);
}

int  encodeRP2VM_STARTAPP(byte* response, int responsesize, int bufsize, byte* buf)
{
	LOG_INFO("");
    if(buf==NULL)
        return false;
    //*((int*) buf)= procid;
    return cbuf_to_xmlrpc("encode_call", "StartApp", responsesize, response, bufsize, buf);

}

bool  decodeRP2VM_STARTAPP( int* data_size, byte * data, const byte* buf)
{
	LOG_TRACE("");
    if(buf==NULL)
        return false;
    //*pprocid= *((int*) buf);
    int status = xmlrpc_to_cbuf("", data_size, data, buf );
    return (status > 0);
}


int encodeVM2RP_SETVM_STATUS(const char* uuid, int vm_status, int bufsize, byte* buf)
{
   LOG_TRACE("Encode Set UUID request");
   const char* args[2];
   memset(buf, 0, bufsize);
   LOG_DEBUG("uuid : %s vm_status : %d", uuid, vm_status);
   args[0] = uuid;
   char status[64];
   snprintf(status, sizeof(status), "%d", vm_status);
   args[1] = status;
   LOG_TRACE("");
   return args_to_xmlrpc((char*)"set_vm_uuid", 2, (char**)args, bufsize, buf);
}


bool  decodeVM2RP_SETVM_STATUS(char** psz, int* pnargs, 
                                     char** args, const byte* buf)
{
    LOG_TRACE("Decode Set UUID request");
 	int status = xmlrpc_to_args (psz, pnargs, args, buf);
    return (status > 0);
}

// NOT IN USE
int  encodeRP2VM_SETVM_STATUS(byte* response, int responsesize, int bufsize, byte* buf)
{
	LOG_TRACE("");
    if(buf==NULL)
        return false;
    //*((int*) buf)= result;
    return cbuf_to_xmlrpc("encode_call", "update_vm_status", responsesize, response, bufsize, buf );
}

bool  decodeRP2VM_SETVM_STATUS(int* data_size, byte * data, const byte* buf)
{
	LOG_TRACE("");
    if(buf==NULL)
        return false;
    //*presult = *((int*) buf);
    int status = xmlrpc_to_cbuf("", data_size, data, buf );
    return (status > 0);
}


//
int  encodeVM2RP_TERMINATEAPP(int size, const byte* data, int bufsize, byte* buf)
{
	LOG_TRACE("Encode Terminate App request");
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_call", "delete_vm", size, data, bufsize, buf);
}


bool  decodeVM2RP_TERMINATEAPP(char** method_name, int* pnargs,
                                     char** args, const byte* buf)
{
    LOG_TRACE("Decode Terminate App request");
    int status = xmlrpc_to_args (method_name, pnargs, args, buf);
    return (status > 0);
}


// NOT IN USE
int  encodeRP2VM_TERMINATEAPP(int size, const byte* data, int bufsize, byte* buf)
{
	LOG_TRACE("");
	memset(buf, 0, bufsize);
	return 	cbuf_to_xmlrpc("encode_response", "", size, data, bufsize, buf);
}


// NOT IN USE
bool  decodeRP2VM_TERMINATEAPP(int* psize, byte* data, const byte* buf)
{
	LOG_TRACE("");
	int status = xmlrpc_to_cbuf (RPC2BUF, psize,  data, buf);
    return (status > 0);
}


/***********************encoding and decoding functions for GETRPID and GETVMMETA****************/
bool  decodeRP2VM_GETRPID(char** method_name, int* pnargs, char** args, const byte* buf)
{
    LOG_TRACE("Decode Get RPID request");
    int status = xmlrpc_to_args (method_name, pnargs, args, buf);
    return (status > 0);
}

int encodeRP2VM_GETRPID(int size,byte *data, int bufsize, byte *buf)
{
	LOG_TRACE("Encode get RPID request");
	memset(buf, 0, bufsize);
	return  cbuf_to_xmlrpc("encode_response", "", size, data, bufsize, buf);
}


bool decodeRP2VM_GETVMMETA(char** method_name, int* pnargs, char** args, const byte* buf)
{
    LOG_TRACE("Decode Get VM Metadata request");
    int status = xmlrpc_to_args (method_name, pnargs, args, buf);
    return (status > 0);
}

int encodeRP2VM_GETVMMETA(int numofMetadata, byte * metadata[], int bufsize, byte *buf)
{
	LOG_TRACE("Encode Get VM Metadata request");
	memset(buf, 0, bufsize);
	return args_to_xmlrpc((char*)"get_vmmeta", numofMetadata, (char**)metadata, bufsize, buf);
}

bool decodeRP2VM_ISVERIFIED(char** method_name, int* pnargs, char** args, const byte* buf)
{
    LOG_TRACE("Decode Is Verified request");
    int status = xmlrpc_to_args (method_name, pnargs, args, buf);
    return (status > 0);
}

int encodeRP2VM_ISVERIFIED(int size, byte *data, int bufsize, byte * buf)
{
	LOG_TRACE("Encode Is Verified request");
	memset(buf, 0, bufsize);
	return  cbuf_to_xmlrpc("encode_response", "", size, data, bufsize, buf);
}

bool decodeRP2VM_GETVMREPORT( char ** psz, int * pnargs,
                                      char** args, const byte* buf)
{
	LOG_TRACE("Decode Get VM Report request");
	int status = xmlrpc_to_args (psz, pnargs, args, buf);
 //   int status = xmlrpc_to_cbuf(RPC2BUF,psize,data,buf);
        return (status > 0);
}

int encodeRP2VM_GETVMREPORT(int size, byte *data, int bufsize, byte * buf)
{
	LOG_TRACE("Encode Get VM Report request");
       memset(buf, 0, bufsize);
       return  cbuf_to_xmlrpc("encode_response", "", size, data, bufsize, buf);
}


// ------------------------------------------------------------------------------


