//
//  File: rpinterface.cpp
//  Description: tcIO implementation
//
//  Copyright (c) 2012, Intel Corporation. 
//
// Use, duplication and disclosure of this file and derived works of
// this file are subject to and licensed under the Apache License dated
// January, 2004, (the "License").  This License is contained in the
// top level directory originally provided with the CloudProxy Project.
// Your right to use or distribute this file, or derived works thereof,
// is subject to your being bound by those terms and your use indicates
// consent to those terms.
//
// If you distribute this file (or portions derived therefrom), you must
// include License in or with the file and, in the event you do not include
// the entire License in the file, the file must contain a reference
// to the location of the License.

// -------------------------------------------------------------------


#include "jlmTypes.h"
#include "logging.h"
#include "tcIO.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#ifdef LINUX
#include <wait.h>
#else
#include <sys/wait.h>
#endif

#include <pthread.h>
#include <semaphore.h>
#include "rp_api_code.h"
#include "pyifc.h"

#include "tcconfig.h"

#include "tcpchan.h"

/*************************************************************************************************************/
tcChannel   g_reqChannel;
//#if 1

extern char g_rpcore_ip [64];
int g_ifc_status = 0; 

#define IFC_UNKNOWN		0
#define IFC_ERR			-1
#define IFC_UP			1

void* dom_listener_main ( void* p) ;
pthread_t dom_listener_thread; 
pthread_attr_t  g_attr;

//code pulled from device driver

pthread_mutex_t gm;
pthread_cond_t 	gc;

//variable for request id
pthread_mutex_t req_id_mutex;
static int req_id;


#ifndef byte
typedef unsigned char byte;
#endif


#define TYPE(minor)     (((minor) >> 4) & 0xf)
#define NUM(minor)      ((minor) & 0xf)


#define JIFTIMEOUT 100

// -------------------------------------------- ----------------------------------
/*************************************************************************************************************/
// -------------------------------------------------------------------


void tcBufferprint(tcBuffer* p)
{
    fprintf(g_logFile, "Buffer:  req: %ld, size: %ld, status: %ld\n",
           (long int)p->m_reqID, (long int)p->m_reqSize,
           (long int)p->m_ustatus);
}


// -------------------------------------------------------------------


bool g_fterminateLoop= false;

#ifndef TCIODEVICEDRIVERPRESENT
bool openserver(int* pfd, const char* szunixPath, struct sockaddr* psrv)
{
    int                 fd;
    int                 slen= 0;
    int                 iQsize= 5;
    int                 iError= 0;
    int                 l;

//    fprintf(g_logFile, "open server FILE: %s\n", szunixPath);
    unlink(szunixPath);
    if((fd=socket(AF_UNIX, SOCK_STREAM, 0))==(-1))
        return false;

    slen= strlen(szunixPath)+sizeof(psrv->sa_family)+1;
    memset((void*) psrv, 0, slen);
    psrv->sa_family= AF_UNIX;
    strcpy(psrv->sa_data, szunixPath);

    iError= bind(fd, psrv, slen);
    if(iError<0) {
        fprintf(g_logFile, "openserver:bind error %s\n", strerror(errno));
        return false;
    }
    if(listen(fd, iQsize)==(-1)) {
        fprintf(g_logFile, "listen error in server init");
        return false;
    }

    *pfd= fd;
    return true;
}


bool openclient(int* pfd, const char* szunixPath, struct sockaddr* psrv)
{
    int     fd;
    int     newfd;
    int     slen= strlen(szunixPath)+sizeof(psrv->sa_family)+1;
    int     iError= 0;

//    fprintf(g_logFile, "open client FILE: %s\n", szunixPath);
    if((fd=socket(AF_UNIX, SOCK_STREAM, 0))==(-1))
        return false;

    memset((void*) psrv, 0, slen);
    psrv->sa_family= AF_UNIX;
    strcpy(psrv->sa_data, szunixPath);

    iError= connect(fd, psrv, slen);
    if(iError<0) {
        fprintf(g_logFile, "openclient: Cant connect client, %s\n", strerror(errno));
        close(fd);
        return false;
    }

    *pfd= fd;
    return true;
}
#endif


