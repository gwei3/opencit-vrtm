//
//  File: tcService.cpp
//  Description: tcService implementation
//
//  Copyright (c) 2012, John Manferdelli.  All rights reserved.
//     Some contributions Copyright (c) 2012, Intel Corporation. 
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


#include "jlmTypes.h"
#include "logging.h"
#include "tcIO.h"
#include "jlmcrypto.h"
#include "tcService.h"
#include "keys.h"
#include "sha256.h"
#include "buffercoding.h"
#include "fileHash.h"
#include "jlmUtility.h"

#include "tao.h"
#include "TPMHostsupport.h"
#include "linuxHostsupport.h"

//#include "vault.h"
#include "cryptoHelper.h"
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
#ifdef LINUX
#include <linux/un.h>
#else
#include <sys/un.h>
#endif
#include <errno.h>


tcServiceInterface      g_myService;
int                     g_servicepid= 0;
extern bool		g_fterminateLoop;
u32                     g_fservicehashValid= false;
u32                     g_servicehashType= 0;
int                     g_servicehashSize= 0;
byte                    g_servicehash[32]= {
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                        };

#define NUMPROCENTS 200


// ---------------------------------------------------------------------------


bool uidfrompid(int pid, int* puid)
{
    char        szfileName[256];
    struct stat fattr;

    sprintf(szfileName, "/proc/%d/stat", pid);
    if((lstat(szfileName, &fattr))!=0) {
        printf("uidfrompid: stat failed\n");
        return false;
    }
    *puid= fattr.st_uid;
    return true;
}


// ------------------------------------------------------------------------------


void serviceprocEnt::print()
{
    fprintf(g_logFile, "procid: %ld, ", (long int)m_procid);
    if(m_szexeFile!=NULL)
        fprintf(g_logFile, "file: %s, ", m_szexeFile);
    fprintf(g_logFile, "hash size: %d, ", m_sizeHash);
    PrintBytes("", m_rgHash, m_sizeHash);
}


serviceprocTable::serviceprocTable()
{
    m_numFree= 0;
    m_numFilled= 0;
    m_pFree= NULL;
    m_pMap= NULL;
    m_rgProcMap= NULL;
    m_rgProcEnts= NULL;
}


serviceprocTable::~serviceprocTable()
{
    // delete m_rgProcEnts;
    // delete m_rgProcMap;
    // m_numFree= 0;
    // m_numFilled= 0;
    // m_pFree= NULL;
    // m_pMap= NULL;
}


bool serviceprocTable::initprocTable(int size)
{
    int             i;
    serviceprocMap* p;

    m_rgProcEnts= new serviceprocEnt[size];
    m_rgProcMap= new serviceprocMap[size];
    p= &m_rgProcMap[0];
    m_pMap= NULL;
    m_pFree= p;
    for(i=0; i<(size-1); i++) {
        p= &m_rgProcMap[i];
        p->pElement= &m_rgProcEnts[i];
        p->pNext= &m_rgProcMap[i+1];
    }
    m_rgProcMap[size-1].pElement= &m_rgProcEnts[size-1];
    m_rgProcMap[size-1].pNext= NULL;
    m_numFree= size;
    m_numFilled= 0;
    return true;
}


bool serviceprocTable::addprocEntry(int procid, const char* file, int an, char** av,
                                    int sizeHash, byte* hash)
{
    if(m_pFree==NULL)
        return false;
    if(sizeHash>32)
        return false;

    serviceprocMap* pMap= m_pFree;
    m_pFree= pMap->pNext;
    serviceprocEnt* pEnt= pMap->pElement;
    m_numFilled++;
    m_numFree--;
    pEnt->m_procid= procid;
    pEnt->m_sizeHash= sizeHash;
    memcpy(pEnt->m_rgHash, hash, sizeHash);
    pEnt->m_szexeFile= strdup(file);
    pMap->pNext= m_pMap;
    m_pMap= pMap;
    return true;
}


