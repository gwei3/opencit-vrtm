//
//  File: rpcoreservice.cpp
//  Description: tcService implementation
//
//	Copyright (c) 2012, Intel Corporation. 
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



#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef LINUX
#include <linux/un.h>
#else
#include <sys/un.h>
#endif
#include <errno.h>

#include "tcIO.h"
#include "logging.h"
#include "tcconfig.h"
#include "jlmcrypto.h"

int  g_quit = 0;

int g_fdLock;
int singleInstanceRpcoreservice()
{
    //const char *pidPath="/var/run/rpcoreservice.pid";
    const char *lockFile="/tmp/rpcoreservice__DT_XX99";
    
    struct flock rpcsFlock;
    
    rpcsFlock.l_type = F_WRLCK;
    rpcsFlock.l_whence = SEEK_SET;
    rpcsFlock.l_start = 0;
    rpcsFlock.l_len = 1;

    if( ( g_fdLock = open(lockFile, O_WRONLY | O_CREAT, 0666)) == -1) 
    {
#ifdef TEST
    	fprintf(stdout, "Can't open rpcoreservice lock file \n");
#endif
        return -1;
     }
     
     chmod(lockFile, 0666); // Just in case umask is not set properly.
     
     if ( fcntl(g_fdLock, F_SETLK, &rpcsFlock) == -1) {

#ifdef TEST
    	fprintf(stdout, "Already locked - rpcoreservice lock file \n");
#endif
		return -2;
     }

    return 1;

}
//test function declarations
void test_host_init();
int modmain(int an, char** av);

int main(int an, char** av)
{
    int            	iRet= 0;
    int 			instanceValid = 0;
    const char*		configfile = "./tcconfig.xml";
//------------------------------------------------------------------
    
    if ((instanceValid = singleInstanceRpcoreservice()) == -2) {
    	fprintf(stdout, "Process(rpcoreservice) already running\n");
		return 1;
    }
    
    if(instanceValid == -1) {
    	fprintf(stdout, "Process(rpcoreservice) could not open lock file\n");
		return 1;
    }

//--------------------------------------------------------------------
	  initLog("log_rpcoresvc.log");
	  
//------------------------------------------------------------------
	/*
	 * Start rpinterface
	 */

	//init linux service 

	if(!start_rp_interface(NULL)) {
		fprintf(g_logFile, "%s : %d: cant init start_rp_interface\n", __FUNCTION__, __LINE__);
		goto cleanup;
	}

//------------------------------------------------------------------
   if(!initAllCrypto()) {
        //fprintf(g_logFile, "tcService main: can't initcrypto\n");
        iRet= 1;
        goto cleanup;
    }
//------------------------------------------------------------------       

    //
	// initialize configuration
	//
	

	if ( LoadConfig(configfile) < 0 ) {
		fprintf(stdout, "tcService main: can't load config file %s\n", configfile);
		goto cleanup;
	}
	
	if ( modmain(an, av) == 1) {
		fprintf(stdout, "tcService main: initialization failed");
        goto cleanup;
    }
	
	g_quit = 1;
	
	sleep(100);
//------------------------------------------------------------------

cleanup:
	  closeLog();
	  return iRet;
}
