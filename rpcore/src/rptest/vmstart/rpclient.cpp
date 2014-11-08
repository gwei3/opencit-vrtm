#include "rpclient.h"
/*
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
*/

#ifndef byte
typedef unsigned char byte;
#endif

static int fd1;

#define PARAMSIZE 8192

#define TCSERVICESTARTAPPFROMAPP   15



int g_fd = 0;
#define RPCORESERVICE_PORT 6005

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
// Todo: security problem with overflow here
int encodeTCSERVICESTARTAPPFROMAPP(const char* file, int nargs, char** args, 
                                   int bufsize, byte* buf)
{
    int n= 0;
    int m= strlen(file)+1;
    int i;
    int k= 0;

#ifdef TEST 
    fprintf(stdout, "encodeTCSERVICESTARTAPPFROMAPP, file %s, nargs: %d\n", file, nargs);

    for(i=0;i<nargs; i++)
        fprintf(stdout, "\targ[%d]: %s\n", i, args[i]);
#endif

    for(i=1;i < nargs;i++)
        k+= sizeof(int) + strlen(args[i]) + 1;

    if(bufsize <( sizeof(int) + m + k )) {
        fprintf(stdout, "encodeTCSERVICESTARTAPPFROMAPP buffer too small\n");
        return -1;
    }

    memcpy(buf, &m, sizeof(int));
    n+= sizeof(int);

    memcpy(&buf[n], (byte*)file, m);
    n+= m;
	
	
    memcpy(&buf[n], (byte*)&nargs, sizeof(int));
    n+= sizeof(int);

    for(i=1;i<nargs;i++) {
        m= strlen(args[i])+1;
        memcpy(&buf[n], &m, sizeof(int));

        n+= sizeof(int);
        memcpy(&buf[n], (byte*)args[i], m);

        n+= m;
    }

    return n;
}


int  decodeTCSERVICESTARTAPPFROMAPP(char** psz, int* pnargs, 
                                     char** args, const byte* buf)
{
    int     size= 0;
    int     n= 0;
    int     i, m, k;

#ifdef CODINGTEST
    fprintf(stdout, "decodeTCSERVICESTARTAPPFROMAPP: starting\n");
#endif 
    memcpy(&size, &buf[n], sizeof(int));
    n+= sizeof(int);
    *psz= strdup((const char*)(&buf[n]));
    if(*psz==NULL) {
        return 0;
    }
    n+= size;
    memcpy(&m, &buf[n], sizeof(int));
    if(m>*pnargs) {
        fprintf(stdout, "decodeTCSERVICESTARTAPPFROMAPP too few args avail\n");
        return 0;
    }
    n+= sizeof(int);
    *pnargs= m;
    for(i=1;i<m;i++) {
        memcpy(&k, &buf[n], sizeof(int));
        n+= sizeof(int);
        args[i]= strdup((const char*)(&buf[n]));
        n+= k;
    }
   
#ifdef CODINGTEST
    fprintf(stdout, "decodeTCSERVICESTARTAPPFROMAPP file: %s, %d args\n", *psz, m);
    for(i=1; i<m;i++)
        fprintf(stdout, "\targ[%d]: %s\n", i, args[i]);
#endif 
    return 1;
}

#endif



int get_rpcore_decision(char* kernel_file, char* ramdisk_file) {
	
	char buf[1024*4] = {0};
	char kernel[1024] = {0};
	char ramdisk[1024] = {0};
	char config[1024] = {0};

	int err = -1;
	int i = 0;
	int retval;
	
	int         size= PARAMSIZE;
	byte        rgBuf[PARAMSIZE];
    tcBuffer*   pReq= (tcBuffer*) rgBuf;
	int         an = 0;
	char*      av[20] = {0};
	
	char  *ip = NULL, *port =NULL;


	setenv("PYTHONPATH", "../scripts", 1);
	if (init_pyifc("rppy_ifc") < 0 )
		return -1;
	

	av[an++] = "./vmtest";
	av[an++] = "-kernel";
	av[an++] = kernel_file;
	av[an++] = "-ramdisk";
	av[an++] = ramdisk_file;
	av[an++] = "-config";
	av[an++] = "config";

 
	fprintf(stdout, "Opening device driver .. \n");

    if (!fd1)
        fd1 = channel_open();

	if(fd1 < 0)
	{
        	fprintf(stdout, "Open error chandu: %s\n", strerror(errno));
        	return -1;
    }

	
	size = sizeof(tcBuffer);

	size= encodeTCSERVICESTARTAPPFROMAPP("foo", an, av, PARAMSIZE -size, &rgBuf[size]);

	pReq->m_procid= 0;
	pReq->m_reqID= TCSERVICESTARTAPPFROMAPP;
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
	retval = *(int*) &rgBuf[sizeof(tcBuffer)];
	return retval;

fail:
	if ( fd1 >= 0)
		ch_close (fd1);

	deinit_pyifc();	
	return 0;
}



int main ( int argc, char** argv) 
{
	return 0;
}

