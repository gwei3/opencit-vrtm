//
//  File: tcService.h
//  Description: tcService defines
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


// ------------------------------------------------------------------------------


#ifndef __TCSERVICE_H__
#define __TCSERVICE_H__


//#include "jlmTypes.h"
//#include "tcIO.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <map>

typedef unsigned TCSERVICE_RESULT;


#define MAXREQRESSIZE                         512


#define TCSERVICE_RESULT_SUCCESS                0
#define TCSERVICE_RESULT_UNKNOWNREQ             1
#define TCSERVICE_RESULT_NOREQUESTS             2
#define TCSERVICE_RESULT_CANTOPENOSSERVICE      3
#define TCSERVICE_RESULT_CANTREADOSSERVICE      4
#define TCSERVICE_RESULT_CANTWRITEOSSERVICE     5
#define TCSERVICE_RESULT_CANTALLOCBUFFER        6
#define TCSERVICE_RESULT_DATANOTVALID           7
#define TCSERVICE_RESULT_BUFFERTOOSMALL         8
#define TCSERVICE_RESULT_REQTOOLARGE            9
#define TCSERVICE_RESULT_FAILED                10

#define VM_STATUS_CANCELLED						0
#define VM_STATUS_STARTED						1
#define VM_STATUS_STOPPED						2
#define VM_STATUS_DELETED						3

#define RG_HASH_SIZE		32
#define UUID_SIZE		48
#define IMAGE_ID_SIZE  		256
#define CUSTOMER_ID_SIZE 	256
#define MANIFEST_HASH_SIZE	65
#define MANIFEST_SIGNATURE_SIZE 512
#define LAUNCH_POLICY_SIZE	15
#define MANIFEST_DIR_SIZE	1024

//#include "jlmUtility.h"
#ifndef byte
typedef unsigned char byte;
#endif


extern int g_max_uuid;
extern int g_sz_uuid;
class serviceprocEnt {
public:
    int                 m_procid;
    int                 m_sizeHash;
    int                 m_size_vm_image_id;
    int                 m_size_vm_customer_id;
    int                 m_size_vm_manifest_hash;
    int                 m_size_vm_manifest_signature;
    int                 m_size_vm_launch_policy;
    int                 m_size_vm_manifest_dir;
    byte                m_rgHash[RG_HASH_SIZE];
    char                m_uuid[UUID_SIZE];
    char                m_vdi_uuid[UUID_SIZE];
    char*               m_szexeFile;
    int                 m_nArgs;
    char**              m_Args;

    void                print();
    char                m_vm_image_id[IMAGE_ID_SIZE];
    char                m_vm_customer_id[CUSTOMER_ID_SIZE];
    char                m_vm_manifest_hash[MANIFEST_HASH_SIZE];
    char                m_vm_manifest_signature[MANIFEST_SIGNATURE_SIZE];
    char                m_vm_launch_policy[LAUNCH_POLICY_SIZE];
    bool                m_vm_verfication_status;
    char                m_vm_manifest_dir[MANIFEST_DIR_SIZE];
    int					m_vm_status;
    time_t				m_status_upadation_time;

    serviceprocEnt() : m_rgHash(), m_uuid(), m_vdi_uuid(), m_vm_image_id(), m_vm_customer_id(),
    		m_vm_manifest_hash(), m_vm_manifest_signature(), m_vm_launch_policy(), m_vm_manifest_dir() {
    	
	m_procid = m_sizeHash = m_size_vm_image_id = m_size_vm_customer_id = m_size_vm_manifest_hash =
    			m_size_vm_manifest_signature = m_size_vm_launch_policy = m_size_vm_manifest_dir = m_nArgs = 0;

    	m_Args = NULL;
    	m_szexeFile = NULL;
    	m_vm_verfication_status = false;
    	m_vm_status = VM_STATUS_STOPPED;
    	m_status_upadation_time = time(NULL);
    }
};


// typedef aNode<serviceprocEnt>  serviceprocMap;
typedef std::map<int, serviceprocEnt> proc_table_map;

class serviceprocTable {
public:
	pthread_mutex_t		loc_proc_table;
    proc_table_map proc_table;

    serviceprocTable();
    ~serviceprocTable();

    bool                addprocEntry(int procid, const char* file, int an, char** av,
    			int sizeHash, byte* hash);
    bool 				updateprocEntry(int procid, char* uuid, char* vdi_uuid);
    bool        		updateprocEntry(int procid, char* vm_image_id, char* vm_customer_id, char* vm_manifest_hash, char* vm_manifest_signature,char* launch_policy,bool status, char * vm_manifest_dir);
    bool                removeprocEntry(int procid);
    bool                removeprocEntry(char* uuid);
    serviceprocEnt*     getEntfromprocId(int procid);
    int					getprocIdfromuuid(char* uuid);
    int					getproctablesize();
    int 				getcancelledvmcount();
    void                print();

};


class tcServiceInterface {
public:
    serviceprocTable    m_procTable;
    // This is the lock used by asyncStartApp method
	pthread_mutex_t startAppLock;
	pthread_mutex_t max_thread_lock;
	int maxThread;
	pthread_attr_t pthreadInit;
	bool THREAD_ENABLED;
	int threadCount;
    tcServiceInterface();
    ~tcServiceInterface();

    TCSERVICE_RESULT    initService(const char* execfile, int an, char** av);
    TCSERVICE_RESULT    StartApp( int procid, int an, char** av,
                            int* poutsize, byte* out);
	void 				        printErrorMessagge(int error);
    TCSERVICE_RESULT    UpdateAppID(char* rp_id, char* uuid, char* vdi_uuid, int* psizeOut, byte* rgOut);
    TCSERVICE_RESULT    UpdateAppStatus(char *uuid, int status);
    TCSERVICE_RESULT    TerminateApp(char* uuid, int* psizeOut, byte* rgOut);
    TCSERVICE_RESULT	GetRpId(char *vm_uuid, byte *rp_idbuf, int* bufsize);
    TCSERVICE_RESULT 	GetVmMeta(int procId, byte *vm_imageIdbuf, int * vm_imageIdsize,
    						byte * vm_customerId, int * vm_customerIdsize, byte * vm_manifestHash, int * vm_manifestHashsize,
    						byte * vm_manifestSignature, int * vm_manifestSignaturesize);
    TCSERVICE_RESULT	IsVerified(char *vm_uuid, int* verification_status);
    TCSERVICE_RESULT	GenerateSAMLAndGetDir(char *vm_uuid, char * nonce, char * vm_manifest_dir);
    TCSERVICE_RESULT 	CleanVrtmTable(unsigned long entry_max_age,int vm_status, int* deleted_entries);
};

bool serviceRequest(int procid, u32 uReq, int inparamsize, byte* inparams, int *outparamsize, byte* outparams);
#endif


// ------------------------------------------------------------------------------


