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

#include "logging.h"
#include "tcpchan.h"

#define g_logFile stdout

int ch_read(int fd, void* buf, int bufsize){
   
   LOG_DEBUG("reading data.....%d \n", bufsize);
   
   tcBuffer*   pReq= (tcBuffer*) buf;
   int sz_header = sizeof(tcBuffer);
   
   ssize_t bytesRead = 0;
   ssize_t data_len = 0;
     do {
   
      ssize_t ret = read(fd, (char*)buf + bytesRead, sz_header - bytesRead);
      if (ret < 0) {
        LOG_ERROR("Could not read a bufferfrom the network after %d bytes were read\n",  bytesRead);
        return ret;
      }
	  
	  if ( ret == 0 )
		break;
	
	  bytesRead += ret;
      
    } while (bytesRead < sz_header);


#ifdef TEST
    LOG_INFO("From RPCore pReq is reqid = %d, reqsize=%d, status=%d\n",
				pReq->m_reqID, pReq->m_reqSize, pReq->m_ustatus);
#endif
				
   if (pReq->m_reqSize == 0) {
		return bytesRead;
	}
	
   data_len  = sz_header + pReq->m_reqSize;
   
   do {
   
      ssize_t ret = read(fd, (char*)buf + bytesRead, bufsize - bytesRead);
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
      ssize_t ret = write(fd, (char*)buf + bytesWritten, len - bytesWritten);
      if (ret < 0) {
        LOG_ERROR("Could not write the full buffer of length %d to the network after %d bytes were written\n", len, bytesWritten);
        return ret;
      }

      bytesWritten += ret;
    } while (bytesWritten < len);
    
    return bytesWritten;
}


int ch_register(int fd)
{
    char rpid[64]={0};
    pid_t rpproxyid;
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
    
	if ( serverip == NULL ) {
		serverip = getenv("RPCORE_IPADDR");
		if (! serverip)
			serverip = "127.0.0.1";
	}
	
	if (port == 0 ) {
		char* sport = getenv("RPCORE_PORT");
		if (! sport)
			port = 16005;
		else	
			port = atoi(sport);
	}
	
      // open socket
	fd= socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0) 
		return -1;
	
	memset((void*) &server_addr, 0, sizeof(struct sockaddr_in));

	server_addr.sin_family= AF_INET;
	inet_aton(serverip, &server_addr.sin_addr);
	server_addr.sin_port= htons(port);


	iError= connect(fd, (const struct sockaddr*) &server_addr, (socklen_t) slen);
	if (iError < 0 )
		fd = -1;

	if(fd < 0)
	{
        	LOG_ERROR("Can't connect with server, Please check that it's running : %s\n", strerror(errno));
        	return -1;
    } 
    
    return fd;

}

void ch_close(int fd)
{
	close(fd);
}
