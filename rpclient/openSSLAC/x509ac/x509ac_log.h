/*****************************************************************
*
* This code has been developed at:
*************************************
* Pervasive Computing Laboratory
*************************************
* Telematic Engineering Dept.
* Carlos III university
* Contact:
*		Daniel Díaz Sanchez
*		Florina Almenarez
*		Andrés Marín
*************************************		
* Mail:	dds[@_@]it.uc3m.es
* Web: http://www.it.uc3m.es/dds
* Blog: http://rubinstein.gast.it.uc3m.es/research/dds
* Team: http://www.it.uc3m.es/pervasive
**********************************************************
* This code is released under the terms of OpenSSL license
* http://www.openssl.org
*****************************************************************/
#ifndef _X509AC_LOG_H_
#define _X509AC_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#	include <windows.h>
#endif
#include <stdio.h>
#include <openssl/rsa.h>
#include <openssl/err.h>


#define TRACEINIT(a) char* fname = a;\
	TraceIn();\
	TRACE(fname,"%s","entering...");

#define TRACEEND(b) b==TRUE?TRACE(fname,"%s","exiting...Ok"):TRACE(fname,"%s","exiting...Fail");\
	TraceOut();

#define MaxfTypesStringFormatLength 8
#define MaxfTypesDescriptorLength 12
#define MaxDataDetail 128
#define MaxfTypesFullLength 128
#define MaxLogReasonLength 48

void Pinta_Long(unsigned long);
void Pinta_Hexa(unsigned long);
void DumpBin(unsigned char* numero,int size);


void Trace_Log(char *FunctionName,char *LogReason,int space);

void PrintRSAKey(RSA*);
void TRACE(const char *fname,const char *fmt, ...);
void TraceIn();
void TraceOut();


#ifdef __cplusplus
}
#endif

#endif


