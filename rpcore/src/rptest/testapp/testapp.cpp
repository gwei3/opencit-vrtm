#include "testapp.h"
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
#include "rptrust.h"
*/

#ifndef byte
typedef unsigned char byte;
#endif

static int fd1;

#define PARAMSIZE 8192

#define TCSERVICESTARTAPPFROMAPP   15



int g_fd = 0;
#define RPCORESERVICE_PORT 6005

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


	setenv("PYTHONPATH", "../../scripts", 1);
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

	size= encodeVM2RP_STARTAPP("foo", an, av, PARAMSIZE - size, &rgBuf[size]);

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
	if ( fd1 >= 0) ch_close (fd1);
	deinit_pyifc();	
	return 0;
}

void create_test_data (byte *data, int length) {
    int i;
    for (i = 0; i < length; i ++) data[i] = 'A'+(i%8);
}

int main ( int argc, char** argv) 
{
    static const int DATA_LENGTH=8*1024;
    int data_out_length = DATA_LENGTH*2,
        secret_out_length = DATA_LENGTH;
    int rpid = get_rpcore_decision("/root/kernel","/root/initrd");
	byte secretin[DATA_LENGTH], secretout[DATA_LENGTH], encdata[DATA_LENGTH*2];
    printf ("rpid: %d\n",rpid);
    if (rpid <= 999) {
        printf ("Something went wrong.\n");
        return -1;
    }
    create_test_data(secretin,DATA_LENGTH);
    Seal(DATA_LENGTH,secretin,&data_out_length,encdata);
    UnSeal(data_out_length,encdata,&secret_out_length,secretout);
    printf ("output compared to input: %d\n",memcmp(secretin,secretout,DATA_LENGTH));
    return 0;
}



