
#define __STDC_LIMIT_MACROS
#include "logging.h"

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
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>

#ifdef LINUX
#include <wait.h>
#else
#include <sys/wait.h>
#endif

#include <pthread.h>
#include <semaphore.h>
#include "vrtm_api_code.h"
#include "parser.h"

#include "tcconfig.h"

#include "tcpchan.h"
#include "modtcService.h"
#include "vrtminterface.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "safe_lib.h"
#ifdef __cplusplus
}
#endif

/*************************************************************************************************************/
//tcChannel   g_reqChannel;
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
    LOG_INFO(, "Buffer:  req: %ld, size: %ld, status: %ld\n",
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

    LOG_TRACE("Open Server");
//    fprintf(g_logFile, "open server FILE: %s\n", szunixPath);
    unlink(szunixPath);
    if((fd=socket(AF_UNIX, SOCK_STREAM, 0))==(-1))
        return false;

    slen= strnlen_s(szunixPath, MAX_LEN)+sizeof(psrv->sa_family)+1;
    memset_s((void*) psrv, slen, 0);
    psrv->sa_family= AF_UNIX;
    strcpy_s(psrv->sa_data, sizeof(psrv->sa_data), szunixPath);

    iError= bind(fd, psrv, slen);
    if(iError<0) {
        LOG_ERROR("openserver:bind error %s\n", strerror(errno));
		close(fd);
        return false;
    }
    if(listen(fd, iQsize)==(-1)) {
        LOG_ERROR("listen error in server init");
		close(fd);
        return false;
    }

    *pfd= fd;
    return true;
}

/*
bool openclient(int* pfd, const char* szunixPath, struct sockaddr* psrv)
{
    int     fd;
    int     newfd;
    int     slen= strlen(szunixPath)+sizeof(psrv->sa_family)+1;
    int     iError= 0;

    LOG_TRACE("Open Client");
//    fprintf(g_logFile, "open client FILE: %s\n", szunixPath);
    if((fd=socket(AF_UNIX, SOCK_STREAM, 0))==(-1))
        return false;

    memset_s((void*) psrv, slen, 0);
    psrv->sa_family= AF_UNIX;
    strcpy_s(psrv->sa_data, sizeof(psrv->sa_data), szunixPath);

    iError= connect(fd, psrv, slen);
    if(iError<0) {
        LOG_ERROR( "openclient: Cant connect client, %s\n", strerror(errno));
        close(fd);
        return false;
    }

    *pfd= fd;
    return true;
}*/
#endif

int g_mx_sess = 32;
int g_sessId = 1;
//tcSession ctx[32];
sem_t   g_sem_sess;

int generate_req_id() {
	int t_req_id;
	LOG_TRACE("Generating request ID");
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
	LOG_DEBUG("Size of Data Received from client : %d and data : %s", data_size, buf);
	if(data_size < tcBuffer_size) {
		LOG_ERROR("Data received is not valid.");
		return -1;
	}
	tcBuffer* recv_tc_buff = (tcBuffer *)buf;
	uReq = recv_tc_buff->m_reqID;
	payload_size = recv_tc_buff->m_reqSize;
	LOG_DEBUG("API Request No. : %u and Payload size %d", uReq, payload_size );
	if( payload_size > PARAMSIZE ) {
		LOG_ERROR( "Size of Payload received is more than buffer size");
		return -1;
	}
	outparams = (char *) calloc(1,sizeof(byte)*PARAMSIZE);
	if ( outparams == NULL ) {
		LOG_ERROR("couldn't allocate memory for output buffer");
		return -1;
	}
	if(!serviceRequest(req_id, uReq, payload_size, (byte *)buf + tcBuffer_size, &outparams_size, (byte *)outparams)) {
		LOG_ERROR("Error in serving the request");
	}
	LOG_DEBUG("Output data size after request processing %d", outparams_size);
	if( outparams_size > PADDEDREQ ) {
		LOG_ERROR("Can't send the response, data is more than available buffer");
		return -1;
	}
        LOG_TRACE("Prepare response payload");
	memset_s(buf, PADDEDREQ, 0);
	tcBuffer* send_tc_buff = (tcBuffer *) buf;
	send_tc_buff->m_reqID = uReq;
	send_tc_buff->m_reqSize = outparams_size;
	if( outparams_size == 0 ) {
		send_tc_buff->m_ustatus = -1;
	}
	else {
		send_tc_buff->m_ustatus = 0;
	}
	LOG_DEBUG("Response tcbuffer Attributes API Request No. : %d payload size : %d response status : %d",
			send_tc_buff->m_reqID, send_tc_buff->m_reqSize, send_tc_buff->m_ustatus);
	int res_buf_size = tcBuffer_size + outparams_size;
	memcpy_s(&buf[tcBuffer_size], PARAMSIZE, outparams, PARAMSIZE - tcBuffer_size - 1);
	free(outparams);
	int data_send_size = -1;
        LOG_TRACE("Sending response");
	data_send_size = ch_write(fd, buf, res_buf_size);
	if ( data_send_size < 0 ) {
		LOG_TRACE("Error in writing response to socket");
		return -1;
	}
	LOG_TRACE("Response sent");
	return 0;
}

