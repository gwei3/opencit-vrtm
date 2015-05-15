//
// Author: Vinay Phegade
//
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <errno.h>
#include <sys/stat.h>

#include "modtcService.h"
#include "tcIO.h"
#include "logging.h"
#include "tcconfig.h"
#include "rp_api_code.h"
#include "channelcoding.h"
#include "jlmUtility.h"
//#include "dombuilder.h"

//global variables. 
// note: handle with care, may buf overflow
char 	g_python_scripts[256] = "../scripts";
char 	g_config_dir[256] 	= "/tmp/rptmp/config";
char 	g_staging_dir[256] 	= "/tmp/rpimg/";
char 	g_config_file[256] 	= "tcconfig.xml";
int  	g_mode  			= 	RP_MODE_OPENSTACK;
char 	g_kns_ip [64] 		= "127.0.0.1";
char 	g_domain[512] 		= "www.rptest.com";
int 	g_kns_port = 0;


char 	g_rpcore_ip [64] 		= "127.0.0.1";
int 	g_rpcore_port = 16005;
int	g_max_thread_limit = 64;

extern tcChannel   g_reqChannel;

int get_rp_addr(const char * dev, char * ipv4) 
{
    struct ifreq ifc;
    int res;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if(fd < 0)
        return -1;
    
    strcpy(ifc.ifr_name, dev);
    
    res = ioctl(fd, SIOCGIFADDR, &ifc);
    
    close(fd);
    
    if(res < 0)
        return -1;     
    
    strcpy(ipv4, inet_ntoa(((struct sockaddr_in*)&ifc.ifr_addr)->sin_addr));
    
    return 0;
}

#define MAX_BUF 1024*32



int  get_rp_configuration(tcChannel& chan)
{
    int                 procid;
    int                 origprocid;
    u32                 uReq;
    u32                 uStatus;


    int                 inparamsize;
    byte                inparams[MAX_BUF];

    int                 outparamsize;
    byte                outparams[MAX_BUF];

    int                 size;
//    byte                rgBuf[PARAMSIZE];


	bool 				status;

    // get request
    inparamsize= MAX_BUF;
    
		
    if(!chan.gettcBuf(&procid, &uReq, &uStatus, &origprocid, &inparamsize, inparams)) {
        fprintf(stdout, "serviceRequest: gettcBuf failed\n");
        return TCSERVICE_RESULT_FAILED;
    }
    
    if(uStatus==TCIOFAILED) {
        chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
        return false;
    }

	fprintf(stdout, "config file size is %d\n", inparamsize);
	
    switch(uReq) {
		
		
		default:
			outparamsize = sprintf ((char*)outparams, "Host configuration needs to be set."); 
			chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, outparamsize, outparams);
			return TCSERVICE_RESULT_FAILED;
			
	     case RP2VM_SETFILE:
				
				if(!decodeVM2RP_SETFILE(&outparamsize, outparams, inparams)) {
					fprintf(g_logFile, "serviceRequest: decodeRP2VM_STARTAPP failed\n");
					chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return TCSERVICE_RESULT_FAILED;
				}
				
				
				memcpy(g_config_file, outparams, outparamsize);
				
				// filename will be in outparams
				
				size = (status)? sizeof(outparamsize): 0;
				
		        outparamsize= encodeRP2VM_SETFILE(size,(byte*) &outparamsize, PARAMSIZE, outparams);
				if(outparamsize<0) {
					fprintf(g_logFile, "serviceRequest: encodeRP2VM_SEALFOR buf too small\n");
					chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return TCSERVICE_RESULT_FAILED;
				}
		
				if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
					fprintf(stdout, "serviceRequest: sendtcBuf (startapp) failed\n");
					chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return TCSERVICE_RESULT_FAILED;
				}
				break;
    }
	
	return TCSERVICE_RESULT_SUCCESS;
	
}


static TiXmlNode* findNode(TiXmlNode* pNode, const char* szElementName)
{
    TiXmlNode* pNode1;

    while(pNode) {
		
        if(pNode->Type()==TiXmlNode::TINYXML_ELEMENT) {
            if(strcmp(((TiXmlElement*)pNode)->Value(),szElementName)==0) {
                return pNode;
            }
        }
        
        pNode1= findNode(pNode->FirstChild(),szElementName);
        if(pNode1!=NULL)
           return pNode1; 
           
        pNode= pNode->NextSibling();
    }

    return NULL;
}

