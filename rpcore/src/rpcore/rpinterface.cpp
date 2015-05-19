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

/*
#include "../jlmcrypto/algs.h"
#include "serviceHash.inc"
#include "policyKey.inc"*/
#include "tcconfig.h"

#include "tcpchan.h"

/*************************************************************************************************************/
tcChannel   g_reqChannel;
#if 1

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

unsigned    tciodd_serviceInitilized= 1;
int         tciodd_servicepid= 0;

#ifndef byte
typedef unsigned char byte;
#endif

//static int domid = 100;

// Q's for read and write
struct tciodd_Qent {
    int                 m_pid;
    byte*               m_data;
    int                 m_sizedata;
    struct tciodd_Qent* m_next;
    //void*               m_next;
};

struct tciodd_Qent* tciodd_makeQent(int pid, int sizedata, byte* data, 
                                                struct tciodd_Qent* next);
void   tciodd_deleteQent(struct tciodd_Qent* pent);
int    tciodd_insertQent(struct tciodd_Qent** phead, struct tciodd_Qent* pent);
int    tciodd_appendQent(struct tciodd_Qent** phead, struct tciodd_Qent* pent);
int    tciodd_removeQent(struct tciodd_Qent** phead, struct tciodd_Qent* pent);
struct tciodd_Qent* tciodd_findQentbypid(struct tciodd_Qent* head, int pid);


#define TYPE(minor)     (((minor) >> 4) & 0xf)
#define NUM(minor)      ((minor) & 0xf)


#define JIFTIMEOUT 100

sem_t     sem_req; 
struct tciodd_Qent* tciodd_reqserviceq= NULL;

sem_t sem_resp; 
struct tciodd_Qent* tciodd_resserviceq= NULL;

// -------------------------------------------- ----------------------------------


struct tciodd_Qent* tciodd_makeQent(int pid, int sizedata, byte* data, 
                                    struct tciodd_Qent* next)
{
    struct tciodd_Qent* pent= NULL;

    pent= (struct tciodd_Qent*) malloc(sizeof(struct tciodd_Qent));

    if(pent ==NULL)
        return NULL;

    pent->m_pid= pid;
    pent->m_data= data;
    pent->m_sizedata= sizedata;
    pent->m_next= NULL;
    return pent;
}


void tciodd_deleteQent(struct tciodd_Qent* pent)
{
    memset((void*)pent, 0, sizeof(struct tciodd_Qent));
    free((void*)pent);
    return;
}


int  tciodd_insertQent(struct tciodd_Qent** phead, struct tciodd_Qent* pent)
{
    pent->m_next= *phead;
    *phead= pent;
    return 1;
}


int  tciodd_appendQent(struct tciodd_Qent** phead, struct tciodd_Qent* pent)
{
    struct tciodd_Qent* p;

    pent->m_next= NULL;
    if(*phead==NULL) {
        *phead= pent;
	//fprintf(stdout, "added pent\n");
        return 1;
    }
    p= *phead;

    while(p->m_next!=NULL) {
        p= p->m_next;
    }
    p->m_next= pent;
	
    //fprintf(stdout, "appended pent\n");
    return 1;
}


int  tciodd_removeQent(struct tciodd_Qent** phead, struct tciodd_Qent* pent)
{
    struct tciodd_Qent* pPrev= NULL;
    struct tciodd_Qent* p= NULL;

    if(*phead==NULL)
        return 0;
    if(pent==*phead) {
        *phead= pent->m_next;
        return 1;
    }
    pPrev= *phead;
    p= (*phead)->m_next;

    while(p!=NULL) {
        if(p==pent) {
            pPrev->m_next= p->m_next;
            return 1;
        }
        pPrev= p;
        p= p->m_next;
    }
    return 0;
}


struct tciodd_Qent* tciodd_findQentbypid(struct tciodd_Qent* head, int pid)
{
    struct tciodd_Qent* p= head;

    while(p!=NULL) {
        if(p->m_pid==pid)
            return p;
        p= p->m_next;
    }
    return NULL;
}


// --------------------------------------------------------------------