bool tcChannel::OpenBuf(u32 type, int fd, const char* file, u32 flags)
{
	bool status = false;
	
    m_uType= type;
    m_fd= -1;
#ifdef TCSERVICE
//v:
	pthread_mutex_init(&gm, NULL);
	pthread_cond_init(&gc, NULL);
	//sem_init(&sem_req, 0, 1);
	//sem_init(&sem_resp, 0, 1);
	
	pthread_attr_init(&g_attr);
	pthread_create(&dom_listener_thread, &g_attr, dom_listener_main, (void*)NULL);
	pthread_join(dom_listener_thread,NULL);
//v:
#endif

	m_fd = 0;
	
	while (g_ifc_status  == IFC_UNKNOWN ) {
		sleep(1);
	}
	
	if (g_ifc_status == IFC_UP )
		status = true;
		
	return status;
}

// ------------------------------------------------------------------------------
#define PRINTF(buf, len) for(int ix=0; ix < len; ix++) printf("%02x", (buf)[ix]); printf("\n");


//
// this is temporary to be replaced by TLS-PSK
//
typedef struct _session {
	int fd;
	struct in_addr	addr;
	int dom_id; //this is not the real dom_id
	//byte ekey[32];
	//byte ikey[32];
} tcSession;

int g_mx_sess = 32;
int g_sessId = 1;
tcSession ctx[32];
sem_t   g_sem_sess;

int generate_req_id() {
	int t_req_id;
	pthread_mutex_lock(&req_id_mutex);
	req_id = (req_id + 1)%INT32_MAX;
	t_req_id = req_id;
	pthread_mutex_unlock(&req_id_mutex);
	return t_req_id;
}

int process_request(int fd, int req_id, char* buf, int data_size) {
	char *outparams = NULL;
	int outparams_size = -1;
	int tcBuffer_size = sizeof(tcBuffer);
	u32 uReq;
	int payload_size = 0;
	if(data_size < tcBuffer_size) {
		fprintf(g_logFile,"In start_request_processing() : data received is not valid\n");
		return -1;
	}
	tcBuffer* recv_tc_buff = (tcBuffer *)buf;
	uReq = recv_tc_buff->m_reqID;
	payload_size = recv_tc_buff->m_reqSize;
	if( payload_size > PARAMSIZE ) {
		fprintf(g_logFile, "size of payload recieved is more than buffer size\n");
		return -1;
	}
	outparams = (char *) calloc(1,sizeof(byte)*PARAMSIZE);
	if(!serviceRequest(req_id, uReq, payload_size, (byte *)buf + tcBuffer_size, &outparams_size, (byte *)outparams)) {
		fprintf(g_logFile,"Error in serving the request \n");
		//return -1;
	}
	if( outparams_size > PADDEDREQ ) {
		fprintf(g_logFile,"Can't send the response, data is more than available buffer\n");
		return -1;
	}
	memset(buf,0,PADDEDREQ);
	tcBuffer* send_tc_buff = (tcBuffer *) buf;
	send_tc_buff->m_reqID = uReq;
	send_tc_buff->m_reqSize = outparams_size;
	if(outparams == NULL && outparams_size == 0 ) {
		send_tc_buff->m_ustatus = -1;
	}
	else {
		send_tc_buff->m_ustatus = 0;
	}
	int res_buf_size = tcBuffer_size + outparams_size;
	memcpy(&buf[tcBuffer_size], outparams, outparams_size);
	free(outparams);
	int data_send_size = -1;
	data_send_size = ch_write(fd, buf, res_buf_size);
	if ( data_send_size < 0 ) {
		fprintf(g_logFile,"Error in writing response");
		return -1;
	}
	return 0;
}

void* handle_session(void* p) {

	
	char buf[PADDEDREQ] = {0};

	const int sz_buf = sizeof(buf);
	int sz_data = 0;
	int err = -1;
	int domid = -1;
	//int fd1 = ps->fd;
	int fd1 = *(int *)p;
	fprintf(g_logFile, "Entered handle_session() with fd1 as %d\n",fd1);
	fprintf(g_logFile, "handle_session(): Client connection from domid %d\n", domid);
	
	//generate new request Id
	domid = generate_req_id();
	fprintf(g_logFile, "handle_session():inter-domain channel registered for id %d\n", domid);
	
	sz_data = sz_buf;
	err = 0;
	memset(buf, 0, sz_buf);
	fprintf(g_logFile,"handle_session():XXXX dom_listener reading \n");

	//read command from the client
	err = ch_read(fd1, buf, sz_buf);
	if (err < 0){

		fprintf(g_logFile, "handle_session():inter-domain channel read failed ... closing thread\n");
		goto fail;
	}
	fprintf(stdout,"g_quit = %d\n",g_quit);
	fflush(stdout);
	sz_data = err;
	if(process_request(fd1, domid, buf, sz_data) < 0 ) {
		goto fail;
	}

fail:
		
#ifdef TEST
	fprintf(g_logFile,"handle_session():closing fd1 = %d \n",fd1);
#endif
	fprintf(stdout,"closing connection for fd : %d\n",fd1);
	fflush(stdout);
	close(fd1);
	fprintf(g_logFile,"handle_session():exiting\n");		
	return 0;
}

