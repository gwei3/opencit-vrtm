#include "vmidtest.h"


#ifndef byte
typedef unsigned char byte;
#endif

static int fd1;

#define PARAMSIZE 8192




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

	size= encodeVM2RP_STARTAPP ("foo", an, av, PARAMSIZE -size, &rgBuf[size]);

	pReq->m_procid= 0;
	pReq->m_reqID=  VM2RP_STARTAPP ;
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

	return 0;
}

 int map_rpid_uuid(int rpid, char* uuid) {
	
	char buf[32] = {0};

	int err = -1;
	int i = 0;
	int retval;
	
	int         size= PARAMSIZE;
	byte        rgBuf[PARAMSIZE];
    tcBuffer*   pReq= (tcBuffer*) rgBuf;
	int         an = 0;
	char*      av[20] = {0};
	
	char  *ip = NULL, *port =NULL;

	//generate test uuid string
	int pd[2];
	pipe(pd);
	dup2(pd[1],1);
	system("/usr/bin/uuidgen");
	read(pd[0], uuid, 128);

	while(uuid[i] != '\n') 
		i++;
	uuid[i] = '\0';
	
	sprintf(buf, "%d", rpid);
	
 
	fprintf(stdout, "Opening channel .. \n");

    if (!fd1)
        fd1 = channel_open();

	if(fd1 < 0)
	{
        	fprintf(stdout, "Open error channel: %s\n", strerror(errno));
        	return -1;
    }

	
	size = sizeof(tcBuffer);

	size= encodeVM2RP_SETUUID (buf, uuid, PARAMSIZE -size, (byte*)&rgBuf[size]);

	pReq->m_procid= 0;
	pReq->m_reqID=  VM2RP_SETUUID ;
	pReq->m_ustatus= 0;
	pReq->m_origprocid= 0;
	pReq->m_reqSize= size;

	fprintf(stdout, "Sending request size %d \n %s\n", size + sizeof(tcBuffer), &rgBuf[20]);
	
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

	printf( "Response  from server status %d return %d\n", pReq->m_ustatus, *(int*) &rgBuf[sizeof(tcBuffer)]);
	retval = *(int*) &rgBuf[sizeof(tcBuffer)];
	return retval;

fail:
	if ( fd1 >= 0)
		ch_close (fd1);

	return 0;
}

int delete_uuid(char* uuid) {
	
	char buf[32] = {0};

	int err = -1;
	int i = 0;
	int retval;
	
	int         size= PARAMSIZE;
	byte        rgBuf[PARAMSIZE];
    tcBuffer*   pReq= (tcBuffer*) rgBuf;
	int         an = 0;
	char*      av[20] = {0};
	
	char  *ip = NULL, *port =NULL;


 
	fprintf(stdout, "Opening channel .. \n");

    if (!fd1)
        fd1 = channel_open();

	if(fd1 < 0)
	{
        	fprintf(stdout, "Open error channel: %s\n", strerror(errno));
        	return -1;
    }

	
	size = sizeof(tcBuffer);

	size= encodeVM2RP_TERMINATEAPP (strlen(uuid), (byte*)uuid, PARAMSIZE -size, (byte*)&rgBuf[size]);

	pReq->m_procid= 0;
	pReq->m_reqID=  VM2RP_TERMINATEAPP ;
	pReq->m_ustatus= 0;
	pReq->m_origprocid= 0;
	pReq->m_reqSize= size;

	fprintf(stdout, "Sending request size %d \n %s\n", size + sizeof(tcBuffer), &rgBuf[20]);
	
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

	printf( "Response  from server status %d return %d\n", pReq->m_ustatus, *(int*) &rgBuf[sizeof(tcBuffer)]);
	retval = *(int*) &rgBuf[sizeof(tcBuffer)];
	return retval;

fail:
	if ( fd1 >= 0)
		ch_close (fd1);

	return 0;
}


int main ( int argc, char** argv) 
{
	int rpid = 0;
	char buffer[128] = {0};
	
	setenv("PYTHONPATH", "../scripts", 1);
	if (init_pyifc("rppy_ifc") < 0 )
		return -1;

	if(rpid = get_rpcore_decision("/boot/vmlinuz-3.7.1-dU", "/boot/initrd.img-3.7.1-dU") )
		printf ("Success: get_rpcore_decision is LAUNCH %d \n", rpid);
	else
		printf ("Failed: get_rpcore_decision DO NOT LAUNCH\n");

	map_rpid_uuid(rpid, buffer);

	delete_uuid(buffer);
	deinit_pyifc();	
	return 0;

	return 0;
}

