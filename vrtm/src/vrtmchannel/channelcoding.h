#ifndef _CHANNELCODING_H__
#define _CHANNELCODING_H__

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#endif
#include <stdint.h>
#include "vrtmCommon.h"

#ifndef byte
typedef unsigned char byte;
#endif

//start/measure VM
VRTMCHANNEL_DLLPORT int encodeVM2RP_STARTAPP(const char* file, int nargs, char** args, int bufsize, byte* buf);
VRTMCHANNEL_DLLPORT bool decodeVM2RP_STARTAPP(char** psz, int* pnargs, char**, const byte* buf);

VRTMCHANNEL_DLLPORT int   encodeRP2VM_STARTAPP(byte* response, int responsesize, int bufsize, byte* buf);
VRTMCHANNEL_DLLPORT bool  decodeRP2VM_STARTAPP(int* data_size, byte * data, const byte* buf);

//terminate app/vm
VRTMCHANNEL_DLLPORT int  encodeVM2RP_TERMINATEAPP(int size, const byte* data, int bufsize, byte* buf);
VRTMCHANNEL_DLLPORT bool decodeVM2RP_TERMINATEAPP(char** method_name, int* pnargs, char** args, const byte* buf);

VRTMCHANNEL_DLLPORT int  encodeRP2VM_TERMINATEAPP(int size, const byte* data, int bufsize, byte* buf);
VRTMCHANNEL_DLLPORT bool  decodeRP2VM_TERMINATEAPP(int* psize, byte* data, const byte* buf);


VRTMCHANNEL_DLLPORT int encodeVM2RP_SETVM_STATUS(const char* uuid, int vm_status, int bufsize, byte* buf);
VRTMCHANNEL_DLLPORT bool  decodeVM2RP_SETVM_STATUS(char** psz, int* pnargs, char** args, const byte* buf);
VRTMCHANNEL_DLLPORT int  encodeRP2VM_SETVM_STATUS(byte* response, int responsesize, int bufsize, byte* buf);
VRTMCHANNEL_DLLPORT bool  decodeRP2VM_SETVM_STATUS(int* data_size, byte * data, const byte* buf);
// get rpid
VRTMCHANNEL_DLLPORT bool  decodeRP2VM_GETRPID(char** method_name, int* pnargs, char** args, const byte* buf);
VRTMCHANNEL_DLLPORT int encodeRP2VM_GETRPID(int size, byte *data, int bufsize, byte *buf);
// get vm meta
VRTMCHANNEL_DLLPORT bool decodeRP2VM_GETVMMETA(char** method_name, int* pnargs, char** args, const byte* buf);
VRTMCHANNEL_DLLPORT int encodeRP2VM_GETVMMETA(int numofMetadata, byte * metadata[], int bufsize, byte *buf);

VRTMCHANNEL_DLLPORT int encodeRP2VM_ISVERIFIED(int size, byte *data, int bufsize, byte * buf);
VRTMCHANNEL_DLLPORT bool decodeRP2VM_ISVERIFIED(char** method_name, int* pnargs, char** args, const byte* buf);

VRTMCHANNEL_DLLPORT int encodeRP2VM_GETVMREPORT(int size, byte *data, int bufsize, byte * buf);
VRTMCHANNEL_DLLPORT bool decodeRP2VM_GETVMREPORT(char** psz, int* pnargs, char** args, const byte* buf);

#endif



// ------------------------------------------------------------------------------