//#define  TEST_SEG

void* dom_listener_main ( void* p)
{
    int                 fd, newfd;
    struct sockaddr_in  server_addr, client_addr;
    int                 slen= sizeof(struct sockaddr_in);
    int                 clen= sizeof(struct sockaddr);
    int                 iError;
    //int 		iQueueSize = 10;
    int 		iQueueSize = 100;
    int 		flag = 0;
    int*		thread_fd;
    pthread_t tid;
    pthread_attr_t  attr;

    fprintf(g_logFile, "\nEntered dom_listener_main()\n");
    pthread_attr_init(&attr);
    sem_init(&g_sem_sess, 0, 1);
	
    fd= socket(AF_INET, SOCK_STREAM, 0);

    if(fd<0) {
        fprintf(g_logFile, "Can't open socket\n");
        g_ifc_status = IFC_ERR;
        return false;
    }

    memset((void*) &server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family= AF_INET;
    server_addr.sin_addr.s_addr= htonl(INADDR_ANY);     // 127.0.0.1
	
	//ip_env = getenv("RPCORE_IPADDR");
    //if (ip_env)
	//	strncpy(g_rpcore_ip, ip_env, 64);

    //inet_aton(g_rpcore_ip, &server_addr.sin_addr);
    server_addr.sin_port= htons(g_rpcore_port);

    iError= bind(fd,(const struct sockaddr *) &server_addr, slen);
    if(iError<0) {
        fprintf(g_logFile, "dom_listener_main():Can't bind socket %s", strerror(errno));
        g_ifc_status = IFC_ERR;
        return false;
    }

    listen(fd, iQueueSize);

    // set the signal disposition of SIGCHLD to not create zombies
    struct sigaction sigAct;
    memset(&sigAct, 0, sizeof(sigAct));
    sigAct.sa_handler = SIG_DFL;
    sigAct.sa_flags = SA_NOCLDWAIT; // don't zombify child processes
   
    int sigRv = sigaction(SIGCHLD, &sigAct, NULL);
    if (sigRv < 0) {
        fprintf(g_logFile, "dom_listener_main():Failed to set signal disposition for SIGCHLD\n");
    } else {
        fprintf(g_logFile, "dom_listener_main():Set SIGCHLD to avoid zombies\n");
    }

	g_ifc_status = IFC_UP;

    while(!g_quit)
    {
        newfd= accept(fd, (struct sockaddr*) &client_addr, (socklen_t*)&clen);
        flag = fcntl(newfd, F_GETFD);
        if (flag >= 0) {
			flag =  fcntl (newfd, F_SETFD, flag|FD_CLOEXEC);
			if (flag < 0) {
				fprintf(g_logFile, "Socket resource may leak to child process..%s", strerror(errno));
			}
		}else {
				fprintf(g_logFile, "Socket resources may leak to child process %s", strerror(errno));
		}

        if(newfd<0) {
            fprintf(g_logFile, "dom_listener_main():Can't accept socket %s", strerror(errno));
            continue;
        }
		
		fprintf(g_logFile, "dom_listener_main():Client connection from %s \n", inet_ntoa(client_addr.sin_addr));
	if (g_quit)                                                       
		continue;
	thread_fd = (int *)malloc(sizeof(int));
	*thread_fd=newfd;
		pthread_create(&tid, &attr, handle_session, (void*)thread_fd);

    }
    close(fd);
    return NULL;
}



bool start_rp_interface(const char* name)
{
    if(!g_reqChannel.OpenBuf(TCDEVICEDRIVER, 0, name ,0)) {
        fprintf(g_logFile, "%s: OpenBuf returned false \n", __FUNCTION__);
        return false;
    }
    return true;
}