bool sendTerminate(int procid, int origprocid)
{
    tcBuffer*           newhdr= NULL;
    int                 newdatasize;
    byte*               newdata;
    struct tciodd_Qent* pent= NULL;

    //  create new buffer
    newdatasize= sizeof(tcBuffer);
    newdata=  (byte*) malloc(newdatasize);
    if(newdata==NULL)
        return false;

    //  make header and copy answer
    newhdr= (tcBuffer*) newdata;
    newhdr->m_procid= origprocid;
    newhdr->m_reqID= TCSERVICETERMINATE;
    newhdr->m_ustatus= TCIOSUCCESS;
    newhdr->m_origprocid= origprocid;
    newhdr->m_reqSize= 0;

    //  adjust pent
    pent= tciodd_makeQent(tciodd_servicepid, newdatasize, newdata, NULL);
    if(pent==NULL)
        return false;

//    if(down_interruptible(&tciodd_reqserviceqsem)) 
//        return false;
    sem_wait(&sem_req);
    tciodd_appendQent(&tciodd_reqserviceq, pent);

    sem_post(&sem_req);
//    up(&tciodd_reqserviceqsem);
    return true;
}


bool copyResultandqueue(struct tciodd_Qent* pent, u32 type, int sizebuf, byte* buf)
//  Copy from buf to new ent
{
    int         n= 0;
    tcBuffer*   hdr= (tcBuffer*)pent->m_data;
    tcBuffer*   newhdr= NULL;
    int         newdatasize;
    byte*       newdata;
    int         m= 0;

#ifdef TESTDEVICE
    printk(KERN_DEBUG "tcioDD: copyResultandqueue\n");
#endif
    //  create new buffer
    newdatasize= sizebuf+sizeof(u32)+sizeof(int)+sizeof(tcBuffer);
    newdata =  (byte*)malloc(newdatasize);
    if(newdata==NULL)
        return false;

    //  make header and copy answer
    newhdr= (tcBuffer*) newdata;
    newhdr->m_procid= hdr->m_procid;
    newhdr->m_reqID= hdr->m_reqID;
    newhdr->m_ustatus= TCIOFAILED;
    newhdr->m_origprocid= hdr->m_origprocid;
    newhdr->m_reqSize= newdatasize-sizeof(tcBuffer);
    m= sizeof(tcBuffer);
    memcpy(newdata+m, (byte*) &type, sizeof(u32));
    m+= sizeof(u32);
    memcpy(newdata+m, (byte*) &sizebuf, sizeof(int));
    m+= sizeof(int);
    memcpy(newdata+m, buf, sizebuf);

    //  delete old buffer if non NULL
    if(pent->m_data!=NULL) {
        free(pent->m_data);
        pent->m_data= NULL;
    }

    //  adjust pent
    hdr= (tcBuffer*)newdata;
    pent->m_sizedata= newdatasize;
    pent->m_data= newdata;
    newhdr->m_ustatus= TCIOSUCCESS;

//    if(down_interruptible(&tciodd_resserviceqsem)) 
//        return false;
    sem_wait(&sem_resp);
    n= tciodd_appendQent(&tciodd_resserviceq, pent);
//    up(&tciodd_resserviceqsem);
    sem_post(&sem_resp);
    return true;
}


bool queueforService(struct tciodd_Qent* pent, u32 appReq, u32 serviceReq)
{
    int         n =0;
    tcBuffer*   hdr= (tcBuffer*)pent->m_data;

#ifdef TEST
    printf( "tcioDD: queueforService appCode: %d serviceCode: %d\n",
            appReq, serviceReq);
#endif
    // adjust and append to waitq
//    if(down_interruptible(&tciodd_resserviceqsem)) 
//        return false;
    sem_wait(&sem_resp);

    hdr->m_origprocid= hdr->m_procid;
    hdr->m_procid= tciodd_servicepid;
    hdr->m_reqID= serviceReq;
    hdr->m_ustatus= TCIOSUCCESS;
    pent->m_pid= tciodd_servicepid;
    n= tciodd_appendQent(&tciodd_resserviceq, pent);

//    up(&tciodd_resserviceqsem);
    sem_post(&sem_resp);
    return true;
}


