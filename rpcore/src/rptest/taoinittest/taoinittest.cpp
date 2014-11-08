#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tcpchan.h"
#include "channel.h"
#include "channelcoding.h"
#include "pyifc.h"
#include "rp_api_code.h"

#ifndef byte
typedef unsigned char byte;
#endif

#define PARAMSIZE 1024*16

#define TCSERVICEGETRPHOSTQUOTEFROMAPP            		1
#define TCSERVICERPHOSTQUOTERESPONSEFROMAPP            23


int g_fd = 0;
#define RPCORESERVICE_PORT 16005
#define KEYSERVERPORT 6001
#define BYPASS_KSS_CALL 1
int channel_open() {
    int fd = -1;
    
	fd =  ch_open(NULL, 0);

	if(fd < 0)
	{
        	fprintf(stdout, "Open error chandu: %s\n", strerror(errno));
        	return -1;
    }
    
    ch_register(fd);
    
    return fd;
}

#if 0

int encodeTCSERVICEGETRPHOSTQUOTEFROMAPP(int size, const byte* data, int bufsize, 
                                  byte* buf)
{
    int n= 0;

    if(bufsize<(sizeof(int)+size))
        return -1;
    memcpy(&buf[n], &size, sizeof(int));
    n+= sizeof(int);
    memcpy(&buf[n], data, size);
    n+= size;
    return n;
}


bool  decodeTCSERVICEGETRPHOSTQUOTEFROMTCSERVICE(int* ppdatasize, byte* pdata, const byte* buf)
{
    int n= 0;

    memcpy(ppdatasize, &buf[n], sizeof(int));
    n+= sizeof(int);
    memcpy(pdata, &buf[n], *ppdatasize);
    return true;
}

int encodeTCSERVICERPHOSTQUOTERESPONSEFROMAPP(int size, const byte* data, int bufsize, 
                                  byte* buf)
{
    int n= 0;

    if(bufsize<(sizeof(int)+size))
        return -1;
    memcpy(&buf[n], &size, sizeof(int));
    n+= sizeof(int);
    memcpy(&buf[n], data, size);
    n+= size;
    return n;
    
}


bool  decodeTCSERVICERPHOSTQUOTERESPONSEFROMTCSERVICE(int* ppdatasize, byte* pdata, const byte* buf)
{
    int n= 0;

    memcpy(ppdatasize, &buf[n], sizeof(int));
    n+= sizeof(int);
    memcpy(pdata, &buf[n], *ppdatasize);
    return true;
}
#endif

