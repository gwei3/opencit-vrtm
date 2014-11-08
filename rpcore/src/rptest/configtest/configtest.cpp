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
#include "channelcoding.h"
#include "pyifc.h"

#ifndef byte
typedef unsigned char byte;
#endif

#define PARAMSIZE 1024*16

#define TCSERVICESETFILEFROMAPP   21



int g_fd = 0;
#define RPCORESERVICE_PORT 16005


int channel_open() {
    int fd = -1;
    
#ifndef USE_DRV_CHANNEL //not defined
	fd =  ch_open(NULL, 0);
#else
	fd = open("/dev/chandu", O_RDWR);

#endif
	if(fd < 0)
	{
        	fprintf(stdout, "Open error chandu: %s\n", strerror(errno));
        	return -1;
    }
    
    ch_register(fd);
    
    return fd;
}

#if 0

int encodeTCSERVICESETFILEFROMAPP(int size, const byte* data, int bufsize, 
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


bool  decodeTCSERVICESETFILEFROMTCSERVICE(int* ppdatasize, byte* pdata, const byte* buf)
{
    int n= 0;

    memcpy(ppdatasize, &buf[n], sizeof(int));
    n+= sizeof(int);
    memcpy(pdata, &buf[n], *ppdatasize);
    return true;
}
#endif

int main ( int argc, char** argv) 
{
	
	char buf[1024*4] = {0};
	char filebuf[PARAMSIZE] = {0};
	int fsz = PARAMSIZE;
	
	char ramdisk_name[1024] = {0};

	int err = -1;
	int i = 0;

	int fd1;
	
	int         size= PARAMSIZE;
	byte        rgBuf[PARAMSIZE];
    tcBuffer*   pReq= (tcBuffer*) rgBuf;

	
	char *fname = "";
	char *server = "192.168.241.2";
	char *port = "15999";

	for ( i = 0; i < argc; i++) {
		if( strcmp(argv[i], "-file") == 0 ){
			fname = argv[ ++i ];
		}
		
		if( strcmp(argv[i], "-host") == 0 ){
			server = argv[ ++i ];
		}
		if( strcmp(argv[i], "-port") == 0 ){
			port = argv[ ++i ];
		}
	}
	
	setenv("PYTHONPATH", "../scripts", 1);
	if (init_pyifc("rppy_ifc") < 0 )
	{
		printf("init_pyifc returned error\n");
		return -1;
	}
	
	//commandline for tftp server port file 
	fsz = sprintf(filebuf, "%s  -P %s -g %s", server, port, fname);

	fd1 = channel_open();
	if(fd1 < 0)
	{
        	fprintf(stdout, "Open error chandu: %s\n", strerror(errno));
        	return -1;
    }

	
	size = sizeof(tcBuffer);

	size= encodeVM2RP_SETFILE(fsz, (byte*) filebuf, PARAMSIZE -size, &rgBuf[size]);

	pReq->m_procid= 0;
	pReq->m_reqID= TCSERVICESETFILEFROMAPP;
	pReq->m_ustatus= 0;
	pReq->m_origprocid= 0;
	pReq->m_reqSize= size;

	fprintf(stdout, "Sending request size %d \n", size + sizeof(tcBuffer));
	
	err = ch_write(fd1, rgBuf, size + sizeof(tcBuffer) );
	if (err < 0){
		fprintf(stdout, "write error: %s\n", strerror(errno));
		goto fail;
	}

	memset(rgBuf, 0, sizeof(rgBuf));
	fprintf(stdout, "Sent request ..........\n");
	
again:	
	err = ch_read(fd1, rgBuf, sizeof(rgBuf));
	if (err < 0){
		if (errno == 11) {
			sleep(1);
			goto again;
		}

		fprintf(stdout, "read error:%d  %s\n", errno, strerror(errno));
		goto fail;
	}

	fprintf(stdout, "Response  from server status %d return %d\n", pReq->m_ustatus, *(int*) &rgBuf[sizeof(tcBuffer)]);

fail:
	if ( fd1 >= 0)
		ch_close (fd1);

	deinit_pyifc();
	return 0;
}

