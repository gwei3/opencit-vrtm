#include "testquote.h"

#include <stdio.h>
#include <stdlib.h>
/*
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

//global variable for channel, used by the rptrust lib
int g_rpchan_fd = -1;

#define PARAMSIZE 8192

#define TCSERVICESTARTAPPFROMAPP   15

#define RPCORESERVICE_PORT 6005

int channel_open() {
    int fd = -1;

	fd =  ch_open(NULL, 0);


	if(fd < 0)
	{
        	fprintf(stdout, "Cannot connect: %s\n", strerror(errno));
        	return -1;
    }
    
  	ch_register(fd);  
    return fd;

}

int get_rpcore_decision(char* kernel_file, char* ramdisk_file, char* manifest_path, char* disk_path) {
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
    av[an++] = "-disk";
    av[an++] = disk_path;
    av[an++] = "-manifest";
    av[an++] = manifest_path;
	av[an++] = "-config";
	av[an++] = "config";
    
 
	fprintf(stdout, "Opening channel .. \n");

    if (g_rpchan_fd <= 0)
        g_rpchan_fd = channel_open();

	if(g_rpchan_fd < 0)
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
	
	err = ch_write(g_rpchan_fd, rgBuf, size + sizeof(tcBuffer) );
	if (err < 0){
		fprintf(stdout, "write error: %s\n", strerror(errno));
		goto fail;
	}

	memset(rgBuf, 0, sizeof(rgBuf));
	fprintf(stdout, "Sent request ..........\n");
	
again:
	err = ch_read(g_rpchan_fd, rgBuf, sizeof(rgBuf));
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
	if ( g_rpchan_fd >= 0) {
		ch_close (g_rpchan_fd);
		g_rpchan_fd = 0;
	}
	
	deinit_pyifc();	
	return 0;
}



void create_test_data (byte *data, int length) {
    int i;
    for (i = 0; i < length; i ++) data[i] = 'A'+(i%8);
}

static void bin2ascii(int iSize, const byte* pbData, char* szMsg)
{
    int i;
    for (i= 0; i<iSize; i++)	
			sprintf(&szMsg[2*i], "%02x", pbData[i]);
    
    szMsg[2*iSize] = '\0';
}

int main ( int argc, char** argv) 
{
	char s_rpid[64] = {0};
	
    static const int DATA_LENGTH = 8*1024;
    
    int secret_in_length = 0;
    
    int data_out_length = DATA_LENGTH*2;
        
    int secret_out_length = DATA_LENGTH;
    
    int attest_out_length = DATA_LENGTH;
    
    FILE *fptr;

    fptr = fopen ("tmp_kernel_ramdisk", "w");
    
    fprintf(fptr, "This is tmp file");

    fclose(fptr);

    char *man = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?><Manifest version=\"1.1\"><Headers><Customer_ID>fdsfs</Customer_ID><Image_ID>84e91753-a282-47f2-948a-53d2befa8048</Image_ID><Launch_Policy>Audit</Launch_Policy><Hash_Type>SHA-256</Hash_Type><Hidden_Files>true</Hidden_Files><Image_Hash>2f99ee195471b189d9a0864d1156f60f282229e157df7d52c448bf2931cce54f</Image_Hash></Headers><File_Hashes><Kernel_Hash>5bc182952a30cb59c0438bb0a48004cb23c54af2dbdbc920dc6d04ed1a5f4f56</Kernel_Hash><Initrd_Hash>5bc182952a30cb59c0438bb0a48004cb23c54af2dbdbc920dc6d04ed1a5f4f56</Initrd_Hash></File_Hashes></Manifest>";

    fptr = fopen ("tmp_kernel_ramdisk_manifest", "w");

    fprintf(fptr, "%s", man);

    fclose(fptr);
 
    char tmp_kernel_ramdisk_realpath[512];
    char tmp_kernel_ramdisk_manifest_realpath[512];
    realpath("tmp_kernel_ramdisk", tmp_kernel_ramdisk_realpath);
    realpath("tmp_kernel_ramdisk_manifest", tmp_kernel_ramdisk_manifest_realpath);

    printf("%s %s", tmp_kernel_ramdisk_realpath, tmp_kernel_ramdisk_manifest_realpath);
    int rpid = get_rpcore_decision(tmp_kernel_ramdisk_realpath, tmp_kernel_ramdisk_realpath, tmp_kernel_ramdisk_manifest_realpath, tmp_kernel_ramdisk_realpath);

	byte secretin[DATA_LENGTH]={0}, secretout[DATA_LENGTH]={0}, encdata[DATA_LENGTH*2]={0}, attestdata[DATA_LENGTH*2]={0};
	byte dataout[DATA_LENGTH]={0};

	char sz_buf[DATA_LENGTH] = {0};
	
	unsigned txx = 0;
	
	char* xmlquote;
	
	strcpy((char*)secretin, "the master secret");
	secret_in_length = strlen((char*)secretin);
	
    sprintf (s_rpid, "%d",rpid);
    printf("rpid is %s\n", s_rpid );
    if (rpid <= 999) {
        printf ("Something went wrong.\n");
        return -1;
    }
    
    if ( g_rpchan_fd >= 0) {
		ch_close (g_rpchan_fd);
		g_rpchan_fd = 0;
	}
	
	setenv("THIS_RPID", s_rpid, 1);
	
	g_rpchan_fd = channel_open();
	if ( g_rpchan_fd < 0)
		return -1;
		
   GenHostedComponentQuote(strlen(s_rpid), (byte*) s_rpid, "veryverysecretkey", &xmlquote);
   printf("XML : \n %s", xmlquote);
   if (VerifyHostedComponentQuote(xmlquote, NULL))
		printf("Quote verification successful!!!\n");
		
    return 0;
}