static const char* findValueOf(TiXmlNode* pRoot, const char* szElementName) {
	
	TiXmlNode* 	pchild = NULL;
	TiXmlNode* 	pnode = NULL;
	
		pnode  = findNode(pRoot, szElementName);
		if (!pnode )
			return NULL;
			
		pchild = pnode->FirstChild();
		if (!pchild)
			return NULL;
	
		return pchild->Value();
}

int read_config(const char* szInFile ){
	
	TiXmlDocument   	doc;

	
	
	const char* value;
	int rc = -1;
	
	if (!szInFile ) 
		return rc;
		

	
	if(!doc.LoadFile(szInFile)) {
		fprintf(stdout, "Cant load file %s\n", szInFile);
		return -1;
	}
	
	TiXmlElement*   pRoot= doc.RootElement();
	
	if ( pRoot ) {
			
		const char* mode = findValueOf(pRoot, "mode");
		const char* kns_ip = findValueOf(pRoot, "signing_server_ip");
		const char* kns_port =  findValueOf(pRoot, "signing_server_port");
		const char* domain = findValueOf(pRoot, "cert_domain");
		const char* rpcore_ip = findValueOf(pRoot, "rpcore_ip"); 
		const char* rpcore_port = findValueOf(pRoot, "rpcore_port"); 	//"6005";
		const char* mtwproxy_ip = findValueOf(pRoot, "mtwproxy_ip");
		const char* mtwproxy_port = findValueOf(pRoot, "mtwproxy_port");
		const char* max_thread_limit = findValueOf(pRoot, "max_thread_limit");
		
		if (mode ) {
			if ( strncasecmp( mode, "openstack", strlen("openstack")) == 0)
				g_mode = RP_MODE_STANDALONE;
				
			fprintf(stdout, "mode = %s\n", mode);
		}
		
		if ( kns_ip ) {
			memcpy(g_kns_ip, kns_ip, strlen(kns_ip));
			fprintf(stdout, "signing_server = %s\n", kns_ip);
		}
		
		if ( kns_port ) {
			g_kns_port =  atoi(kns_port);
			fprintf(stdout, "signing_server port = %d\n", g_kns_port);
		}
		
		if ( rpcore_ip ) {
			memcpy(g_rpcore_ip, rpcore_ip, strlen(rpcore_ip));
			fprintf(stdout, "rpcore ip = %s\n", rpcore_ip);
		}
		if ( rpcore_port ) {
			g_rpcore_port =  atoi(rpcore_port);
			fprintf(stdout, "rp core service port = %d\n", g_rpcore_port);
		}
		
		if ( domain ) {
			memcpy(g_domain, domain, strlen(domain));
			fprintf(stdout, "cert domain  = %s\n", domain);
		}
		
		if ( max_thread_limit ) {
			g_max_thread_limit = atoi(max_thread_limit);
			fprintf(stdout,"max thread limit = %d", g_max_thread_limit);
		}
		
		if ( mtwproxy_ip ) {
			memcpy(g_mtwproxy_ip, mtwproxy_ip, strlen(mtwproxy_ip));
			fprintf(stdout, "signing_server = %s\n", mtwproxy_ip);
			g_mtwproxy_on = 1;
		}
		
		if ( mtwproxy_port ) {
			g_mtwproxy_port =  atoi(mtwproxy_port);
			fprintf(stdout, "signing_server port = %d\n", g_mtwproxy_port);
		}
		
	}
		
	rc = 0;
	
	return 0;
}

int LoadConfig(const char* szInFile ){
	
	
	int rc = -1;
	char cmd_line[256];
	struct stat statBlock;
  

    bool is_statted = false;
    if( stat(g_config_dir, &statBlock) == 0) {
        is_statted = true;
	}
    else if( stat("../../rptmp/config", &statBlock) == 0) {
        strcpy(g_config_dir, "../../rptmp/config");
        is_statted = true;
    }
    else if( stat (getenv("RP_CONFIG"), &statBlock) == 0 ) {
        strcpy(g_config_dir, getenv("RP_CONFIG"));
        is_statted = true;
    }
		
    rc = get_rp_addr("xenbr0", g_rpcore_ip);
    if(rc < 0)
        return rc;

    if(is_statted) {
            fprintf (stdout, "Host IP is %s\n", g_rpcore_ip );	    
    }
    else {
	fprintf (stdout, "rptmp dir is not found at required locations");
	return -1;
    }
    sprintf(cmd_line, "%s/%s", g_config_dir, g_config_file);
	read_config(cmd_line);
	rc = TCSERVICE_RESULT_SUCCESS;
	
	return rc;
}
 
#if 0//def CONFIG_TEST
int main(int argc, char** argv) 
{
	
	LoadConfig (argv[1]);
	return 0;
}

#endif