bool queueforApp(struct tciodd_Qent* pent, u32 appReq, u32 serviceReq)
{
    tcBuffer*   hdr= (tcBuffer*) pent->m_data;

#ifdef TESTDEVICE
    printk(KERN_DEBUG "tcioDD: queueforApp appCode: %d serviceCode: %d\n",
            appReq, serviceReq);
#endif
    // adjust and append to waitq
//    if(down_interruptible(&tciodd_resserviceqsem))
//        return false;
    
    sem_wait(&sem_resp);

    hdr->m_procid= hdr->m_origprocid;
    hdr->m_reqID= appReq;
    pent->m_pid= hdr->m_origprocid;
    tciodd_appendQent(&tciodd_resserviceq, pent);
//    up(&tciodd_resserviceqsem);
 
    sem_post(&sem_resp);

   return true;
}


bool tciodd_processService(void)
//  Take entry off request queue, service it and put it on response queue
{
    int                 n= 0;
    struct tciodd_Qent* pent= NULL;
    tcBuffer*           hdr= NULL;
    int                 datasize= 0;
    byte*               data= NULL;
    bool                fRet= true;

 //   fprintf(stdout, "tciodd_processService started\n");

//    if(down_interruptible(&tciodd_reqserviceqsem)) 
//        return false;
	   
    sem_wait(&sem_req);

    pent= tciodd_reqserviceq;
    n= tciodd_removeQent(&tciodd_reqserviceq, pent);
//    up(&tciodd_reqserviceqsem);
	
    sem_post(&sem_req);

    if(n<=0 || pent==NULL)
        return false;

    datasize= pent->m_sizedata;
    data= pent->m_data;
    hdr= (tcBuffer*)data;

#if 0
    fprintf(stdout, "processService got ent from proc  %d, reqID %d size: %d\n", 
           hdr->m_procid, hdr->m_reqID, hdr->m_reqSize);
    PrintBytes("processService ", pent->m_data, pent->m_sizedata );
#endif

    // don't forget to wake up reading processes
    switch(hdr->m_reqID) {

      // For first four, no need to go to service
      case VM2RP_GETRPHOSTQUOTE:
        if(!queueforService(pent, VM2RP_GETRPHOSTQUOTE, RP2VM_GETRPHOSTQUOTE)) {
            fRet= false;
        }
        break;
      case RP2VM_GETRPHOSTQUOTE:
        if(!queueforApp(pent, VM2RP_GETRPHOSTQUOTE, RP2VM_GETRPHOSTQUOTE)) {
            fRet= false;
        }
        break;

	case VM2RP_GETTPMQUOTE:
        if(!queueforService(pent, VM2RP_GETTPMQUOTE, RP2VM_GETTPMQUOTE)) {
            fRet= false;
        }
        break;
      case RP2VM_GETTPMQUOTE:
        if(!queueforApp(pent, VM2RP_GETTPMQUOTE, RP2VM_GETTPMQUOTE)) {
            fRet= false;
        }
        break;

      case VM2RP_RPHOSTQUOTERESPONSE:
        if(!queueforService(pent, VM2RP_RPHOSTQUOTERESPONSE, RP2VM_RPHOSTQUOTERESPONSE)) {
            fRet= false;
        }
        break;
      case RP2VM_RPHOSTQUOTERESPONSE:
        if(!queueforApp(pent, VM2RP_RPHOSTQUOTERESPONSE, RP2VM_RPHOSTQUOTERESPONSE)) {
            fRet= false;
        }
        break;
        
      case VM2RP_SETFILE:
        if(!queueforService(pent, VM2RP_SETFILE, RP2VM_SETFILE)) {
            fRet= false;
        }
        break;
      case RP2VM_SETFILE:
        if(!queueforApp(pent, VM2RP_SETFILE, RP2VM_SETFILE)) {
            fRet= false;
        }
        break;

     case VM2RP_CHECK_VM_VDI:
        if(!queueforService(pent, VM2RP_CHECK_VM_VDI, RP2VM_CHECK_VM_VDI)) {
            fRet= false;
        }
        break;
      case RP2VM_CHECK_VM_VDI:
        if(!queueforApp(pent, VM2RP_CHECK_VM_VDI, RP2VM_CHECK_VM_VDI)) {
            fRet= false;
        }
        break;

     case VM2RP_SETUUID:
        if(!queueforService(pent, VM2RP_SETUUID, RP2VM_SETUUID)) {
            fRet= false;
        }
        break;
      case RP2VM_SETUUID:
        if(!queueforApp(pent, VM2RP_SETUUID, RP2VM_SETUUID)) {
            fRet= false;
        }
        break;

      case VM2RP_GETOSHASH:
        if(!queueforService(pent, VM2RP_GETOSHASH, RP2VM_GETOSHASH)) {
            fRet= false;
        }
        break;
      case RP2VM_GETOSHASH:
        if(!queueforApp(pent, VM2RP_GETOSHASH,  RP2VM_GETOSHASH)) {
            fRet= false;
        }
        break;

      // forward to service or app as appropriate
      case VM2RP_GETOSCREDS:
        if(!queueforService(pent, VM2RP_GETOSCREDS,  RP2VM_GETOSCREDS)) {
            fRet= false;
        }
        break;
      case RP2VM_GETOSCREDS:
        if(!queueforApp(pent, VM2RP_GETOSCREDS,  RP2VM_GETOSCREDS)) {
            fRet= false;
        }
        break;
		
	  case VM2RP_GETOSCERT:
        if(!queueforService(pent, VM2RP_GETOSCERT, RP2VM_GETOSCERT)) {
            fRet= false;
        }
        break;
      
      case RP2VM_GETOSCERT:
        if(!queueforApp(pent, VM2RP_GETOSCERT,   RP2VM_GETOSCERT)) {
            fRet= false;
        }
        
        break;

       case VM2RP_GETAIKCERT:
        if(!queueforService(pent, VM2RP_GETAIKCERT, RP2VM_GETAIKCERT)) {
            fRet= false;
        }
        break;

      case RP2VM_GETAIKCERT:
        if(!queueforApp(pent, VM2RP_GETAIKCERT,   RP2VM_GETAIKCERT)) {
            fRet= false;
        }

        break;

      case VM2RP_SEALFOR:
        if(!queueforService(pent, VM2RP_SEALFOR,   RP2VM_SEALFOR)) {
            fRet= false;
        }
        break;
      case RP2VM_SEALFOR:
        if(!queueforApp(pent, VM2RP_SEALFOR,  RP2VM_SEALFOR)) {
            fRet= false;
        }
        break;

      case VM2RP_UNSEALFOR:
        if(!queueforService(pent, VM2RP_UNSEALFOR,  RP2VM_UNSEALFOR)) {
            fRet= false;
        }
        break;

      case RP2VM_UNSEALFOR:
        if(!queueforApp(pent, VM2RP_UNSEALFOR, 
                        RP2VM_UNSEALFOR)) {
            fRet= false;
        }
        break;

      case VM2RP_ATTESTFOR:
        if(!queueforService(pent, VM2RP_ATTESTFOR,   RP2VM_ATTESTFOR)) {
            fRet= false;
        }
        break;
      case RP2VM_ATTESTFOR:
        if(!queueforApp(pent, VM2RP_ATTESTFOR, RP2VM_ATTESTFOR)) {
            fRet= false;
        }
        break;

      case VM2RP_STARTAPP:
        if(!queueforService(pent, VM2RP_STARTAPP, RP2VM_STARTAPP)) {
            fRet= false;
        }
        break;
      case RP2VM_STARTAPP:
        if(!queueforApp(pent, VM2RP_STARTAPP, RP2VM_STARTAPP)) {
            fRet= false;
        }
        break;

      case VM2RP_TERMINATEAPP:
        if(!queueforService(pent, VM2RP_TERMINATEAPP, RP2VM_TERMINATEAPP)) {
            fRet= false;
        }
        break;
      case RP2VM_TERMINATEAPP:
        if(!queueforApp(pent, VM2RP_TERMINATEAPP, RP2VM_TERMINATEAPP)) {
            fRet= false;
        }
        break;
        case RP2VM_GETRPID:
        if (!queueforApp(pent, VM2RP_GETRPID, RP2VM_GETRPID)) {
                fRet = false;
        }
        break;
        case VM2RP_GETRPID:
        if (!queueforService(pent, VM2RP_GETRPID, RP2VM_GETRPID)) {
                fRet = false;
        }
        break;
      case RP2VM_GETVMMETA:
        if (!queueforApp(pent, VM2RP_GETVMMETA, RP2VM_GETVMMETA)) {
                fRet = false;
        }
        break;
      case VM2RP_GETVMMETA:
        if (!queueforService(pent, VM2RP_GETVMMETA, RP2VM_GETVMMETA)) {
              fRet = false;
        }
        break;
      case VM2RP_GETPROGHASH:
        if(!queueforService(pent, VM2RP_GETPROGHASH, RP2VM_GETPROGHASH)) {
            fRet= false;
        }
        break;
      case RP2VM_GETPROGHASH:
        if(!queueforApp(pent, VM2RP_GETPROGHASH,  RP2VM_GETPROGHASH)) {
            fRet= false;
        }
        break;

      case VM2RP_IS_MEASURED:
        if(!queueforService(pent, VM2RP_IS_MEASURED, RP2VM_IS_MEASURED)) {
            fRet= false;
        }
        break;
      case RP2VM_IS_MEASURED:
        if(!queueforApp(pent, VM2RP_IS_MEASURED,  RP2VM_IS_MEASURED)) {
            fRet= false;
        }
        break;
      case RP2VM_ISVERIFIED:
    	  if(!queueforApp(pent, VM2RP_ISVERIFIED,  RP2VM_ISVERIFIED)) {
			  fRet= false;
		  }
    	  break;
      case VM2RP_ISVERIFIED:
    	  if(!queueforService(pent, VM2RP_ISVERIFIED, RP2VM_ISVERIFIED)) {
			  fRet= false;
		  }
		  break;
      case RP2VM_GETVMREPORT:
         if(!queueforApp(pent, VM2RP_GETVMREPORT,  RP2VM_GETVMREPORT)) {
                         fRet= false;
                 }
         break;
      case VM2RP_GETVMREPORT:
         if(!queueforService(pent, VM2RP_GETVMREPORT, RP2VM_GETVMREPORT)) {
                         fRet= false;
                 }
                 break;

       case TCSERVICETERMINATE:
        if(!queueforService(pent, TCSERVICETERMINATE, TCSERVICETERMINATE)) {
            fRet= false;
        }
        break;

      default:
        fRet= false;
        break;
    }

    return fRet;
}


