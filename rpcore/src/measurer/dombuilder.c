/*
	Author: Vinay Phegade
*/

/*
	5. Launch new VM

	
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <zlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <tcconfig.h>

//declared again from tcIO.h
#define PADDEDREQ 64*1024
#define PARAMSIZE (PADDEDREQ-20)

#define true 	1
#define false 	0
typedef uint8_t byte;
typedef int bool;

int 	g_mtwproxy_port = 16006;
int  	g_mtwproxy_on = 0;

char 	g_mtwproxy_ip[64] = "127.0.0.1";

//this is fake domain id
uint32_t	g_rpdomid = 1000;

char    *g_tftp_script = "../../scripts/copyfile.sh";
char    *g_tftp_uncompress = "../../scripts/copy_uncompress.sh";
char    *g_vm_script = "../../scripts/startvm.sh";



const char* g_mtw_req = "<CheckPolicyReq> <domain_type>%s</domain_type><Hash> %s</Hash>"\
	"<Attestation> <RPSignature> %s</RPSignature> <Cert> %s</Cert></Attestation></CheckPolicyReq>\n";

const char* g_mtw_resp = "<CheckPolicyResp>%s</CheckPolicyResp>";

int g_mtwprxy_fd = -1;

int connect_to_mtwproxy ( ) {
	
	int  fd= g_mtwprxy_fd;
	int  ret = 0;
	struct sockaddr_in  server_addr;
	int  slen= sizeof(struct sockaddr_in);
		
	//if already connected then skip connect to mtw proxy
	if (fd < 0) {

		  // open socket
		fd= socket(AF_INET, SOCK_STREAM, 0);
		if(fd < 0) 
			return -1;
		
		memset((void*) &server_addr, 0, sizeof(struct sockaddr_in));

		server_addr.sin_family= AF_INET;
		inet_aton(g_mtwproxy_ip, &server_addr.sin_addr);
		server_addr.sin_port= htons(g_mtwproxy_port);
		
		int retries = 5;
		do {
			
			ret = connect(fd, (const struct sockaddr*) &server_addr, (socklen_t) slen);
			
			if (ret >= 0 )
				break;
			
			fprintf(stdout, "Retrying connection to MtW proxy\n");
				
		}while (retries-- > 0 );
				
		
		if(ret < 0){
			fprintf(stdout, "Connection to MtW proxy failed\n");
			return -1;
		}
	
		g_mtwprxy_fd = fd;
	}
	
	return fd;
}


bool is_hash_whitelisted(uint32_t parent, int hashtype, int rgHashsize, byte* rgHash){
	int ret = -1;
	
	byte sigAttest[256];
	int  sizeAttest = 256;
	
	
	int  size = PARAMSIZE;
    char rgBuf[PARAMSIZE];
    uint32_t  uType= 0;

	char b64_hash[64];
	char b64_sign[512];
    
    int  b64_hash_size = 64;
    int  b64_sign_size = 512;
    
	char * sz_cert = NULL;
    char * sz_type = "domApp";
    
    int fd  = 0 ;
    
 
	do {

		fd  = connect_to_mtwproxy ( );
		
		if ( fd < 0 )
			return false;

#if 0 //need to be enabled if we want whitelist verification from MtWilson

		if ( parent == PLATFORMTYPELINUX ) {
			
			if ( !g_myService.m_trustedHome.Attest(rgHashsize, rgHash, rgHashsize, rgHash, &sizeAttest, sigAttest))
				return false;
			
			
			if(! g_myService.m_trustedHome.m_myCertificate){
				fprintf(g_logFile, "No serialized certificate or public key\n");
				return false;
			}
			
			sz_cert = g_myService.m_trustedHome.m_myCertificate;
		}
		else if (parent  == PLATFORMTYPEHW ) {
			
			if (! g_myService.m_host.Attest (rgHashsize, rgHash, &sizeAttest, sigAttest))
				return false;
				
			sz_cert = (char*) g_myService.m_host.m_hostCertificate;
			sz_type = "dom0";
		}
		
		//create XML
		if(!toBase64(rgHashsize, rgHash, &b64_hash_size, b64_hash)){
			fprintf(g_logFile, "Hash base64 encode failed\n");
			return false;
		}

		if(!toBase64(sizeAttest, sigAttest, &b64_sign_size, b64_sign)){
			fprintf(g_logFile, "Signature base64 encode failed\n");
			return false;
		}
		
		size = sprintf(rgBuf, g_mtw_req, sz_type, b64_hash, b64_sign, sz_cert );
							
		//send attestation and host certificate to MTW agent
		
		fprintf(g_logFile, rgBuf);

#endif
		//write the request
		ret =  write ( fd, rgBuf, strlen(rgBuf));
		if (ret < 0 ) {	
			return false;
		}
		
//		fprintf(g_logFile, "Send request...\n");
		memset (rgBuf, 0, sizeof(rgBuf));
		
		//read the response
		FILE* fd_rd = fdopen(fd, "r");
		
		char * p = fgets( (char*)rgBuf, sizeof(rgBuf), fd_rd);
		if ( ! p ) {
			return false;
		}
		
		//check response here
		fprintf(stdout, "Received data is %s", rgBuf);
		if ( strstr(rgBuf, "approve") )
			return true;
		else
			return false;
	} while (0);
	
	return true;
}



//int create_domain_via_xl_cmd(int argc, char **argv)
int create_domain(int argc, char **argv)
{
	int ret = -1;

	ret = g_rpdomid++;

	return ret;
}


//note this function needs error handling and hardcoded param removal
int tftp_get_file (char* dir, char *cmdline)
{	
	char cmd_buf[2048] = {0};
	
	printf("%s\n", cmdline);
	sprintf (cmd_buf, "%s %s \"%s\"", g_tftp_script, dir, cmdline);
	return system(cmd_buf);
}

//note this function needs error handling and hardcoded param removal
int tftp_get_and_uncompress (char* dir, char *cmdline)
{	
	char cmd_buf[2048] = {0};
	
	printf("%s\n", cmdline);
	sprintf (cmd_buf, "%s %s \"%s\"", g_tftp_uncompress, dir, cmdline);
	return system(cmd_buf);
}

#ifdef STANDALONE

int main(int argc, char **argv)
{
	char* av[] = {"-name", "cli", "-memory", "64", "-bridge", "xenbr0", 
	"-kernel", "/boot/vmlinuz-3.7.1-dU", "-ramdisk" ,"/home/v/xentest/tmp/cli.igz"};

	return create_domain(argc, argv);
        //return create_domain_via_xl_cmd(argc, argv)
}
#endif