void* handle_session(void* p) {

	
	char buf[PADDEDREQ] = {0};

	const int sz_buf = sizeof(buf);
	int sz_data = 0;
	int err = -1;
	int domid = -1;
	int fd1 = -1;
	if ( p!= NULL) {
		fd1 = *(int *)p;
		free(p);
	}
	LOG_TRACE("Entered handle_session() with fd as %d",fd1);
	//fprintf(g_logFile, "handle_session(): Client connection from domid %d\n", domid);
	
	//generate new request Id
	domid = generate_req_id();
	LOG_DEBUG("Request id for request is : %d", domid);
	
	sz_data = sz_buf;
	err = 0;
	memset_s(buf, sz_buf, 0);
	LOG_TRACE("Reading data from socket");

	//read command from the client
	err = ch_read(fd1, buf, sz_buf);
	LOG_TRACE("Done reading from socket");
	if (err < 0){
		LOG_ERROR("Error in reading data from socket. Closing thread");
		goto fail;
	}
	sz_data = err;
    LOG_TRACE("Process request"); 
	if(process_request(fd1, domid, buf, sz_data) < 0 ) {
		LOG_ERROR("Error in processing the request. Closing thread.");
		goto fail;
	}

fail:
		
#ifdef TEST
	LOG_TRACE("Closing fd = %d",fd1);
#endif
	LOG_TRACE("Closing connection for fd : %d",fd1);
	close(fd1);
	LOG_TRACE("Exiting thread");
	return 0;
}

//#define  TEST_SEG