// ------------------------------------------------------------------------------



int tc_close()
{
    int                 pid= 0;
    struct tciodd_Qent* pent= NULL;


    // make sure q's don't have entries with this pid
	sem_wait(&sem_req);

        for(;;) {
            pent= tciodd_findQentbypid(tciodd_reqserviceq, pid);
            if(pent==NULL)
                break;
            if(pent->m_data!=NULL) {
                memset(pent->m_data, 0, pent->m_sizedata);
                free(pent->m_data);
            }
            pent->m_data= NULL;
            tciodd_removeQent(&tciodd_reqserviceq, pent); 
            pent= NULL;
        }
        sem_post(&sem_req);

	sem_wait(&sem_resp);

	for(;;) {
            pent= tciodd_findQentbypid(tciodd_resserviceq, pid);
            if(pent==NULL)
                break;
            if(pent->m_data!=NULL) {
                memset(pent->m_data, 0, pent->m_sizedata);
                free(pent->m_data);
            }
            pent->m_data= NULL;
            tciodd_removeQent(&tciodd_resserviceq, pent); 
            pent= NULL;
        }
    	
	sem_post(&sem_resp);
    
    sendTerminate(tciodd_servicepid, pid);
    
    return 1;
}


ssize_t tc_read(int pid, char *buf, size_t count)
{
    ssize_t             retval= 0;
    struct tciodd_Qent* pent= NULL;

    //fprintf(stdout, "tc_read pid %d, count %d\n", pid, count);
  
    // if something is on the response queue, fill buffer, otherwise wait
     for(;;) {
		fprintf(stdout,"before sem pid is : %d\n",pid);
		fflush(stdout);	
		sem_wait(&sem_resp);
			
		pent= tciodd_findQentbypid(tciodd_resserviceq, pid);
		
		sem_post(&sem_resp);
			
		if(pent!=NULL) {
			   fprintf(stdout, "pent found for %d\n", pid);
			fprintf(stdout,"data size of entry %d against data %d\n",pent->m_sizedata,count);
			fflush(stdout);
				break;
		}
		
	   //wait_event_timeout(dev->waitq, tciodd_resserviceq!=NULL, TIMEOUT);
		//fprintf(stdout, "tc_read pid %d, waiting for write to happen\n", pid);
		fprintf(stdout,"pid : %d not found in queue\n",pid);
		fflush(stdout);
		pthread_mutex_lock(&gm);
		fprintf(stdout,"pid:%d lock applied going to conditional wait\n",pid);
		fflush(stdout);
		pthread_cond_wait(&gc, &gm);
		pthread_mutex_unlock(&gm);
		fprintf(stdout,"pid : %d lock released\n",pid);
    }


    if(pent->m_sizedata <= count) {
        memcpy(buf, pent->m_data, pent->m_sizedata);
        retval= pent->m_sizedata;
	fprintf(stdout,"copied data in buff\n ");
	
//        PrintBytes("Data in tc_read", (byte*)buf, (retval > sizeof(tcBuffer))? sizeof(tcBuffer): retval);
    }
    else {
        retval= -EFAULT;
    }
	fprintf(stdout,"before removing from q and sem_wait\n");
	fflush(stdout);
	sem_wait(&sem_resp);
   
	tciodd_removeQent(&tciodd_resserviceq, pent);
    
        // erase and free entry and data
        if(pent->m_data!=NULL) {
            memset(pent->m_data, 0, pent->m_sizedata);
            free(pent->m_data);
        }
        
	pent->m_data= NULL;
        tciodd_deleteQent(pent);
	fprintf(stdout,"removed from queue pid:%d\n",pid);
	fflush(stdout);
	sem_post(&sem_resp);

    return retval;
}


