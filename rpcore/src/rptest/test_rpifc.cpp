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

#include "tao.h"
#include "tcIO.h"
#include "logging.h"
#include "tcconfig.h"
#include "rp_api_code.h"
#include "channelcoding.h"


extern tcChannel   g_reqChannel;

bool  test_rp_request(tcChannel& chan, bool* pfTerminate)
{
    int                 procid;
    int                 origprocid;
    u32                 uReq;
    u32                 uStatus;

    char*               szappexecfile= NULL;
    int                 sizehash= 32; //sha256 size
    byte                hash[32];

    int                 inparamsize;
    byte                inparams[PARAMSIZE];

    int                 outparamsize;
    byte                outparams[PARAMSIZE];

    int                 size;
    byte                rgBuf[PARAMSIZE];

    int                 pid;
    u32                 uType= 0;
    int                 an = 0;
    char*               av[20];



    // get request
    inparamsize= PARAMSIZE;
    
    if(!chan.gettcBuf(&procid, &uReq, &uStatus, &origprocid, &inparamsize, inparams)) {
        fprintf(stdout, "serviceRequest: gettcBuf failed\n");
        return false;
    }
    
    if(uStatus==TCIOFAILED) {
        chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
        return false;
    }

    switch(uReq) {
		
		
		default:
			break;
			
	     case RP2VM_STARTAPP:

#ifdef TCTEST1
		fprintf(stdout, "serviceRequest, RP2VM_STARTAPP, decoding\n");
#endif
				an= 20;
				if(!decodeVM2RP_STARTAPP(&szappexecfile, &an, (char**) av, inparams)) {
					fprintf(g_logFile, "serviceRequest: decodeRP2VM_STARTAPP failed\n");
					chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return false;
				}
				outparamsize= PARAMSIZE;
		
#ifdef TEST
		fprintf(stdout, "serviceRequest, about to StartHostedProgram %s, for %d\n", szappexecfile, origprocid);
#endif

#ifdef TEST
		fprintf(stdout, "serviceRequest, StartHostedProgram succeeded, about to send\n");
#endif

				if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
					fprintf(stdout, "serviceRequest: sendtcBuf (startapp) failed\n");
					chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return false;
				}
				return true;
    }
	
	return true;
	
}


void test_interface_main(){
	bool                fTerminate= false;
	while(1) {
		test_rp_request( g_reqChannel, &fTerminate);
	};
}


void test_host_init() {
	const char** parameters = NULL;    
	const char*         directory= NULL;
	int status = 0;
	
    directory = &g_config_dir[0];
    int parameterCount = 0;
    
    taoHostServices host;
    taoEnvironment  trustedHome;
    
	parameters = (const char**)&directory;
	parameterCount = 1;
	
	host.HostInit(PLATFORMTYPEHW, parameterCount, parameters);
	
	trustedHome.EnvInit(PLATFORMTYPELINUX, "TrustedOS",
                                "www.foo.com", directory, 
                                &host, 0, NULL);
  /*
    taoInit     myInit(&host);
    printf("Waiting for RP certification by server\n");
	while (TAOINIT_DONE == status) {
		status = myInit.do_rp_certification();
	}
*/
	
}