serviceprocEnt*  serviceprocTable::getEntfromprocId(int procid)
{
    serviceprocMap* pMap= m_pMap;
    serviceprocEnt* pEnt;
    while(pMap!=NULL) {
        pEnt= pMap->pElement;
        if(pEnt->m_procid==procid) {
            return pEnt;
        }
        pMap= pMap->pNext;
    }
    return NULL;
}


void   serviceprocTable::removeprocEntry(int procid)
{
    serviceprocMap* pMap;
    serviceprocMap* pDelete;
    serviceprocEnt* pEnt;

    if(m_pMap==NULL)
        return;

    pEnt= m_pMap->pElement;
    if(pEnt->m_procid==procid) {
        pMap= m_pMap;
        m_pMap= pMap->pNext;
        pMap->pNext= m_pFree;
        m_pFree= pMap;
        m_numFree++;
        m_numFilled--;
        return;
    }
     
    pMap= m_pMap;   
    while(pMap->pNext!=NULL) {
        pDelete= pMap->pNext;
        pEnt= pDelete->pElement;
        if(pEnt->m_procid==procid) {
            pMap->pNext= pDelete->pNext;
            pDelete->pNext= m_pFree;
            m_pFree= pDelete;
            pEnt->m_procid= -1;
            m_numFree++;
            m_numFilled--;
            return;
        }
        pMap= pDelete;
    }
    return;
}


bool serviceprocTable::gethashfromprocId(int procid, int* psize, byte* hash)
{
    serviceprocEnt* pEnt= getEntfromprocId(procid);

    if(pEnt==NULL)
        return false;

    *psize= pEnt->m_sizeHash;
    memcpy(hash, pEnt->m_rgHash, *psize);
    return true;
}


void serviceprocTable::print()
{
    serviceprocMap* pMap= m_pMap;
    serviceprocEnt* pEnt;

    while(pMap!=NULL) {
        pEnt= pMap->pElement;
        pEnt->print();
        pMap= pMap->pNext;
    }

    fprintf(g_logFile, "proc table %d entries, %d free\n\n", 
                m_numFilled, m_numFree);
    return;
}


// -------------------------------------------------------------------


tcServiceInterface::tcServiceInterface()
{
}


tcServiceInterface::~tcServiceInterface()
{
}