ssize_t tc_write(int pid, const char *buf, size_t count )
{
    ssize_t                 retval= -ENOMEM;
    byte*                   databuf= NULL;
    struct tciodd_Qent*     pent= NULL;
    tcBuffer*               pCBuf= NULL;


    fprintf(stdout, "tc_write pid %d, size %d\n", pid, count);

    // add to tciodd_reqserviceQ then process
    if(count < sizeof(tcBuffer)) {
        retval= -EFAULT;
        goto out;
    }

    databuf= (byte*) malloc(count);
    if(databuf==NULL) {
        retval= -EFAULT;
        goto out;
    }

    memcpy(databuf, buf, count);

    // make tcheader authoritative
    pCBuf= (tcBuffer*) databuf;
    pCBuf->m_procid= pid;
    if(pid!=tciodd_servicepid)
        pCBuf->m_origprocid= pid;

//    fprintf(stdout, "tc_write pid %d, before calling tciodd_makeQent\n", pid);

    pent= tciodd_makeQent(pid, count, databuf, NULL);
	fprintf(stdout,"creation of queue node\n");
    if(pent==NULL) {
        retval= -EFAULT;
	fprintf(stdout,"failed\n");
	fflush(stdout);
        goto out;
    }
	fprintf(stdout,"successful\nbefore sem pid:%d\n",pid);
	fflush(stdout);
    sem_wait(&sem_req);

    tciodd_appendQent(&tciodd_reqserviceq, pent);
	fprintf(stdout,"added to queue pid:%d\n",pid);
    sem_post(&sem_req);

    retval= count;

out:

    if(retval>=0)
        while(tciodd_processService());
	fprintf(stdout,"processed the service pid:%d\n",pid);
    //wake_up(&(dev->waitq));sem_wait
    fprintf(stdout, "tc_write pid %d, waking up waiting reads\n", pid);
	fflush(stdout);
    pthread_cond_broadcast(&gc);
    
     return retval;
}



