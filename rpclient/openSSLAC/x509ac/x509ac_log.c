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
#include "x509ac_log.h"

#ifdef __cplusplus
extern "C" {
#endif

int indent = 0;


void TRACE(const char *fname,const char *fmt, ...)
{
#if defined(_DEBUG)
  char fmt2[100];
  va_list ap;
  int i,j=0;
  char buf[1024];
  
  FILE *milog;
  time_t ltime;
  struct tm *today;
  

  va_start(ap,fmt);
  j=sprintf(fmt2,"%s : ",fname/*""*/);
  for(i=0;i<indent;i++){
  	  j+=sprintf(fmt2+j,""/*"\t"*/);
  }
  j+=sprintf(fmt2+j,"%s",fmt);
  vsprintf(buf,fmt2,ap);
  milog=fopen("log.log","a+");
  time( &ltime );
  today = localtime( &ltime );
  fprintf(milog,"[%.24s]",asctime(today));
  fprintf(milog,"[P:%d,T:%d]",GetCurrentProcessId(),GetCurrentThreadId());
  for(i=0;i<indent;i++){
  	  fprintf(milog,"   ");
  }
  
  fprintf(milog,buf);
  fprintf(milog,"\n");
  fclose(milog);
  va_end(ap);
  return;
#endif	
}

void TraceIn(){
#ifdef _DEBUG
	indent++;
#endif
}

void TraceOut(){
#ifdef _DEBUG
	indent--;
#endif
}


void DumpBin(unsigned char* pointer,int size){
#ifdef _DEBUG
FILE *mylog;
	int count=0;
	int count1=0;
	int count2=0;
	int j = 0;
	mylog=fopen( "x509ac.log", "a+" );
	for (j=0;j<indent;j++){
		fprintf(mylog,"   ");
	}
	fprintf(mylog,"                         ");
	fprintf(mylog,"Size = %d \n",size);
	fprintf(mylog,"                         ");
	for (j=0;j<indent;j++){
		fprintf(mylog,"   ");
	}
	for (count=0;count<size;count++){
	fprintf(mylog,"0x%02x. ",*pointer);
	numero++;
	count1++;
	count2++;
	if (count1==8){
		fprintf(mylog,"\n");
		fprintf(mylog,"                         ");
		if (count2!=size){
		for (j=0;j<indent;j++){
		fprintf(mylog,"   ");
		}
		}
		count1=0;
	}
	
	}
	if (count1!=0)
		fprintf(mylog,"\n");

	fprintf(mylog,"\n");
	fclose(mylog);
#endif
}

void PrintRSAKey(RSA* Key){
#ifdef _DEBUG
	
	unsigned char buffer[1024];
	int bufferLen = 1024;

	if(Key->n != NULL){
	TRACE("public modulus n","");
	bufferLen = BN_bn2bin(Key->n,(unsigned char*)buffer);
	DumpBin(buffer,bufferLen);
	}
	if(Key->e != NULL){
	TRACE("public exponenet e","");
	bufferLen = BN_bn2bin(Key->e,(unsigned char*)buffer);
	DumpBin(buffer,bufferLen);
	}
	if(Key->p != NULL){
	TRACE("p","");
	bufferLen = BN_bn2bin(Key->p,(unsigned char*)buffer);
	DumpBin(buffer,bufferLen);
	}
	if(Key->q != NULL){
	TRACE("q","");
	bufferLen = BN_bn2bin(Key->q,(unsigned char*)buffer);
	DumpBin(buffer,bufferLen);
	}
	if(Key->dmp1 != NULL){
	TRACE("dmp1","");
	bufferLen = BN_bn2bin(Key->dmp1,(unsigned char*)buffer);
	DumpBin(buffer,bufferLen);
	}
	if(Key->dmq1 != NULL){
	TRACE("dmq1","");
	bufferLen = BN_bn2bin(Key->dmq1,(unsigned char*)buffer);
	DumpBin(buffer,bufferLen);
	}
	if(Key->iqmp != NULL){
	TRACE("iqmp","");
	bufferLen = BN_bn2bin(Key->iqmp,(unsigned char*)buffer);
	DumpBin(buffer,bufferLen);
	}
	if(Key->d != NULL){
	TRACE("Private exponent","");
	bufferLen = BN_bn2bin(Key->d,(unsigned char*)buffer);
	DumpBin(buffer,bufferLen);
	}
#endif
}


#ifdef __cplusplus
}
#endif