TCSERVICE_RESULT tcServiceInterface::initService(const char* execfile, int an, char** av)
{
    u32     hashType= 0;
    int     sizehash= SHA256DIGESTBYTESIZE;
    byte    rgHash[SHA256DIGESTBYTESIZE];

    if(!getfileHash(execfile, &hashType, &sizehash, rgHash)) {
        fprintf(g_logFile, "initService: getfileHash failed %s\n", execfile);
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TEST
    fprintf(g_logFile, "initService size hash %d\n", sizehash);
    PrintBytes("getfile hash: ", rgHash, sizehash);
#endif

    if(sizehash>SHA256DIGESTBYTESIZE)
        return TCSERVICE_RESULT_FAILED;
    g_servicehashType= hashType;
    g_servicehashSize= sizehash;
    memcpy(g_servicehash, rgHash, sizehash);
    g_fservicehashValid= true;

    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::GetOsPolicyKey(u32* pType, 
                                            int* psize, byte* rgBuf)
{
    if(!m_trustedHome.m_policyKeyValid)
        return TCSERVICE_RESULT_DATANOTVALID ;
    if(*psize<m_trustedHome.m_sizepolicyKey)
        return TCSERVICE_RESULT_BUFFERTOOSMALL;
    memcpy(rgBuf, m_trustedHome.m_policyKey, m_trustedHome.m_sizepolicyKey);
    *pType= m_trustedHome.m_policyKeyType;
    *psize= m_trustedHome.m_sizepolicyKey;

    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::tcServiceInterface::GetOsCert(u32* pType,
                        int* psizeOut, byte* rgOut)
{
    if(!m_trustedHome.m_myCertificateValid)
        return TCSERVICE_RESULT_DATANOTVALID ;
    if(*psizeOut<m_trustedHome.m_myCertificateSize)
        return TCSERVICE_RESULT_BUFFERTOOSMALL;
    memcpy(rgOut, m_trustedHome.m_myCertificate, 
           m_trustedHome.m_myCertificateSize);
    *psizeOut= m_trustedHome.m_myCertificateSize;
    if(m_trustedHome.m_myCertificateType==KEYTYPERSA1024INTERNALSTRUCT)
        *pType= KEYTYPERSA1024SERIALIZED;
    else if(m_trustedHome.m_myCertificateType==KEYTYPERSA2048INTERNALSTRUCT)
        *pType= KEYTYPERSA2048SERIALIZED;
    else
        *pType= m_trustedHome.m_myCertificateType;

    return TCSERVICE_RESULT_SUCCESS;
}



TCSERVICE_RESULT tcServiceInterface::GetOsEvidence(int* psizeOut, byte* rgOut)
{
    if(!m_trustedHome.m_ancestorEvidenceValid)
        return TCSERVICE_RESULT_DATANOTVALID ;
    if(*psizeOut<m_trustedHome.m_ancestorEvidenceSize)
        return TCSERVICE_RESULT_BUFFERTOOSMALL;
    *psizeOut= m_trustedHome.m_ancestorEvidenceSize;
    memcpy(rgOut, m_trustedHome.m_ancestorEvidence, *psizeOut);

    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::GetHostedMeasurement(int pid, u32* phashType, 
                int* psize, byte* rgBuf)
{
    if(!m_procTable.gethashfromprocId(pid, psize, rgBuf)) {
        return TCSERVICE_RESULT_FAILED;
    }
    *phashType= HASHTYPEJLMPROGRAM;
    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::GetOsHash(u32* phashType, 
                                        int* psize, byte* rgOut)
{
    if(!m_trustedHome.m_myMeasurementValid)
        return TCSERVICE_RESULT_DATANOTVALID ;
    if(*psize<m_trustedHome.m_myMeasurementSize)
        return TCSERVICE_RESULT_BUFFERTOOSMALL;
    *psize= m_trustedHome.m_myMeasurementSize;
    memcpy(rgOut, m_trustedHome.m_myMeasurement, *psize);
    *phashType= m_trustedHome.m_myMeasurementType;

    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::GetServiceHash(u32* phashType, 
                    int* psize, byte* rgOut)
{
    if(!g_fservicehashValid)
        return TCSERVICE_RESULT_FAILED;
    *phashType= g_servicehashType;
    if(*psize<g_servicehashSize)
        return TCSERVICE_RESULT_FAILED;
    *psize= g_servicehashSize;
    memcpy(rgOut, g_servicehash, *psize);

    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::StartApp(tcChannel& chan,
                                int procid, const char* file, int an, char** av, 
                                int* poutsize, byte* out)
{
    u32     uType= 0;
    int     size= SHA256DIGESTBYTESIZE;
    byte    rgHash[SHA256DIGESTBYTESIZE];
    int     child= 0;
    int     i;
    int     newan= an;
    char*   newav[32];
    int     uid= -1;

#ifdef TEST
    fprintf(g_logFile, "tcServiceInterface::StartApp(%s) %d args\n", file, an);
    for(i=0;i<an;i++)
       fprintf(g_logFile, "\tav[%d]: %s\n", i, av[i]);
#endif

    if(an>30) {
        return TCSERVICE_RESULT_FAILED;
    }
    newav[0]= strdup(file);
    for(i=1;i<newan;i++)
        newav[i]= strdup(av[i]);
    newav[newan]= NULL;

#ifdef TEST
    fprintf(g_logFile, "StartApp file %s %d arguments\n", file, newan);
#endif

    // lock file
#ifdef LOCKFILE
    struct flock lock;
    int     ret;
    int     fd= open(file, O_RDONLY);

    // F_UNLCK
    lock.l_type= F_WRLCK;
    lock.l_start= 0;
    lock.l_len= SEEK_SET;
    lock.l_pid= getpid();
    ret= fcntl(fd, F_SETLK, &lock);
#endif

    if(!getfileHash(file, &uType, &size, rgHash)) {
        fprintf(g_logFile, "StartApp : getfilehash failed %s\n", file);
        return TCSERVICE_RESULT_FAILED;
    }

    child= fork();
    if(child<0) {
        fprintf(g_logFile, "StartApp: fork failed\n");
        return TCSERVICE_RESULT_FAILED;
    }
    if(child==0) {
        chan.CloseBuf();
    }

    if(child>0) {
        // look up uid for procid
        if(!uidfrompid(procid, &uid)) {
            fprintf(g_logFile, "StartApp: cant get uid from procid\n");
            return TCSERVICE_RESULT_FAILED;
	}

        // setuid to correct user (uid, eid and saved id)
        setresuid(uid, uid, uid);

        // record procid and hash
        if(!g_myService.m_procTable.addprocEntry(child, file, 0, (char**) NULL, 
                                                 size, rgHash)) {
            fprintf(g_logFile, "StartApp: cant add to proc table\n");
            return TCSERVICE_RESULT_FAILED;
        }
#ifdef TCTEST
        fprintf(g_logFile, "\nProc table after create. child: %d, serviceid: %d\n", 
                child, g_servicepid);
        g_myService.m_procTable.print();
#endif
#ifdef LOCKFILE
        close(fd);
#endif
    }

    // child
    if(child==0) {
#ifdef TEST1
        fprintf(g_logFile, "StartApp about to exec %d args\n", newan);
#endif
#ifdef LOCKFILE
        // drop lock
        // this actually drops it a bit too soon
        // we can leave it locked or change owner or copy it somewhere
        lock.l_type= F_UNLCK;
        lock.l_start= 0;
        lock.l_len= SEEK_SET;
        lock.l_pid= getpid();   // is this right?
        ret= fcntl(fd, F_SETLK, &lock);
        close(fd);
#endif
        if(execve(file, newav, NULL)<0) {
            fprintf(g_logFile, "StartApp: execvp %s failed\n", file);
        }
    }

    *poutsize= sizeof(int);
    *((int*)out)= child;
    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::SealFor(int procid, int sizeIn, byte* rgIn, 
                                             int* psizeOut, byte* rgOut)
//  Sealed value is hash-size hash size-in rgIn
{
    byte    rgHash[32];
    int     hashSize= 0;

    if(!m_procTable.gethashfromprocId(procid, &hashSize, rgHash)) {
        fprintf(g_logFile, "SealFor can't find hash in procTable %ld\n", (long int)procid);
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TCTEST
    fprintf(g_logFile, "SealFor: %ld(proc), %d(hashsize), %d (size seal)\n",
           (long int)procid, hashSize, sizeIn);
#endif
    if(!m_trustedHome.Seal(hashSize, rgHash, sizeIn, rgIn,
                       psizeOut, rgOut)) {
        fprintf(g_logFile, "SealFor: seal failed\n");
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TCTEST
    fprintf(g_logFile, "tcServiceInterface::SealFor\n");
#endif
    
    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::UnsealFor(int procid, int sizeIn, byte* rgIn, 
                            int* psizeOut, byte* rgOut)
{
    byte    rgHash[32];
    int     hashSize= 0;

    if(!m_procTable.gethashfromprocId(procid, &hashSize, rgHash)) {
        fprintf(g_logFile, "UnsealFor can't find hash in procTable %ld\n", (long int)procid);
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TCTEST
    fprintf(g_logFile, "UnsealFor: %ld(proc), %d(hashsize), %d (size seal)\n",
           (long int)procid, hashSize, sizeIn);
#endif
    if(!m_trustedHome.Unseal(hashSize, rgHash, sizeIn, rgIn,
                       psizeOut, rgOut)) {
        fprintf(g_logFile, "UnsealFor: unseal failed\n");
        return TCSERVICE_RESULT_FAILED;
    }
    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::AttestFor(int procid, int sizeIn, byte* rgIn, 
                            int* psizeOut, byte* rgOut)
{
    byte    rgHash[32];
    int     hashSize= 32;

    if(!m_procTable.gethashfromprocId(procid, &hashSize, rgHash)) {
#ifdef TCTEST
        fprintf(g_logFile, "tcServiceInterface::AttestFor lookup failed\n");
        m_procTable.print();
#endif
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TEST
    fprintf(g_logFile, "tcServiceInterface::AttestFor procid: %d\n", 
            procid);
#endif
    if(!m_trustedHome.Attest(hashSize, rgHash, sizeIn, rgIn,
                       psizeOut, rgOut)) {
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TEST
        fprintf(g_logFile, "tcServiceInterface::AttestFor new output buf size\n",
               *psizeOut);
#endif
    return TCSERVICE_RESULT_SUCCESS;
}


// ------------------------------------------------------------------------------


bool  serviceRequest(tcChannel& chan, bool* pfTerminate)
{
    int                 procid;
    int                 origprocid;
    u32                 uReq;
    u32                 uStatus;

    char*               szappexecfile= NULL;

    int                 sizehash= SHA256DIGESTBYTESIZE;
    byte                hash[SHA256DIGESTBYTESIZE];

    int                 inparamsize;
    byte                inparams[PARAMSIZE];

    int                 outparamsize;
    byte                outparams[PARAMSIZE];

    int                 size;
    byte                rgBuf[PARAMSIZE];

    int                 pid;
    u32                 uType= 0;
    int                 an;
    char*               av[10];

#ifdef TEST
    fprintf(g_logFile, "Entering serviceRequest\n");
#endif

    // get request
    inparamsize= PARAMSIZE;
    if(!chan.gettcBuf(&procid, &uReq, &uStatus, &origprocid, &inparamsize, inparams)) {
        fprintf(g_logFile, "serviceRequest: gettcBuf failed\n");
        return false;
    }
    if(uStatus==TCIOFAILED) {
        chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
        return false;
    }

#ifdef TEST
    fprintf(g_logFile, "serviceRequest after get procid: %d, req, %d, origprocid %d\n", 
           procid, uReq, origprocid); 
#endif

    switch(uReq) {

      case TCSERVICEGETPOLICYKEYFROMTCSERVICE:
        size= PARAMSIZE;
        if(g_myService.GetOsPolicyKey(&uType, &size, rgBuf)!=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: getpolicyKey failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }

        outparamsize= encodeTCSERVICEGETPOLICYKEYFROMOS(uType, size, rgBuf, 
                                      PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: TCSERVICEGETPOLICYKEYFROMTCSERVICE buffer too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (policyKey) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICEGETOSHASHFROMTCSERVICE:
        size= PARAMSIZE;
        if(g_myService.GetOsHash(&uType, &size, rgBuf)!=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: getosHash failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
#ifdef TEST
       fprintf(g_logFile, "serviceRequest: TCSERVICEGETOSHASHFROMTCSERVICE type %d, size %d\n",
        uType, size);
        PrintBytes("OsHash in tc service ", rgBuf, size);
#endif
        outparamsize= encodeTCSERVICEGETOSHASHFROMTCSERVICE(uType, size, rgBuf, 
                                      PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: encodeTCSERVICEGETOSHASHFROMTCSERVICE buffer too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (getosHash) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICEGETOSCREDSFROMTCSERVICE:
        size= PARAMSIZE;
        if(g_myService.GetOsEvidence(&size, rgBuf)!=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: getosCredsfailed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        outparamsize= encodeTCSERVICEGETOSCREDSFROMTCSERVICE(uType, size, rgBuf, 
                                      PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: encodeTCSERVICEGETOSCREDSFROMTCSERVICE buffer too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)){
            fprintf(g_logFile, "serviceRequest: sendtcBuf (getosCreds) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICESEALFORFROMTCSERVICE:
        // Input buffer to decode:
        //  size of sealdata || sealedata
        //  decodeTCSERVICESEALFORFROMAPP outputs
        //  size of sealdata, sealedata
        //  sealfor returns m= ENC(hashsize||hash||sealsize||sealdata)
        //  returned buffer is sizeof(m) || m
        outparamsize= PARAMSIZE;
#ifdef TCTEST
        fprintf(g_logFile, "TCSERVICESEALFORFROMTCSERVICE********about to sealFor\n");
        PrintBytes("inparams: ", inparams, inparamsize);
#endif
        if(!decodeTCSERVICESEALFORFROMAPP(&outparamsize, outparams, inparams)) {
            fprintf(g_logFile, "serviceRequest: TCSERVICESEALFORFROMTCSERVICE buffer too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        size= PARAMSIZE;
#ifdef TCTEST1
        fprintf(g_logFile, "about to sealFor %d\n", outparamsize);
        PrintBytes("bytes to seal: ", outparams, outparamsize);
#endif
        if(g_myService.SealFor(origprocid, outparamsize, outparams, &size, rgBuf)
                !=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: sealFor failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        outparamsize= encodeTCSERVICESEALFORFROMTCSERVICE(size, rgBuf, PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: encodeTCSERVICESEALFORFROMTCSERVICE buf too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (sealFor) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICEUNSEALFORFROMTCSERVICE:
        // outparamsize is sizeof(m)m outparams is m from above
        // unsealfor returns sizof unsealed data || unsealeddata
#ifdef TCTEST1
        fprintf(g_logFile, "TCSERVICEUNSEALFORFROMTCSERVICE********about to UnsealFor\n");
#endif
        outparamsize= PARAMSIZE;
        if(!decodeTCSERVICEUNSEALFORFROMAPP(&outparamsize, outparams, inparams)) {
            fprintf(g_logFile, "serviceRequest: service loop TCSERVICEUNSEALFORFROMTCSERVICE failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        size= PARAMSIZE;
#ifdef TCTEST
        fprintf(g_logFile, "about to UnsealFor %d\n", outparamsize);
#endif
        if(g_myService.UnsealFor(origprocid, outparamsize, outparams, &size, rgBuf)
                !=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: UnsealFor failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
#ifdef TCTEST1
        PrintBytes("return from UnsealFor:\n", rgBuf, size);
#endif
        outparamsize= encodeTCSERVICEUNSEALFORFROMAPP(size, rgBuf, PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: encodeTCSERVICESEALFORFROMTCSERVICE buf too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (unsealFor) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICEGETPROGHASHFROMTCSERVICE:
        if(!decodeTCSERVICEGETPROGHASHFROMAPP(&pid, inparams)) {
            fprintf(g_logFile, "serviceRequest: TCSERVICEGETPROGHASHFROMTCSERVICE failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }

        // Process
#ifdef TEST
        fprintf(g_logFile, "looking up hash for pid %d\n", pid);
        g_myService.m_procTable.print();
        fflush(g_logFile);
#endif
        sizehash= SHA256DIGESTBYTESIZE;
        uType= SHA256HASH;
        if(!g_myService.m_procTable.gethashfromprocId(pid, &sizehash, hash)) {
#ifdef TEST
            fprintf(g_logFile, "hash not found setting to 0\n");
#endif
            memset(hash, 0, sizehash);
        }
#ifdef TEST
        fprintf(g_logFile, "program hash for pid found\n");
        PrintBytes("Hash: ", hash, sizehash);
        fflush(g_logFile);
#endif
        outparamsize= encodeTCSERVICEGETPROGHASHFROMSERVICE(uType, sizehash, 
                                hash, PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: encodeTCSERVICEGETPROGHASHFROMSERVICE buf too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (getproghash) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICEATTESTFORFROMTCSERVICE:
        outparamsize= PARAMSIZE;
        if(!decodeTCSERVICEATTESTFORFROMAPP(&outparamsize, outparams, inparams)) {
            fprintf(g_logFile, "serviceRequest: TCSERVICEATTESTFORFROMTCSERVICE failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        size= PARAMSIZE;
        if(g_myService.AttestFor(origprocid, outparamsize, outparams, &size, rgBuf)
                !=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: AttestFor failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        outparamsize= encodeTCSERVICEATTESTFORFROMAPP(size, rgBuf, PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: encodeTCSERVICEATTESTFORFROMAPP buf too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (AttestFor) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICESTARTAPPFROMTCSERVICE:
#ifdef TCTEST1
        fprintf(g_logFile, "serviceRequest, TCSERVICESTARTAPPFROMTCSERVICE, decoding\n");
#endif
        an= 10;
        if(!decodeTCSERVICESTARTAPPFROMAPP(&szappexecfile, &an, 
                    (char**) av, inparams)) {
            fprintf(g_logFile, "serviceRequest: decodeTCSERVICESTARTAPPFROMTCSERVICE failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        outparamsize= PARAMSIZE;
#ifdef TEST
        fprintf(g_logFile, "serviceRequest, about to StartHostedProgram %s, for %d\n",
                szappexecfile, origprocid);
#endif
        if(g_myService.StartApp(chan, origprocid, szappexecfile, an, av,
                                    &outparamsize, outparams)
                !=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: StartHostedProgram failed %s\n", szappexecfile);
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
#ifdef TEST
        fprintf(g_logFile, "serviceRequest, StartHostedProgram succeeded, about to send\n");
#endif
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (startapp) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICETERMINATE:  // no reply required
#ifdef TEST
        fprintf(g_logFile, "serviceRequest, TCSERVICETERMINATE\n");
#endif
        g_myService.m_procTable.removeprocEntry(origprocid);
#ifdef TEST
        fprintf(g_logFile, "serviceRequest, removeprocEntry %d\n", origprocid);
        g_myService.m_procTable.print();
#endif
        return true;

      default:
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
        return false;
    }
}


// ------------------------------------------------------------------------------


// Remove this

bool             g_globalpolicyValid= false;
//metaData         g_theVault;
//PrincipalCert*   g_policyPrincipalCert= NULL;
RSAKey*          g_policyKey= NULL;
//accessPrincipal* g_policyAccessPrincipal= NULL;


int main(int an, char** av)
{
    int                 iRet= 0;
    TCSERVICE_RESULT    ret;
    bool                fInitKeys= false;
    const char*               szexecFile= "./tcService.exe";

    int                 i;
    bool                fTerminate= false;
    bool                fServiceStart;
    const char*               directory= NULL;

    initLog("tcService.log");
#ifdef TEST
    fprintf(g_logFile, "tcService started\n\n");
#endif

    for(i=0; i<an; i++) {
        if(strcmp(av[i], "-help")==0) {
            fprintf(g_logFile, "\nUsage: tcService.exe [-initKeys] ");
            return 0;
        }
        if(strcmp(av[i], "-initKeys")==0) {
            fInitKeys= true;
        }
        if(strcmp(av[i], "-directory")==0) {
            directory= av[++i];
        }
    }

    // set the signal disposition of SIGCHLD to not create zombies
    struct sigaction sigAct;
    memset(&sigAct, 0, sizeof(sigAct));
    sigAct.sa_handler = SIG_DFL;
    sigAct.sa_flags = SA_NOCLDWAIT; // don't zombify child processes
    int sigRv = sigaction(SIGCHLD, &sigAct, NULL);
    if (sigRv < 0) {
        fprintf(g_logFile, "Failed to set signal disposition for SIGCHLD\n");
    } else {
        fprintf(g_logFile, "Set SIGCHLD to avoid zombies\n");
    }

    g_servicepid= getpid();
    const char** parameters = NULL;
    int parameterCount = 0;
    if(directory==NULL) {
        directory= DEFAULTDIRECTORY;
    } else {
        parameters = &directory;
	parameterCount = 1;
    }

    if(!initAllCrypto()) {
        fprintf(g_logFile, "tcService main: can't initcrypto\n");
        iRet= 1;
        goto cleanup;
    }

    // init Host and Environment
    g_myService.m_taoHostInitializationTimer.Start();
    if(!g_myService.m_host.HostInit(PLATFORMTYPEHW, parameterCount, parameters)) {
        fprintf(g_logFile, "tcService main: can't init host\n");
        iRet= 1;
        goto cleanup;
    }
    g_myService.m_taoHostInitializationTimer.Stop();

#ifdef TEST
    fprintf(g_logFile, "tcService main: after HostInit, pid: %d\n",
            g_servicepid);
    g_myService.m_host.printData();
#endif

    if(fInitKeys) {
        taoFiles  fileNames;

        if(!fileNames.initNames(directory, "TrustedOS")) {
            fprintf(g_logFile, "tcService::main: cant init names\n");
            iRet= 1;
            goto cleanup;
        }
        unlink(fileNames.m_szsymFile);
        unlink(fileNames.m_szprivateFile);
        unlink(fileNames.m_szcertFile);
        unlink(fileNames.m_szAncestorEvidence);
    }

    g_myService.m_taoEnvInitializationTimer.Start();
    if(!g_myService.m_trustedHome.EnvInit(PLATFORMTYPELINUX, "TrustedOS",
                                DOMAIN, directory, 
                                &g_myService.m_host, 0, NULL)) {
        fprintf(g_logFile, "tcService main: can't init environment\n");
        iRet= 1;
        goto cleanup;
    }
    g_myService.m_taoEnvInitializationTimer.Stop();

#ifdef TEST
    fprintf(g_logFile, "tcService main: after EnvInit\n");
    g_myService.m_trustedHome.printData();
#endif

    if(fInitKeys) {
        // EnvInit should have initialized keys
        iRet= 0;
        goto cleanup;
    }

    if(!g_myService.m_procTable.initprocTable(NUMPROCENTS)) {
        fprintf(g_logFile, "tcService main: Cant init proctable\n");
        iRet= 1;
        goto cleanup;
    }
#ifdef TEST
    fprintf(g_logFile, "tcService main: proctable init complete\n\n");
#endif

    ret= g_myService.initService(szexecFile, 0, NULL);
    if(ret!=TCSERVICE_RESULT_SUCCESS) {
        fprintf(g_logFile, "tcService main: initService failed %s\n", szexecFile);
        iRet= 1;
        goto cleanup;
    }
#ifdef TEST
    fprintf(g_logFile, "tcService main: initService succeeds\n\n");
#endif

    // add self proctable entry
    g_myService.m_procTable.addprocEntry(g_servicepid, strdup(szexecFile), 0, NULL,
                                      g_myService.m_trustedHome.m_myMeasurementSize,
                                      g_myService.m_trustedHome.m_myMeasurement);
   
#ifdef TEST
    fprintf(g_logFile, "\ntcService main: initprocEntry succeeds\n");
#endif

    while(!g_fterminateLoop) {
        fServiceStart= serviceRequest(g_reqChannel, &fTerminate);
#ifdef TEST
        if(fServiceStart)
            fprintf(g_logFile, "tcService main: successful service\n\n");
        else
            fprintf(g_logFile, "tcService main: unsuccessful service\n\n");
#else 
        UNUSEDVAR(fServiceStart);
#endif
    }

#ifdef TEST
    fprintf(g_logFile, "tcService main: tcService ending\n");
#endif

cleanup:
#ifdef TEST
    if(iRet!=0)
        fprintf(g_logFile, "tcService returns with error\n");
    else
        fprintf(g_logFile, "tcService returns successful\n");
#endif
    g_myService.m_trustedHome.EnvClose();
    g_myService.m_host.HostClose();
    closeLog();
    return iRet;
}


// ------------------------------------------------------------------------------