#endif
/*************************************************************************************************************/
// -------------------------------------------------------------------


void tcBufferprint(tcBuffer* p)
{
    fprintf(g_logFile, "Buffer: procid: %ld, req: %ld, size: %ld, status: %ld, orig proc: %ld\n",
           (long int)p->m_procid, (long int)p->m_reqID, (long int)p->m_reqSize, 
           (long int)p->m_ustatus, (long int)p->m_origprocid); 
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
	sem_init(&sem_req, 0, 1);
	sem_init(&sem_resp, 0, 1);
	
	pthread_attr_init(&g_attr);
	pthread_create(&dom_listener_thread, &g_attr, dom_listener_main, (void*)NULL); 
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


int tcChannel::WriteBuf(byte* buf, int size)
{
#ifdef TCIODEVICEDRIVERPRESENT
#ifdef TCSERVICE
	//v:
    return tc_write(m_fd, (const char*)buf, size);
#else
    return write(m_fd, (const char*)buf, size);

#endif

#else
    return send(m_fd, buf, size, 0);
#endif
}


int tcChannel::ReadBuf(byte* buf, int size)
{
#ifdef TCIODEVICEDRIVERPRESENT
#ifdef TCSERVICE
	//v:
    return tc_read(m_fd, (char*) buf, size);
#else
    return read(m_fd, (char*) buf, size);

#endif

#else
    return recv(m_fd, buf, size, 0);
#endif
}


void tcChannel::CloseBuf()
{
#ifdef TCSERVICE
    sem_destroy(&sem_req);
    sem_destroy(&sem_resp);
#else

    close(m_fd);
#endif
}


// -------------------------------------------------------------------


bool tcChannel::gettcBuf(int* procid, u32* puReq, u32* pstatus, int* porigprocid, 
                         int* paramsize, byte* params)
{
    byte            rgBuf[PADDEDREQ];
    tcBuffer*       pReq= (tcBuffer*) rgBuf;
    int             i;
    int             n;

#ifdef TEST
    fprintf(g_logFile, "gettcBuf outstanding %d\n", m_fd);
#endif

    n= *paramsize+sizeof(tcBuffer);
    if(n>PADDEDREQ) {
        fprintf(g_logFile, "Buffer too small\n");
        return false;
    }
    i= ReadBuf(rgBuf, n);
	fprintf(stdout,"tc_read returned size of data read : %d\n",i);
	fflush(stdout);
    if(i<0) {
        fprintf(g_logFile, "ReadBufRequest failed in gettcBuf %d %d\n", i, n);
        return false;
    }

    // only for socket
    if(i==0) {
        fprintf(g_logFile, "Client closed connection? %d\n", m_fd);
        return false;
    }

#ifdef TEST
    fprintf(g_logFile, "gettcBuf succeeds %d\n", i);
    tcBufferprint((tcBuffer*) rgBuf);
    PrintBytes("Buffer: ", rgBuf, i);
#endif

#ifdef HEADERTEST
    fprintf(g_logFile, "gettcBuf: "); pReq->print();
#endif
    *procid= pReq->m_procid;
    *puReq= pReq->m_reqID;
    *pstatus= pReq->m_ustatus;
    *porigprocid= pReq->m_origprocid;
    i-= sizeof(tcBuffer);
    if(*paramsize<i)
        return false;
    *paramsize= i;
    memcpy(params, &rgBuf[sizeof(tcBuffer)], i);
	fprintf(stdout,"pass by value parameters are set\n");
	fflush(stdout);
    return true;
}


bool tcChannel::sendtcBuf(int procid, u32 uReq, u32 status, int origproc, 
                          int paramsize, byte* params)
{
    byte            rgBuf[PADDEDREQ];
    tcBuffer*       pReq= (tcBuffer*) rgBuf;
    int             i;
    int             n;

#ifdef TEST
    fprintf(g_logFile, "SendtcBuf procid %d, uReq %d, status %d originproc %d, paramsize %d\n", procid, uReq, status, origproc, paramsize);
#endif

    if(paramsize>(PARAMSIZE)) {
        fprintf(g_logFile, "sendtcBuf buffer too small %d\n", paramsize);
        return false;
    }
    pReq->m_procid= procid;
    pReq->m_reqID= uReq;
    pReq->m_ustatus= status;
    pReq->m_origprocid= origproc;
    pReq->m_reqSize= paramsize;

#if 1//def HEADERTEST
    PrintBytes("sendtcBuf: ", (byte*) pReq, sizeof(tcBuffer));
#endif
    n= paramsize+sizeof(tcBuffer);
    memcpy(&rgBuf[sizeof(tcBuffer)], params, paramsize);
    i= WriteBuf(rgBuf, n);
    if(i<0) {
        fprintf(g_logFile, "tcChannel::sendtcBuf: WriteBufRequest failed %d\n", i);
        return false;
    }
    return true;
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

void* handle_session(void* p) {

	
	char buf[PARAMSIZE] = {0};

	const int sz_buf = sizeof(buf);
	int sz_data = 0;
	int err = -1;
	int domid = -1;
	//int fd1 = ps->fd;
	int fd1 = *(int *)p;
	fprintf(g_logFile, "Entered handle_session() with fd1 as %d\n",fd1);
	fprintf(g_logFile, "handle_session(): Client connection from domid %d\n", domid);
	//read domid
	do {
		err = read(fd1, buf+sz_data, 1);
		if (err < 0){			
			goto fail;
		}

		if (err == 0) {
			break;
		}
			
	} while (buf[sz_data++] != '\0');

	
	buf[sz_data] = '\0';
	domid = *(int *)p;
			
	fprintf(g_logFile, "handle_session():inter-domain channel registered for id %d\n", domid);
	
	while(!g_quit)
	{
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
		if (g_quit)
			continue;

		sz_data = err;

#ifdef TEST			
		fprintf(stdout,"handle_session(): XXXX dom_listener received data from domid = %d len= %d \n ", domid, sz_data);
#endif
		
		err = tc_write(domid, (const char*)buf, sz_data);
		fprintf(stdout,"tcwrite written size %d for pid : %d\n",err,domid);
		fflush(stdout);
		if (err < 0)	
			goto fail;

		//write response

		memset(buf, 0, sz_buf);
		
		
		//write response from server
		err = tc_read(domid, (char*)buf, sz_buf);
		fprintf(stdout,"tc_read read data of size %d for pid : %d",err,domid);
		fflush(stdout);
		if (err < 0)	
			goto fail;
		
		sz_data = err;

#ifdef TEST				
		fprintf(g_logFile,"handle_session():YYYY dom_listener sending data to domid = %d len= %d  \n", domid, sz_data);
		printf(" data starts with  : " );
		PrintBytes("Data in tc_read", (byte*)buf, (sz_data > sizeof(tcBuffer))? sizeof(tcBuffer): sz_data);
#endif

		err = ch_write(fd1, buf, sz_data);
		fprintf(stdout,"ch_write wrote data of size %d for domid:%d\n",err,domid);
		if (err < 0) {
			fprintf(g_logFile,"handle_session():dom_listener error data sending %d \n", err);
			goto fail;
		}


#ifdef TEST
		fprintf(stdout,"handle_session():dom_listener SENT data to domid = %d len= %d  \n", domid, err);		
#endif

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
    tcSession* 		cur_session = NULL;
    pthread_t tid;
    pthread_attr_t  attr;
    
    char* ip_env = NULL;

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
				fprintf(g_logFile, "Socket resource may leak to child process..", strerror(errno));
			}
		}else {
				fprintf(g_logFile, "Socket resources may leak to child process", strerror(errno));
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

	setenv("PYTHONPATH", g_python_scripts, 1);
	if (init_pyifc("rppy_ifc") < 0 ) {
		 fprintf(g_logFile, "%s: init_pyifc returned error \n", __FUNCTION__);
		return false;
	}
    return true;
}