void* dom_listener_main ( void* p)
{
    int                 fd, newfd;
    struct addrinfo     hints, *vrtm_addr;
    struct sockaddr_in  client_addr;
    int                 slen= sizeof(struct sockaddr_in);
    int                 clen= sizeof(struct sockaddr);
    int                 iError;
    //int 		iQueueSize = 10;
    int 		iQueueSize = 100;
    int 		flag = 0;
    int*		thread_fd;
    pthread_t tid;
    pthread_attr_t  attr;
    char vrtm_port[6] = {'\0'};

    LOG_TRACE("Entered dom_listener_main()");
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    //sem_init(&g_sem_sess, 0, 1);

    memset_s(&hints, sizeof hints, 0);
    hints.ai_family = AF_INET; // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    snprintf(vrtm_port, sizeof(vrtm_port), "%d", g_rpcore_port);
	getaddrinfo(g_rpcore_ip, vrtm_port, &hints, &vrtm_addr);
	LOG_DEBUG("Socket type : %d, Socket address : %s, Protocol : %d ", vrtm_addr->ai_family, vrtm_addr->ai_addr->sa_data, vrtm_addr->ai_protocol);

    LOG_TRACE("Create socket for vRTM core");	
    //fd= socket(AF_INET, SOCK_STREAM, 0);
    fd = socket(vrtm_addr->ai_family, vrtm_addr->ai_socktype, 0);
    if(fd<0) {
        LOG_ERROR("Can't open socket");
        g_ifc_status = IFC_ERR;
        freeaddrinfo(vrtm_addr);
        pthread_attr_destroy(&attr);
        return false;
    }
    LOG_TRACE("Bind vRTM core socket");
    /*memset_s((void*) &server_addr, sizeof(struct sockaddr_in), 0);
    server_addr.sin_family= AF_INET;
    server_addr.sin_addr.s_addr= htonl(INADDR_ANY);     // 127.0.0.1*/
	//ip_env = getenv("RPCORE_IPADDR");
    //if (ip_env)
	//	strncpy(g_rpcore_ip, ip_env, 64);

    //inet_aton(g_rpcore_ip, &server_addr.sin_addr);
    //server_addr.sin_port= htons(g_rpcore_port);

    //iError= bind(fd,(const struct sockaddr *) &server_addr, slen);
	iError= bind(fd,vrtm_addr->ai_addr, vrtm_addr->ai_addrlen);
    if(iError<0) {
        LOG_ERROR("Can't bind socket %s", strerror(errno));
        g_ifc_status = IFC_ERR;
		close(fd);
		freeaddrinfo(vrtm_addr);
		pthread_attr_destroy(&attr);
        return false;
    }

    listen(fd, iQueueSize);

    // set the signal disposition of SIGCHLD to not create zombies
    /*struct sigaction sigAct;
    memset_s(&sigAct, sizeof(sigAct), 0);
    sigAct.sa_handler = SIG_DFL;
    sigAct.sa_flags = SA_NOCLDWAIT; // don't zombify child processes
   
    int sigRv = sigaction(SIGCHLD, &sigAct, NULL);
    if (sigRv < 0) {
        LOG_INFO( "Failed to set signal disposition for SIGCHLD");
    } else {
        LOG_INFO( "Set SIGCHLD to avoid zombies");
    }*/

	g_ifc_status = IFC_UP;
    
    LOG_INFO("Socket ready to accept requests");
    while(!g_quit)
    {
        newfd= accept(fd, (struct sockaddr*) &client_addr, (socklen_t*)&clen);
        flag = fcntl(newfd, F_GETFD);
        if (flag >= 0) {
			flag =  fcntl (newfd, F_SETFD, flag|FD_CLOEXEC);
			if (flag < 0) {
				LOG_WARN( "Socket resource may leak to child process..%s", strerror(errno));
			}
		}else {
				LOG_WARN( "Socket resources may leak to child process %s", strerror(errno));
		}

        if(newfd<0) {
            LOG_WARN( "Can't accept socket %s", strerror(errno));
            continue;
        }
		
		LOG_INFO( "Client connection from %s ", inet_ntoa(client_addr.sin_addr));
		if (g_quit) {
			close(newfd);
			continue;
		}
		thread_fd = (int *)malloc(sizeof(int));
		if(thread_fd != NULL) {
			*thread_fd=newfd;
        	LOG_DEBUG("Creating separate thread to handle session");
			pthread_create(&tid, &attr, handle_session, (void*)thread_fd);
		}
		else {
			LOG_ERROR("Couldn't allocate memory for fd of this request");
		}
    }
    close(fd);
    freeaddrinfo(vrtm_addr);
    pthread_attr_destroy(&attr);
    return NULL;
}

bool start_vrtm_interface(const char* name)
{
    LOG_DEBUG("Starting vRTM on socket");
    bool status = false;
        pthread_mutex_init(&gm, NULL);
        pthread_cond_init(&gc, NULL);
    LOG_TRACE("Starting separate thread to start vRTM socket");
        pthread_attr_init(&g_attr);
        pthread_create(&dom_listener_thread, &g_attr, dom_listener_main, (void*)NULL);
        pthread_join(dom_listener_thread,NULL);

        while (g_ifc_status  == IFC_UNKNOWN ) {
                sleep(1);
        }

        if (g_ifc_status == IFC_UP )
                status = true;

    LOG_DEBUG("Socket for vRTM is started");
        return status;
}
