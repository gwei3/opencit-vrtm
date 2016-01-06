#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#ifdef __linux__
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>

#include "logging.h"
#include "tcpchan.h"
#include "vrtmsockets.h"
#include "win_headers.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "safe_lib.h"
#ifdef __cplusplus
}
#endif

#define g_logFile stdout

int ch_read(int fd, void* buf, int bufsize){
   
   LOG_DEBUG("reading data.....%d \n", bufsize);
   
   tcBuffer*   pReq= (tcBuffer*) buf;
   int sz_header = sizeof(tcBuffer);
   
   ssize_t bytesRead = 0;
   ssize_t data_len = 0;
     do {
   
      //ssize_t ret = read(fd, (char*)buf + bytesRead, sz_header - bytesRead);
		 ssize_t ret = recv(fd, (char*)buf + bytesRead, sz_header - bytesRead, 0);
      if (ret < 0) {
        LOG_ERROR("Could not read a bufferfrom the network after %d bytes were read\n",  bytesRead);
        return ret;
      }
	  
	  if ( ret == 0 )
		break;
	
	  bytesRead += ret;
      
    } while (bytesRead < sz_header);


    LOG_TRACE("From RPCore pReq is reqid = %d, reqsize=%d, status=%d\n",
				pReq->m_reqID, pReq->m_reqSize, pReq->m_ustatus);
				
   if (pReq->m_reqSize == 0) {
		return bytesRead;
	}
	
   data_len  = sz_header + pReq->m_reqSize;
   
   do {
   
      //ssize_t ret = read(fd, (char*)buf + bytesRead, bufsize - bytesRead);
	   ssize_t ret = recv(fd, (char*)buf + bytesRead, bufsize - bytesRead, 0);
      if (ret < 0) {
        LOG_ERROR("Could not read a bufferfrom the network after %d bytes were read\n",  bytesRead);
        return ret;
      }
	  
	  if ( ret == 0 )
		break;
	
	  bytesRead += ret;
//	  fprintf(stdout, "byteRead %d of %d last chunk %d \n", bytesRead, data_len, ret );
      
    } while (bytesRead < data_len);
    
    return bytesRead;

}

int ch_write(int fd, void* buf, int len) {
	ssize_t bytesWritten = 0;
    do {
      //ssize_t ret = write(fd, (char*)buf + bytesWritten, len - bytesWritten);
		ssize_t ret = send(fd, (char*)buf + bytesWritten, len - bytesWritten,0);
      if (ret < 0) {
        LOG_ERROR("Could not write the full buffer of length %d to the network after %d bytes were written\n", len, bytesWritten);
        return ret;
      }

      bytesWritten += ret;
    } while (bytesWritten < len);
    
    return bytesWritten;
}


void ch_register(int fd)
{
    char rpid[64]={0};
    //pid_t rpproxyid;
    //rpproxyid = getpid();
    //sprintf(rpid,"%d",rpproxyid);
    //fprintf(g_logFile,"\nregisteration of rp_id %s", rpid);
    //ch_write(fd, rpid, strlen(rpid) +1 );
}

int ch_open(char* serverip, int port) {
	int  fd= 0;
	int  iError= 0;
    struct sockaddr_in  server_addr;
    int  slen= sizeof(struct sockaddr_in);
	int iResult = 0;
	if ( serverip == NULL ) {
		if (! serverip)
			serverip = "127.0.0.1";
	}
	
	if (port == 0 ) {
		port = 16005;
	}
#ifdef _WIN32
	WSADATA wsData;	
	iResult = initialise_lib(&wsData);
	if (iResult != 0) {
		LOG_ERROR("Error in initialising library : %d", iResult);
		return -1;
	}
#endif
	struct addrinfo hints, *result;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	char server_port[10];
	sprintf(server_port, "%d", port);
	iResult = getaddrinfo(serverip, server_port, &hints, &result);
	if (iResult != 0) {
		LOG_ERROR("getaddrinfo failed!!! : %d", iResult);
		clean_lib();
		return -1;
	}
      // open socket
	fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (fd < 0) {
		LOG_ERROR("Socket creation failed : %d", fd);
		clean_lib();
		return -1;
	}
	/*
	memset_s((void*) &server_addr, sizeof(struct sockaddr_in), 0);

	server_addr.sin_family= AF_INET;
	inet_aton(serverip, &server_addr.sin_addr);
	server_addr.sin_port= htons(port);
	iError= connect(fd, (const struct sockaddr*) &server_addr, (socklen_t) slen);
	*/
	iError = connect(fd, result->ai_addr, (socklen_t)result->ai_addrlen);
	if (iError < 0) {
		close_connection(fd);
		clean_lib();
		fd = -1;
		LOG_ERROR("Can't connect with server, Please check that it's running : %s\n", strerror(errno));        
    }    
    return fd;
}

void ch_close(int fd)
{
	//close(fd);
	close_connection(fd);
}