int doRPCoreOp(int opcode, uint32_t *putype, int inSize, byte* inData, int* poutSize, byte* outData)
{
	int rc = -1;
	int i = 0;

	int fd1;
	
	int         size= PARAMSIZE;
	byte        rgBuf[PARAMSIZE] = {0};
    tcBuffer*   pReq= (tcBuffer*) rgBuf;

	if ( (poutSize == NULL) || (outData == NULL))
	//		return -1;
		return false;


	//PrintBytes(" Input param ", (byte*)inData, inSize);

 	fd1 = channel_open();
	if(fd1 < 0)
	{
        	return false;
    }
    
    size = sizeof(tcBuffer);
    
    memset(outData, 0, *poutSize);
    
    switch (opcode){
		case VM2RP_GETRPHOSTQUOTE:
			size = encodeVM2RP_GETRPHOSTQUOTE(inSize, inData,  PARAMSIZE -size, &rgBuf[size]);
			break;
		case VM2RP_RPHOSTQUOTERESPONSE:
			size = encodeVM2RP_RPHOSTQUOTERESPONSE(inSize, inData,  PARAMSIZE -size, &rgBuf[size]);
			break;
		default:
			return false;
	}
	
    if(size<0) {
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

	rc = ch_write(fd1, rgBuf, size + sizeof(tcBuffer) );
	if (rc < 0){
		fprintf(stdout, "write error: %s\n", strerror(errno));
		goto fail;
	}

	memset(rgBuf, 0, sizeof(rgBuf));
	
	size = 0;
	size = ch_read(fd1, rgBuf+size, sizeof(rgBuf));
	if (size < 0){
		fprintf(stdout, "read error:%d  %s\n", errno, strerror(errno));
		goto fail;
	}

//PrintBytes(" Encoded data - after read ", (byte*)rgBuf, size );	
	pReq = (tcBuffer *) rgBuf;
	if ( pReq->m_ustatus )
		return false;
		
	size -= sizeof(tcBuffer);
	
	opcode++;
	
	switch (opcode){
		case RP2VM_GETRPHOSTQUOTE:
			rc = decodeRP2VM_GETRPHOSTQUOTE(poutSize, outData, &rgBuf[sizeof(tcBuffer)]);
			break;
			
		case RP2VM_RPHOSTQUOTERESPONSE:
			rc = decodeRP2VM_RPHOSTQUOTERESPONSE(poutSize, outData, &rgBuf[sizeof(tcBuffer)]);
			break;
	}
	
   if(!rc) {
        return false;
    }
	
    
    size = *poutSize ;


fail:
	if ( fd1 >= 0) {
		close (fd1);
	//Is it correct to fall through and return true?
	}
 
    return true;
}


int foomain(int argc, char** argv){

	printf("Inside foomain");	
char buf[1024*4] = {0};
char quote[1024*4] = {0};

	int i = 1024*4;
	int dlen = 1024*4;	
	char * indata = "Please tell who am I?";
	
	uint32_t utype = 0;

	printf("Querying quote from RPCore\n");
	
	doRPCoreOp(TCSERVICEGETRPHOSTQUOTEFROMAPP, &utype, strlen(indata), (byte*)indata, &dlen, (byte*)quote);
	
	printf("RPCOre host quote %d\n", utype);
	printf("CSR: \n %s", quote);
	
	i = 1024*4;
	doRPCoreOp(TCSERVICERPHOSTQUOTERESPONSEFROMAPP, &utype, dlen, (byte*)quote, &i, (byte*)buf);
	
	
	
	return 0;
}


int main(int argc, char** argv)
{
    	bool                fRet= true;
	int                 iError= 0;
	int                 fd= 0;
        struct sockaddr_in  server_addr;
        int                 slen= sizeof(struct sockaddr_in);
        char                rgBuf[PARAMSIZE];
        int                 n= 0;
        int                 type= 0;
        byte                multi= 0;
        byte                final= 0;
	int size =0;
	
	char buf[1024*4] = {0};
	char quote[1024*4] = {0};

	int i = 1024*4;
	int dlen = 1024*4;	
	char * indata = "Please tell who am I?";
	
	uint32_t utype = 0;
	char* ip_env = NULL;
	
	setenv("PYTHONPATH", "../scripts", 1);
	if (init_pyifc("rppy_ifc") < 0 )
		return -1;
	
	//foomain(0,NULL);
	FILE* fp =NULL;
	FILE* fp1=NULL;
	fp = fopen("/tmp/taoinit.log", "w");
	fp1 = fopen("/tmp/x509_csr_req", "w");
	try {
    
        // open socket
        fd= socket(AF_INET, SOCK_STREAM, 0);
        if(fd<0) 
            throw  "Can't get socket\n";
        memset((void*) &server_addr, 0, sizeof(struct sockaddr_in));


        server_addr.sin_family= AF_INET;
        //server_addr.sin_addr.s_addr= htonl(INADDR_ANY);
        ip_env = getenv("RPCORE_IPADDR");
		if (ip_env)
			inet_aton(ip_env, &server_addr.sin_addr);
		else
			inet_aton("127.0.0.1", &server_addr.sin_addr);
			
        server_addr.sin_port= htons(KEYSERVERPORT);
    
        iError= connect(fd, (const struct sockaddr*) &server_addr, (socklen_t) slen);
        if(iError<0)
            throw  "initializeKeys: can't connect";

		printf("Querying quote from RPCore\n");
		
		if ( ! doRPCoreOp(TCSERVICEGETRPHOSTQUOTEFROMAPP, &utype, strlen(indata), (byte*)indata, &dlen, (byte*)rgBuf) )
			throw "from RPCore service";
		
		printf("RPCOre host quote %d\n", utype);
		fprintf(fp, "Request to KSS %s\n", rgBuf);
	#ifdef BYPASS_KSS_CALL
		//fwrite(str , 1 , sizeof(str) , fp);
		fwrite(rgBuf, 1, dlen, fp1);
		//fprintf(fp1,rgBuf);
		fclose(fp1);
		//system("openssl genrsa -out /tmp/signing.key 1024");
		system("openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout /tmp/mtw_private_key -out /tmp/mtw_cert");
		system("echo 00 > /tmp/mtw_cert.srl");
		system("openssl x509 -req -in /tmp/x509_csr_req -out /tmp/cert -CA /tmp/mtw_cert -CAkey /tmp/mtw_private_key");
		FILE *fr;
                char *message;
                fr = fopen("/tmp/cert", "rb");
                /*create variable of stat*/
                struct stat stp = { 0 };
                /*These functions return information about a file. No permissions are required on the file itself*/
                stat("/tmp/cert", &stp);
                /*determine the size of data which is in file*/
                int filesize = stp.st_size;
                /*allocates the address to the message pointer and allocates memory*/
                message = (char *) malloc(sizeof(char) * filesize);
                if (fread(message, 1, filesize, fr) == -1) {
                    printf("\nerror in reading\n");
                    /**close the read file*/
                    fclose(fr);
                    /*free input string*/
                    free(message);
                }
               	fwrite(message, 1, filesize, stdout); 
		
		
		dlen=4096;	
		if ( ! doRPCoreOp(TCSERVICERPHOSTQUOTERESPONSEFROMAPP, &utype, filesize, (byte*)message, &dlen, (byte*)buf) )
                        throw "from RPCore service";

	#else
	       if((n=sendPacket(fd, (byte*)rgBuf, dlen, CHANNEL_NEGO, 0, 1))<0)
        	    throw  "Can't send Packet 1 in certNego\n";

	        // get and interpret response
        	if((n=getPacket(fd, (byte*)rgBuf, PARAMSIZE, (int*)&utype, &multi, &final))<0)
	            throw  "Can't get packet 1 certNego\n";
		dlen = 1024*4;	
		if ( ! doRPCoreOp(TCSERVICERPHOSTQUOTERESPONSEFROMAPP, &utype, size, (byte*)rgBuf, &dlen, (byte*)buf) )
                        throw "from RPCore service";
	 #endif
    }
    catch(const char* szError) {
        fRet= false;
        fprintf(stdout, "Error: %s\n", szError);
    }

	fclose(fp);
	deinit_pyifc();	
    return fRet;
}
