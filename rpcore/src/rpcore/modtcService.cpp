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
#include "modtcService.h"
#include "algs.h"
#include "channelcoding.h"
#include "linuxHostsupport.h"

#include <stdlib.h>
#include <sys/wait.h> /* for wait */
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

#include "tcconfig.h"
#include "tcpchan.h"
#include "rp_api_code.h"

#include <libxml/xmlreader.h>
#include <map>

tcServiceInterface      g_myService;
int                     g_servicepid= 0;
extern bool				g_fterminateLoop;
u32                     g_fservicehashValid= false;
u32                     g_servicehashType= 0;
int                     g_servicehashSize= 0;
byte                    g_servicehash[32]= {
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                        };

uint32_t	g_rpdomid = 1000;
char 		g_mtwproxy_ip[64] = "127.0.0.1";
int 		g_mtwproxy_on = 0;
int 		g_mtwproxy_port = 16006;
#define NUMPROCENTS 200
#define LAUNCH_ALLOWED		"launch allowed"	 
#define LAUNCH_NOT_ALLOWED	"launch not allowed"
#define KEYWORD_UNTRUSTED	"untrusted"





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
    loc_proc_table = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
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


/*bool serviceprocTable::initprocTable(int size)
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
*/

bool serviceprocTable::addprocEntry(int procid, const char* file, int an, char** av,
                                    int sizeHash, byte* hash)
{
   //printf("Entering in addprocEntry");
    if(sizeHash>32)
        return false;
    pthread_mutex_lock(&loc_proc_table);
    serviceprocEnt proc_ent;
    //proc_ent.m_procid = procid;
    proc_ent.m_szexeFile = strdup(file);
    proc_ent.m_sizeHash = sizeHash;
    memcpy(proc_ent.m_rgHash,hash,sizeHash);
    proc_table.insert(std::pair<int, serviceprocEnt>(procid, proc_ent));
    fprintf(g_logFile,"Entry added for procid %d\n",procid);
    pthread_mutex_unlock(&loc_proc_table);
    return true;
}


/*serviceprocEnt*  serviceprocTable::getEntfromprocId(int procid)
{
    serviceprocMap* pMap= m_pMap;
    serviceprocEnt* pEnt;
   //printf("\n inside getEntfromprocId");
    pthread_mutex_lock(&loc_proc_table);
    while(pMap!=NULL) {
        pEnt= pMap->pElement;
	printf("\n in while loop");
        if(pEnt->m_procid==procid) {
	   //printf("\n procid match");
        	pthread_mutex_unlock(&loc_proc_table);
            return pEnt;
        }
        pMap= pMap->pNext;
    }
   //printf("\n NOT FOUND");
    pthread_mutex_unlock(&loc_proc_table);
    return NULL;
}*/


//Step through linked list m_pMap and delete node matching procid
void   serviceprocTable::removeprocEntry(int procid)
{
	pthread_mutex_lock(&loc_proc_table);
	proc_table_map::iterator table_it = proc_table.find(procid);
	if( table_it == proc_table.end()) {
		fprintf(g_logFile,"Table entry can't be removed, given RPID : %d doesn't exist\n", procid);
		pthread_mutex_unlock(&loc_proc_table);
		return;
	}
	proc_table.erase(table_it);
	fprintf(g_logFile,"Table entry removed successfully for RPID : %d\n",procid);
	pthread_mutex_unlock(&loc_proc_table);
	return;
}

/*bool  serviceprocTable::getEntfromuuid(char* uuid)
{
	pthread_mutex_lock(&loc_proc_table);
    serviceprocMap* pMap= m_pMap;
    serviceprocEnt* pEnt;
    while(pMap!=NULL) {
        pEnt= pMap->pElement;
        if( strcasecmp(pEnt->m_uuid, (char*) uuid) == 0 ) {
            return true;
        }
        pMap= pMap->pNext;
    }
    pthread_mutex_unlock(&loc_proc_table);
    return false;
}*/

//Step through linked list m_pMap and delete node matching uuid
void   serviceprocTable::removeprocEntry(char* uuid)
{
	int proc_id = getprocIdfromuuid(uuid);
	if ( proc_id == NULL ) {
		fprintf(g_logFile,"Entry removal failed from Table, UUID is not registered with vRTM\n");
		return;
	}
	pthread_mutex_lock(&loc_proc_table);
	proc_table_map::iterator table_it = proc_table.find(proc_id);
	if(table_it == proc_table.end()) {
		fprintf(g_logFile,"Entry removal failed from Table, UUID is not registered with vRTM\n");
		pthread_mutex_unlock(&loc_proc_table);
		return;
	}
	proc_table.erase(table_it);
	fprintf(g_logFile,"Entry removed from Table for UUID : %s\n", uuid);
	pthread_mutex_unlock(&loc_proc_table);
	return;
}

bool serviceprocTable::updateprocEntry(int procid, char* uuid, char *vdi_uuid)
{
   //printf("Inside updateprocEntry");
    pthread_mutex_lock(&loc_proc_table);
    proc_table_map::iterator table_it = proc_table.find(procid);
	if (table_it == proc_table.end()) {
		fprintf(g_logFile,"UUID can't be registered with vRTM, given rpid doesn't exist\n");
		pthread_mutex_unlock(&loc_proc_table);
		return false;
	}
    memset(table_it->second.m_uuid, 0, g_max_uuid);
    memset(table_it->second.m_vdi_uuid, 0, g_max_uuid);
    memcpy(table_it->second.m_uuid, uuid, g_sz_uuid);
    memcpy(table_it->second.m_vdi_uuid, vdi_uuid, g_sz_uuid);
    fprintf(g_logFile,"UUID : %s is registered with vRTM successfully\n",table_it->second.m_uuid);
    pthread_mutex_unlock(&loc_proc_table);
    return true;
}

bool serviceprocTable::updateprocEntry(int procid, char* vm_image_id, char* vm_customer_id, char* vm_manifest_hash,
									char* vm_manifest_signature, char *launch_policy, bool verification_status, char * vm_manifest_dir) {
    //geting the procentry related to this procid

	pthread_mutex_lock(&loc_proc_table);
	proc_table_map::iterator table_it = proc_table.find(procid);
	if( table_it == proc_table.end()) {
		fprintf(g_logFile,"Couldn't update the given data in table, rpid doesn't exist\n");
		pthread_mutex_unlock(&loc_proc_table);
		return false;
	}
	strcpy(table_it->second.m_vm_image_id,vm_image_id);
	table_it->second.m_size_vm_image_id = strlen(table_it->second.m_vm_image_id);
	strcpy(table_it->second.m_vm_customer_id, vm_customer_id);
	table_it->second.m_size_vm_customer_id = strlen(table_it->second.m_vm_customer_id);
	strcpy(table_it->second.m_vm_manifest_signature, vm_manifest_signature);
	table_it->second.m_size_vm_manifest_signature = strlen(table_it->second.m_vm_manifest_signature);
	strcpy(table_it->second.m_vm_manifest_hash, vm_manifest_hash);
	table_it->second.m_size_vm_manifest_hash = strlen(table_it->second.m_vm_manifest_signature);
	strcpy(table_it->second.m_vm_manifest_dir, vm_manifest_dir);
	table_it->second.m_size_vm_manifest_dir = strlen(table_it->second.m_vm_manifest_dir);
	strcpy(table_it->second.m_vm_launch_policy, launch_policy);
	table_it->second.m_size_vm_launch_policy = strlen(table_it->second.m_vm_launch_policy);
	table_it->second.m_vm_verfication_status = verification_status;
	fprintf(g_logFile,"Data updated against RPID : %d in the Table\n", procid);
	pthread_mutex_unlock(&loc_proc_table);
    return true;
}

serviceprocEnt* serviceprocTable::getEntfromprocId(int procid) {
	pthread_mutex_lock(&loc_proc_table);
	proc_table_map::iterator table_it = proc_table.find(procid);
	if(table_it == proc_table.end()) {
		pthread_mutex_unlock(&loc_proc_table);
		return NULL;
	}
	pthread_mutex_unlock(&loc_proc_table);
	return &(table_it->second);
}

int serviceprocTable::getprocIdfromuuid(char* uuid) {
	pthread_mutex_lock(&loc_proc_table);
	for (proc_table_map::iterator table_it = proc_table.begin(); table_it != proc_table.end() ; table_it++ ) {
		if(strcmp(table_it->second.m_uuid,uuid) == 0 ) {
			pthread_mutex_unlock(&loc_proc_table);
			return (table_it->first);
		}
	}
	pthread_mutex_unlock(&loc_proc_table);
	return NULL;
}

/*bool serviceprocTable::checkprocEntry(char* uuid, char *vdi_uuid)
{
   //printf("Inside checkprocEntry");
	pthread_mutex_lock(&loc_proc_table);
    serviceprocMap* pMap= m_pMap;
    serviceprocEnt* pEnt;
    while(pMap!=NULL) {
        pEnt= pMap->pElement;
	if(strcasecmp(pEnt->m_vdi_uuid, (char*) vdi_uuid) == 0){
		if(strcasecmp(pEnt->m_uuid, (char*) uuid) == 0){
			printf("Same");
			pthread_mutex_unlock(&loc_proc_table);
			return true;
			
		}
		printf("diffenret");
		pthread_mutex_unlock(&loc_proc_table);
		return false;
	}
        pMap= pMap->pNext;
    }
    pthread_mutex_unlock(&loc_proc_table);
    return true;
}*/


/*bool serviceprocTable::gethashfromprocId(int procid, int* psize, byte* hash)
{
    serviceprocEnt* pEnt= getEntfromprocId(procid);

    if(pEnt==NULL)
        return false;
    pthread_mutex_lock(&loc_proc_table);
    *psize= pEnt->m_sizeHash;
    memcpy(hash, pEnt->m_rgHash, *psize);
    pthread_mutex_unlock(&loc_proc_table);
    return true;
}*/


void serviceprocTable::print()
{
	pthread_mutex_lock(&loc_proc_table);
	for(proc_table_map::iterator table_it = proc_table.begin(); table_it != proc_table.end() ; table_it++ ) {
		table_it->second.print();
	}
	fprintf(g_logFile,"Table has %d entries\n",proc_table.size());
    pthread_mutex_unlock(&loc_proc_table);
    return;
}


// -------------------------------------------------------------------
void tcServiceInterface::printErrorMessagge(int error) {
        if ( error == EAGAIN ){
                fprintf(g_logFile, "Insufficient resources to create another thread");
        }
        if ( error == EINVAL) {
                fprintf(g_logFile, "Invalid settings in pthreadInit");
        }
        if ( error == EPERM ) {
                fprintf(g_logFile, "No permission to set the scheduling policy and parameters specified in pthreadInit");
        }
}

tcServiceInterface::tcServiceInterface()
{
	int error;
	startAppLock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	max_thread_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	maxThread  = g_max_thread_limit;
	threadCount = 0;
	if ( (error = pthread_attr_init (&pthreadInit)) ) {
		errno = error;
		fprintf(g_logFile, "Asynchronous measured launch will not work %s", strerror(errno) );
		THREAD_ENABLED = false;
		return;
	}
	THREAD_ENABLED = true;
	// if you want result back from thread replace this line
	pthread_attr_setdetachstate (&pthreadInit, PTHREAD_CREATE_DETACHED);
}


tcServiceInterface::~tcServiceInterface()
{
	fprintf(stdout,"*************************************destructor called");
	fflush(stdout);
	pthread_attr_destroy (&pthreadInit);
}


TCSERVICE_RESULT tcServiceInterface::initService(const char* execfile, int an, char** av)
{
#if 0
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
#endif

    return TCSERVICE_RESULT_SUCCESS;
}

// *****************return the procId (rpid) for given uuid***************

TCSERVICE_RESULT tcServiceInterface::GetRpId(char *vm_uuid, byte * rpidbuf, int * rpidsize)
{
    fprintf(g_logFile,"In GetRpId function\n");
    int proc_id = m_procTable.getprocIdfromuuid(vm_uuid);
    if(proc_id == NULL)
	{
		fprintf(g_logFile,"match not found for Given UUID\n");
		return TCSERVICE_RESULT_FAILED;
	}

    fprintf(g_logFile,"match found for Given UUID\n");
	sprintf((char *)rpidbuf,"%d",proc_id);
	*rpidsize = strlen((char *)rpidbuf);
	return TCSERVICE_RESULT_SUCCESS;
}

//*************************return vmeta for given procid(rpid)*************/

TCSERVICE_RESULT tcServiceInterface::GetVmMeta(int procId, byte *vm_imageId, int * vm_imageIdsize,
    						byte * vm_customerId, int * vm_customerIdsize, byte * vm_manifestHash, int * vm_manifestHashsize,
    						byte * vm_manifestSignature, int * vm_manifestSignaturesize)
{
    fprintf(g_logFile,"In function GetVmMeta\n");
    serviceprocEnt *pEnt = m_procTable.getEntfromprocId(procId);
    if(pEnt == NULL ) {
    	fprintf(g_logFile,"Given RPID is not registered to RPCORE\n");
    	return TCSERVICE_RESULT_FAILED;
    }
	fprintf(g_logFile,"Match found for given RPid\n");
	fprintf(g_logFile,"vmimage id : %s\n",pEnt->m_vm_image_id);
	memcpy(vm_imageId,pEnt->m_vm_image_id,pEnt->m_size_vm_image_id + 1);
	fprintf(g_logFile,"vmimage id copied : %s\n",vm_imageId);
	*vm_imageIdsize = pEnt->m_size_vm_image_id ;
	memcpy(vm_customerId,pEnt->m_vm_customer_id,pEnt->m_size_vm_customer_id + 1);
	*vm_customerIdsize = pEnt->m_size_vm_customer_id ;
	memcpy(vm_manifestHash,pEnt->m_vm_manifest_hash, pEnt->m_size_vm_manifest_hash + 1);
	*vm_manifestHashsize = pEnt->m_size_vm_manifest_hash ;
	memcpy(vm_manifestSignature,pEnt->m_vm_manifest_signature,pEnt->m_size_vm_manifest_signature + 1);
	*vm_manifestSignaturesize = pEnt->m_size_vm_manifest_signature ;
	return TCSERVICE_RESULT_SUCCESS;
}

/****************************return verification status of vm with attestation policy*********************** */
TCSERVICE_RESULT tcServiceInterface::IsVerified(char *vm_uuid, int* verification_status)
{
	fprintf(g_logFile,"\nIn function IsVerified\n");
	pthread_mutex_lock(&m_procTable.loc_proc_table);
	for (proc_table_map::iterator table_it = m_procTable.proc_table.begin(); table_it != m_procTable.proc_table.end() ; table_it++ ) {
		if(strcmp(table_it->second.m_uuid,vm_uuid) == 0 ) {
			fprintf(g_logFile,"Match found for given UUID \n");
			*verification_status = table_it->second.m_vm_verfication_status;
			pthread_mutex_unlock(&m_procTable.loc_proc_table);
			return TCSERVICE_RESULT_SUCCESS;
		}
	}
	pthread_mutex_unlock(&m_procTable.loc_proc_table);
	fprintf(g_logFile,"Match not found for given UUID \n");
	return TCSERVICE_RESULT_FAILED;
}

// Need to get nonce also as input
TCSERVICE_RESULT tcServiceInterface::GenerateSAMLAndGetDir(char *vm_uuid,char *nonce, char* vm_manifest_dir)
{
	fprintf(g_logFile, "\nIn function Generate SAML report\n");
	int proc_id = m_procTable.getprocIdfromuuid(vm_uuid);
	if (proc_id == NULL) {
		fprintf(g_logFile, "UUID : %s is not registered with vRTM\n");
		return TCSERVICE_RESULT_FAILED;
	}
	serviceprocEnt * pEnt = m_procTable.getEntfromprocId(proc_id);
	fprintf(g_logFile, "Match found for given UUID \n");

	sprintf(vm_manifest_dir, "/var/lib/nova/instances/%s/", vm_uuid);
	////OLD CODE HERE
	char xmlstr[8192] = { 0 };
	sprintf(xmlstr,
			"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><VMQuote><nonce>%s</nonce><vm_instance_id>%s</vm_instance_id><digest_alg>%s</digest_alg><cumulative_hash>%s</cumulative_hash></VMQuote>",
			nonce, vm_uuid, "SHA256", pEnt->m_vm_manifest_hash);
	char tempfile1[50];
	sprintf(tempfile1, "%sus_xml.xml", vm_manifest_dir);
	FILE * fp = fopen(tempfile1, "w");

	fprintf(fp, "%s", xmlstr);
	fclose(fp);
	char command1[200] = { 0 };
	sprintf(command1, "openssl dgst -sha1 -binary -out %shash.input %s",
			vm_manifest_dir, tempfile1);
	system(command1);
	//two JB's commands tht use hash.in and store signature in hash.out
	system(
			"export SIGNING_KEY_PASSWORD=$(cat /opt/trustagent/configuration/trustagent.properties | grep signing.key.secret | cut -d = -f 2)");
	char command2[400] = { 0 };
	sprintf(command2,
			"/opt/trustagent/bin/tpm_signdata -i %shash.input -k /opt/trustagent/configuration/signingkey.blob -o %shash.sig 495b2740ddbca3fdbc2c9f61066b4683608c565f -x",
			vm_manifest_dir, vm_manifest_dir);
	char command3[800] = { 0 };
	sprintf(command3,
			"export SIGNING_KEY_PASSWORD=$(cat /opt/trustagent/configuration/trustagent.properties | grep signing.key.secret | cut -d = -f 2); /opt/trustagent/bin/tpm_signdata -i %shash.input -k /opt/trustagent/configuration/signingkey.blob -o %shash.sig -q SIGNING_KEY_PASSWORD -t -x",
			vm_manifest_dir, vm_manifest_dir);

	fprintf(g_logFile, "%s \n\n \n ", command2);
	fflush(stdout);
	system(command3);

	char convertHashInputToBase64[200] = { 0 };
	sprintf(convertHashInputToBase64,
			"openssl base64 -in %shash.input -out %shash.input.b64",
			vm_manifest_dir, vm_manifest_dir);
	system(convertHashInputToBase64);

	char convertHashSigToBase64[200] = { 0 };
	sprintf(convertHashSigToBase64,
			"openssl base64 -A -in %shash.sig -out %shash.sig.b64",
			vm_manifest_dir, vm_manifest_dir);
	system(convertHashSigToBase64);

	char tempfile2[200];
	sprintf(tempfile2, "%shash.sig.b64", vm_manifest_dir);
	fp = fopen(tempfile2, "r");
	char* signature = (char *) malloc(1000 * sizeof(char));
	fscanf(fp, "%s", signature);
	fclose(fp);

	char tempfile3[200];
	sprintf(tempfile3, "%shash.input.b64", vm_manifest_dir);
	fp = fopen(tempfile3, "r");
	char* digval = (char *) malloc(1000 * sizeof(char));
	fscanf(fp, "%s", digval);
	fclose(fp);
	char *path = "/opt/trustagent/configuration/signingkey.pem";
	char command[200];

	sprintf(command,
			"openssl x509 -in %s -text | awk '/-----BEGIN CERTIFICATE-----/,/-----END CERTIFICATE-----/' > %sfile",
			path, vm_manifest_dir);
	system(command);
	memset(command, '\0', strlen(command));
	sprintf(command,
			"grep -vwE \"(-----BEGIN CERTIFICATE-----|-----END CERTIFICATE-----)\" %sfile  > %sfile2",
			vm_manifest_dir, vm_manifest_dir);
	system(command);
	char *file_contents;
	long input_file_size;
	char tempfile4[50];
	sprintf(tempfile4, "%sfile2", vm_manifest_dir);
	FILE *input_file = fopen(tempfile4, "r");
	fseek(input_file, 0, SEEK_END);
	input_file_size = ftell(input_file);
	rewind(input_file);

	file_contents = (char*) malloc((input_file_size + 1) * (sizeof(char)));
	fread(file_contents, sizeof(char), input_file_size, input_file);
	fclose(input_file);
	file_contents[input_file_size] = 0;
	// printf("\nCertificate is : %s \n",file_contents);

/////OLD CODE HERE

	sprintf(xmlstr,
			"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><VMQuote><nonce>%s</nonce><vm_instance_id>%s</vm_instance_id><digest_alg>%s</digest_alg><cumulative_hash>%s</cumulative_hash><Signature xmlns=\"http://www.w3.org/2000/09/xmldsig#\">\n<SignedInfo>\n<CanonicalizationMethod Algorithm=\"http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments\"/>\n<SignatureMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#rsa-sha1\"/>\n<Reference URI=\"#HostTrustAssertion\">\n<Transforms>\n<Transform Algorithm=\"http://www.w3.org/2000/09/xmldsig#enveloped-signature\"/>\n</Transforms>\n<DigestMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#sha1\"/>\n<DigestValue>%s</DigestValue>\n</Reference>\n</SignedInfo>\n<SignatureValue>%s</SignatureValue>\n<KeyInfo>\n<X509Data>\n<X509Certificate>%s</X509Certificate>\n</X509Data>\n</KeyInfo>\n</Signature></VMQuote>",
			nonce, vm_uuid, "SHA256", pEnt->m_vm_manifest_hash, digval,
			signature, file_contents);
	free(file_contents);
	free(signature);
	free(digval);
	//system("rm -rf file file2");
	char filepath[1000] = { 0 };
//	sprintf(vm_manifest_dir,"/var/lib/nova/instances/%s/",vm_uuid);
	sprintf(filepath, "%ssigned_report.xml", vm_manifest_dir);
	fp = fopen(filepath, "w");
	fprintf(fp, "%s", xmlstr);
	fclose(fp);
	//system ("rm us_xml.xml hash.sig hash.input");

	return TCSERVICE_RESULT_SUCCESS;
}
TCSERVICE_RESULT tcServiceInterface::TerminateApp(int sizeIn, byte* rgIn, int* psizeOut, byte* out)
{
	//remove entry from table.
	char uuid[48] = {0};
	
	if ((rgIn == NULL) || (psizeOut == NULL) || (out == NULL))
		return TCSERVICE_RESULT_FAILED;
	memset(uuid, 0, g_max_uuid);
    memcpy(uuid, rgIn, g_sz_uuid);
    g_myService.m_procTable.removeprocEntry(uuid);
    *psizeOut = sizeof(int);
    *((int*)out) = (int)1;
	return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::UpdateAppID(char* str_rp_id, char* in_uuid, char *vdi_uuid, int* psizeOut, byte* out)
{
	//remove entry from table.
	char uuid[48] = {0};
	char vuuid[48] = {0};
	int  rp_id = -1; 
	if ((str_rp_id == NULL) || (in_uuid == NULL) || (out == NULL))
 		return TCSERVICE_RESULT_FAILED;
	rp_id = atoi(str_rp_id);
	memset(uuid, 0, g_max_uuid);
    memcpy(uuid, in_uuid, g_sz_uuid);
	memset(vuuid, 0, g_max_uuid);	
	memcpy(vuuid, vdi_uuid, g_sz_uuid);
	g_myService.m_procTable.updateprocEntry(rp_id, uuid, vuuid);
	*psizeOut = sizeof(int);
	*((int*)out) = (int)1;
	return TCSERVICE_RESULT_SUCCESS;
}

/*This function returns the value of an XML tag.
Input parameter: Line read from the XML file
Output: Value in the tag
How it works: THe function accepts a line containing tag value as input
it parses the line until it reaches quotes (" ")
and returns the value held inside them
so <File Path = "a.txt" .....> returns a.txt
include="*.out" ....> returns *.out and so on..
*/
char NodeValue[500];
char* tagEntry (char* line){

        char key[500];
                /*We use a local string 'key' here so that we dont make any changes
                to the line pointer passed to the funciton.
                This is useful in a line containing more than 1 XML tag values.
                E.g :<Dir Path="/etc" include="*.bin" exclude="*.conf">
                */
        int i =0;
        strcpy(key,line);
        char  *start,*end;

                while(key[i] != '>')
            i++;
        start = &key[++i];
        end = start;
        while(*end != '<')
            end++;
        *end = '\0';
        /*NodeValue is a global variable that holds the XML tag value
                at a given point of time.
                Its contents are copied after its new value addition immediately
                */
                strcpy(NodeValue,start);
        return start;
}


TCSERVICE_RESULT tcServiceInterface::StartApp(tcChannel& chan,
                                int procid, const char* file, int an, char** av,
                                int* poutsize, byte* out)
{
        		fprintf(g_logFile, " Inside StartApp \n");
        		fprintf(stdout, " Inside StartApp \n");
    int     size= SHA256DIGESTBYTESIZE;
    byte    rgHash[SHA256DIGESTBYTESIZE];
    int     child= 0;
    int     i;
    char    kernel_file[1024] = {0};
    char    ramdisk_file[1024] = {0};
    char    disk_file[1024] = {0};
    char    manifest_file[1024] = {0};
    char    formatted_manifest_file[1024] = {0};
    char    nohash_manifest_file[2048] = {0};
    char    cumulativehash_file[2048] = {0};
    char*   config_file = NULL;
    char *  vm_image_id;
    char*   vm_customer_id;
    char*   vm_manifest_hash;
    char*   vm_manifest_signature;
    char    vm_manifest_dir[2048] ={0};
    bool 	verification_status = false;

    if(an>30) {
        return TCSERVICE_RESULT_FAILED;
    }


    for ( i = 1; i < an; i++) {

        		fprintf(stdout, "arg parsing %d \n", i);
        if( av[i] && strcmp(av[i], "-kernel") == 0 ){
            strcpy(kernel_file, av[++i]);
        }

        if( av[i] && strcmp(av[i], "-ramdisk") == 0 ){
            strcpy(ramdisk_file, av[++i]);
        }

        if( av[i] && strcmp(av[i], "-config") == 0 ){
            config_file = av[++i];
        }

        if( av[i] && strcmp(av[i], "-disk") == 0 ){
                        strcpy(disk_file, av[++i]);
                }
        if( av[i] && strcmp(av[i], "-manifest") == 0 ){
                        strcpy(manifest_file, av[++i]);
			//Create path for just list of files to be passes to verifier
        		fprintf(g_logFile, "Manifest path %s\n", manifest_file);
        		fprintf(stdout, "Manifest path %s\n", manifest_file);
	   	       
		        strncpy(nohash_manifest_file, manifest_file, strlen(manifest_file)-strlen("/trustpolicy.xml"));
        		fprintf(g_logFile, "Manifest list path %s\n", nohash_manifest_file);
        		fprintf(stdout, "Manifest list path %s\n", nohash_manifest_file);
        		strcpy(vm_manifest_dir, nohash_manifest_file);
        		sprintf(formatted_manifest_file, "%s%s", nohash_manifest_file, "/fmanifest.xml");
		
        		sprintf(nohash_manifest_file, "%s%s", nohash_manifest_file, "/manifestlist.xml");
        		fprintf(g_logFile, "Manifest list path 2%s\n",nohash_manifest_file);
        		fprintf(stdout, "Manifest list path %s\n",nohash_manifest_file);
		        
        		strncpy(cumulativehash_file, manifest_file, strlen(manifest_file)-strlen("/trustpolicy.xml"));
        		sprintf(cumulativehash_file, "%s%s", cumulativehash_file, "/measurement.sha256");
                }
    }

    //v: this code will be replaced by IMVM call flow
//////////////////////////////////////////////////////////////////////////////////////////////////////

       //create domain process shall check the whitelist
        		fprintf(g_logFile, "Before create domain manifest \n");
        		fprintf(stdout, "Before create domain manifest \n");
        // put lock here
        pthread_mutex_lock(&(g_myService.startAppLock));
        child = create_domain(an, av);
        pthread_mutex_unlock(&(g_myService.startAppLock));
        // unlock here


        // There will be two input files now, one the manifest and other just the list of files to be passed to verifier 
	// Could be moved to separate function
	// Paths to 2nd manifest and output are hardcoded	

//	char * nohash_manifest_file ="/root/nohash_manifest.xml"; // Need to be passed by policy agent

        char launchPolicy[10];
        char goldenImageHash[65];
	

        FILE *fp, *fq ;
        char * line = NULL;
    	char * temp;
    	char * end;
    	size_t length = 0;


   //Open Manifest to get list of files to hash
    	xmlDocPtr Doc;
    	Doc = xmlParseFile(manifest_file);

        /*This will save the XML file in a correct format, as desired by our parser.
        We dont use libxml tools to parse but our own pointer legerdemain for the time being
        Main advantage is simplicity and speed ~O(n) provided space isn't an issue */

    	xmlSaveFormatFile (formatted_manifest_file, Doc, 1); /*This would render even inline XML perfect for line by line parsing*/
    	xmlFreeDoc(Doc);
        fp=fopen(formatted_manifest_file,"r");

        while (getline(&line, &length, fp) != -1) {

        		fprintf(g_logFile, "Reading a line  \n");
        	if(strstr(line,"<LaunchControlPolicy")!= NULL){
        		fprintf(g_logFile,  "Found tag  \n");
        		temp = tagEntry(line);
        		fprintf(g_logFile,  "Processed tag  \n");
        		fprintf(g_logFile,"<Policy=\"%s\">",NodeValue);

        		if (strcmp(NodeValue, "MeasureOnly") == 0) {
                    strcpy(launchPolicy, "Audit");
                }
                else if (strcmp(NodeValue, "MeasureAndEnforce") ==0) {
                    strcpy(launchPolicy, "Enforce");
                }

        	if (strcmp(launchPolicy, "Audit") != 0 && strcmp(launchPolicy, "Enforce") !=0) {
        		fclose(fp);
        		return TCSERVICE_RESULT_SUCCESS;
        	}
        }

        if(strstr(line,"<ImageHash")!= NULL){
        	fprintf(g_logFile,  "Found tag imagehash  \n");
            temp = tagEntry(line);
            fprintf(g_logFile,"<Image Hash=\"%s\"> \n",NodeValue);
            strcpy(goldenImageHash, NodeValue);
            vm_manifest_hash = (char *)malloc(sizeof(char)*(strlen(NodeValue) + 1));
			if(vm_manifest_hash == NULL) {
				fprintf(g_logFile,"\n  StartApp : Error in allocating memory for vm_manifest_hash");
				return TCSERVICE_RESULT_FAILED;
			}
			strcpy(vm_manifest_hash,NodeValue);
        }


        if(strstr(line,"<ImageId")!= NULL){
        	fprintf(g_logFile,   "Found image  id tag  \n");
            temp = tagEntry(line);
            fprintf(g_logFile,"<Image Id =\"%s\">\n",NodeValue);
            vm_image_id = (char *)malloc(sizeof(char)*(strlen(NodeValue) + 1));
            if(vm_image_id == NULL) {
            	fprintf(g_logFile,"\n  StartApp : Error in allocating memory for vm_image_id");
                return TCSERVICE_RESULT_FAILED;
           }
           strcpy(vm_image_id,NodeValue);
        }

        if(strstr(line,"<CustomerId")!= NULL){
        		fprintf(g_logFile,   "found custoimer id tag  \n");
        		temp = tagEntry(line);
        		fprintf(g_logFile,   "Processed custoimer id tag  \n");
        		fprintf(g_logFile,"<Customer Id =\"%s\">\n",NodeValue);
        		vm_customer_id = (char *)malloc(sizeof(char)*(strlen(NodeValue) + 1));
				if(vm_customer_id == NULL) {
					fprintf(g_logFile,"\n  StartApp : Error in allocating memory for vm_customer_id");
					return TCSERVICE_RESULT_FAILED;
				}
				strcpy(vm_customer_id,NodeValue);
          }
		if(strstr(line,"SignatureValue")!= NULL){
				temp = tagEntry(line);
				fprintf(g_logFile,"<Manifest Signature  =\"%s\">\n",NodeValue);
				vm_manifest_signature = (char *)malloc(sizeof(char)*(strlen(NodeValue) + 1));
						if(vm_manifest_signature == NULL) {
							fprintf(g_logFile,"\n  StartApp : Error in allocating memory for vm_manifest_hash");
							return TCSERVICE_RESULT_FAILED;
						}
			   strcpy(vm_manifest_signature,NodeValue);
			  }

		} // end of file parsing
        fclose(fp);
// Only call verfier when measurement is required
        char command[512]={0};
            	// append rpid to /tmp/imvm-result_"rpid".out
        sprintf(command,"./verifier %s %s IMVM  > /tmp/imvm-result_%d.out 2>&1", nohash_manifest_file, disk_file,child);
        system(command);
// Open measurement log file at a specified location
        fq = fopen(cumulativehash_file, "rb");
        if(!fq) 
		{
        	fprintf(stdout, "Error returned by verifer in generating cumulative hash, please check imvm-result.out for more logs\n");
        	return -1; // measurement failed  (verifier failed to measure)
		}

        char imageHash[65];
        int flag=0;

        if (fq != NULL) {
            char line[1000];
            if(fgets(line,sizeof(line),fq)!= NULL)  {
                //line[strlen ( line ) - 1] = '\0';
                strcpy(imageHash, line);
            }
        }
        fclose(fq);
        if (strcmp(imageHash, goldenImageHash) ==0) {
        	fprintf(stdout, "IMVM Verification Successfull\n");
            verification_status = true;
            flag=1;
        }
		else if ((strcmp(launchPolicy, "Audit") == 0)) {
			fprintf(g_logFile, "IMVM Verification Failed, but continuing with VM launch as MeasureOnly launch policy is used\n");
			fprintf(stdout, "IMVM Verification Failed, but continuing with VM launch as MeasureOnly launch policy is used\n");
			flag=1;
		}
		else {
			fprintf(g_logFile, "IMVM Verification Failed, not continuing with VM launch as MeasureAndEnforce launch policy is used\n");
			fprintf(stdout, "IMVM Verification Failed, not continuing with VM launch as MeasureAndEnforce launch policy is used\n");
			flag=0;
		}

        if (flag == 0) {
        	fprintf(g_logFile, "IMVM Verification Failed\n");
            fprintf(stdout, "IMVM Verification Failed\n");
            return TCSERVICE_RESULT_FAILED;
        }

//////////////////////////////////////////////////////////////////////////////////////////////////////

    //The code below does the work of converting 64 byte hex (imageHash) to 32 byte binary (rgHash)
    //same as in rpchannel/channelcoding.cpp:ascii2bin(),
    int c = 0;
    int len = strlen(imageHash);
    int iSize = 0;
    for (c= 0; c < len; c = c+2) {
        sscanf(&imageHash[c], "%02x", &rgHash[c/2]);
        iSize++;
    }

    if(!g_myService.m_procTable.addprocEntry(child, kernel_file, 0, (char**) NULL, size, rgHash)) {
        fprintf(g_logFile, "StartApp: cant add to proc table\n");
        return TCSERVICE_RESULT_FAILED;
    }

   if(!g_myService.m_procTable.updateprocEntry(child, vm_image_id, vm_customer_id, vm_manifest_hash, vm_manifest_signature,launchPolicy,verification_status, vm_manifest_dir)) {
        fprintf(g_logFile, "SartApp : can't update proc table entry\n");
        return TCSERVICE_RESULT_FAILED;
    }

    for ( i = 1; i < an; i++) {
        if( av[i] ) {
            free (av[i]);
            av[i] = NULL;
        }
    }
    // free all allocated variable
    free(vm_image_id);
    free(vm_customer_id);
    free(vm_manifest_hash);
    free(vm_manifest_signature);
    
    *poutsize = sizeof(int);
    *((int*)out) = (int)child;

    return TCSERVICE_RESULT_SUCCESS;
}


// ------------------------------------------------------------------------------


bool  serviceRequest(tcChannel& chan,int procid, u32 uReq, int origprocid, int inparamsize, byte* inparams)
{
    char*               szappexecfile= NULL;
    int                 outparamsize;
    byte                outparams[PARAMSIZE] = {0};
    int                 an = 0;
    char*               av[32];
	char*				str_rp_id = NULL;
		
#ifdef TEST
    fprintf(g_logFile, "Entering serviceRequest\n");
#endif

    outparamsize = PARAMSIZE;

#ifdef TEST
    fprintf(g_logFile, "serviceRequest after get procid: %d, req, %d, origprocid %d\n", 
           procid, uReq, origprocid); 
#endif
	fprintf(g_logFile,"Input Parameters before switch case : %s\n",inparams);
    char response;
    switch(uReq) {

      case RP2VM_STARTAPP:
#ifdef TCTEST1
        fprintf(g_logFile, "serviceRequest, RP2VM_STARTAPP, decoding\n");
#endif
        an= 20;
        if(!decodeVM2RP_STARTAPP(&szappexecfile, &an, 
                    (char**) av, inparams)) {
            fprintf(g_logFile, "serviceRequest: decodeRP2VM_STARTAPP failed\n");
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
        //
        // todo: release memory for szappexecfile??
        //
        return true;


      case RP2VM_SETUUID:
 
#ifdef TCTEST1
        fprintf(g_logFile, "serviceRequest, RP2VM_SETUUID, decoding\n");
#endif
        an= 20;
        //if(!decodeVM2RP_STARTAPP(&str_rp_id, &an, (char**) av, inparams)) {
	if(!decodeVM2RP_SETUUID(&str_rp_id, &an, (char**) av, inparams)) {
            fprintf(g_logFile, "serviceRequest: decodeRP2VM_SETUUID failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        outparamsize= PARAMSIZE;
        
#ifdef TEST
        fprintf(g_logFile, "serviceRequest, about to setuuid %s, for %d\n",
                av[1], origprocid);
#endif
	printf("%s , %s, %s",av[1], av[2], av[3]);
		if (av[0] ){
			if(g_myService.UpdateAppID(av[1], av[2], av[3], &outparamsize, outparams)
					!=TCSERVICE_RESULT_SUCCESS) {
				fprintf(g_logFile, "serviceRequest: setuuid failed %s\n", str_rp_id);
				chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
				return false;
			}
			
			//release av[0] and str_rp_id
		}
#ifdef TEST
        fprintf(g_logFile, "serviceRequest, setuuid succeeded, about to send\n");
#endif
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (setuuid) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        //
        // todo: release memory for szappexecfile
        //
        return true;
        
      case RP2VM_TERMINATEAPP:
#ifdef TCTEST1
        fprintf(g_logFile, "serviceRequest, RP2VM_TERMINATEAPP, decoding\n");
#endif
        
        if(!decodeVM2RP_TERMINATEAPP(&outparamsize, outparams, inparams)) {
            fprintf(g_logFile, "serviceRequest: decodeRP2VM_TERMINATEAPP failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        
        memcpy(inparams, outparams, outparamsize);
        inparamsize = outparamsize;
        outparamsize= PARAMSIZE;
        memset(outparams, 0, outparamsize);
		
		
       if(g_myService.TerminateApp(inparamsize, inparams, &outparamsize, outparams)
                !=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: terminate app failed %s\n", szappexecfile);
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        
        //outparam will be success or failure
        
#ifdef TEST
        fprintf(g_logFile, "serviceRequest, terminate app succeeded, about to send\n");
#endif
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (terminateapp) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

			/***********new API ******************/
        case RP2VM_GETRPID:
        {
        	outparamsize = PARAMSIZE;
        	//printf("%s",inparams);
        	if(!decodeRP2VM_GETRPID(&outparamsize,outparams,inparams))
        	{
        		fprintf(g_logFile, "serviceRequest: decodeRP2VM_GETRPID failed\n");
        		g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
        		return false;
        	}
            fprintf(g_logFile,"Inparams after decoding : %s\n",inparams);
            fprintf(g_logFile,"outparams after decoding: %s\n",outparams);
        	//inparamsize = PARAMSIZE;
        	//memset(inparams,0,inparamsize);
        	char uuid[50];
		char rpid[16];
		int rpidsize=16;
        	memcpy(uuid,outparams,outparamsize+1);
        	//call getRPID(outparams,&inparams)
        	if(g_myService.GetRpId(uuid,(byte *)rpid,&rpidsize))
        	{
        		fprintf(g_logFile, "RP2VM_GETRPID: encodeRP2VM_GETRPID uuid does not exist\n");
				g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
				return false;
        	}
            fprintf(g_logFile,"rpid : %s\n",rpid);

        	//then encode the result
        	outparamsize = PARAMSIZE;
        	outparamsize = encodeRP2VM_GETRPID(rpidsize,(byte *)rpid,outparamsize,outparams);
            fprintf(g_logFile,"after encode : %s",outparams);
        	if(outparamsize<0) {
				fprintf(g_logFile, "RP2VM_GETRPID: encodeRP2VM_GETRPID buf too small\n");
				g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
				return false;
        	}
			if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)){
				fprintf(g_logFile, "serviceRequest: sendtcBuf (getRPID) failed\n");
				chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
				return false;
			}
            fprintf(g_logFile,"************succesfully send the response*************** ");
            return true;
        }
        case RP2VM_GETVMMETA:
        {
        	outparamsize = PARAMSIZE;
			if(!decodeRP2VM_GETVMMETA(&outparamsize,outparams,inparams))
			{
				fprintf(g_logFile, "serviceRequest: decodeRP2VM_GETRPID failed\n");
				g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
				return false;
			}
			//call to getVMMETA
			byte *vm_rpcustomerId;
			byte *vm_rpimageId;
			byte *vm_rpmanifestHash;
			byte *vm_rpmanifestSignature;
			vm_rpcustomerId = (byte *)malloc(sizeof(byte)*256);
			if(vm_rpcustomerId == NULL) {
					fprintf(g_logFile, "RP2VM_GETRPID: memory cann't be allocated for customerId \n");
					g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return false;
			}

			vm_rpimageId = (byte *) malloc(sizeof(byte)*256);
			if(vm_rpimageId == NULL) {
					fprintf(g_logFile, "RP2VM_GETRPID: memory cann't be allocated for imageId \n");
					g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return false;
			}
			vm_rpmanifestSignature = (byte *) malloc(sizeof(byte) *512);
			if(vm_rpmanifestSignature == NULL) {
					fprintf(g_logFile, "RP2VM_GETRPID: memory cann't be allocated for manifestSignature \n");
					g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return false;
			}
			vm_rpmanifestHash = (byte *) malloc(sizeof(byte) * 64);
			if( vm_rpmanifestHash== NULL) {
					fprintf(g_logFile, "RP2VM_GETRPID: memory cann't be allocated for vm_rpmanifestHash \n");
					g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
					return false;
			}
			int vm_rpimageIdsize, vm_rpcustomerIdsize,vm_rpmanifestHashsize,vm_rpmanifestSignaturesize;
			int in_procid = atoi((char *)outparams);
			if(g_myService.GetVmMeta(in_procid,vm_rpimageId, &vm_rpimageIdsize,vm_rpcustomerId, &vm_rpcustomerIdsize,
					vm_rpmanifestHash, &vm_rpmanifestHashsize,vm_rpmanifestSignature, &vm_rpmanifestSignaturesize))
			{
				fprintf(g_logFile, "RP2VM_GETRPID: encodeRP2VM_GETVMMETA RPID does not exist\n");
				g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
				return false;
			}
			//create a map of data vmmeta data
			fprintf(g_logFile,"vmimage id : %s\n",vm_rpimageId);
			fprintf(g_logFile,"vmcustomer id : %s\n",vm_rpcustomerId);
			fprintf(g_logFile,"vmmanifest hash : %s\n",vm_rpmanifestHash);
			fprintf(g_logFile,"vm manifest signature : %s\n",vm_rpmanifestSignature);
			byte * metaMap[4];
			metaMap[0] = vm_rpimageId;
			metaMap[1] = vm_rpcustomerId;
			metaMap[2] = vm_rpmanifestHash;
			metaMap[3] = vm_rpmanifestSignature;
			int numOfMetaComp = 4;
			//encode the vmMeta data
			outparamsize = PARAMSIZE;
			outparamsize = encodeRP2VM_GETVMMETA(numOfMetaComp,metaMap,outparamsize,outparams);
			fprintf(g_logFile,"after encode : %s\n",outparams);
			if(outparamsize<0) {
				fprintf(g_logFile, "RP2VM_GETRPID: encodeRP2VM_GETVMMETA buf too small\n");
				g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
				return false;
			}
			if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)){
				fprintf(g_logFile, "serviceRequest: sendtcBuf (getVMMETA) failed\n");
				chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
				return false;
			}
			fprintf(g_logFile,"************succesfully send the response*************** \n");
			free(vm_rpimageId);
			free(vm_rpcustomerId);
			free(vm_rpmanifestHash);
			free(vm_rpmanifestSignature);
				return true;
        }

        case RP2VM_ISVERIFIED:
        {
                fprintf(g_logFile, "\nin case ISVerified \n");
                if(!decodeRP2VM_ISVERIFIED(&outparamsize,outparams,inparams))
				{
						fprintf(g_logFile, "serviceRequest: decodeRP2VM_GETRPID failed\n");
						g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
						return false;
				}
                fprintf(g_logFile, "\ninparams before decode : %s\n",inparams);
                fprintf(g_logFile, "\noutparams after decode : %s \n",outparams);

				char uuid[50];
				char verificationstat[8];
				int  verificationstatsize=8;
				memcpy(uuid,outparams,outparamsize+1);
				int verification_status;
				if(g_myService.IsVerified(uuid,&verification_status))
				{
						fprintf(g_logFile, "RP2VM_ISVERIFIED : uuid does not exist\n");
						g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
						return false;
				}
				sprintf(verificationstat,"%d",verification_status);
				verificationstatsize = strlen((char *)verificationstat);
				outparamsize = PARAMSIZE;

				outparamsize = encodeRP2VM_ISVERIFIED(verificationstatsize, (byte *)verificationstat, outparamsize, outparams);
				if(outparamsize<0) {
						fprintf(g_logFile, "RP2VM_ISVERIFIED: encodeRP2VM_isverified buf too small\n");
						g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
						return false;
				}
				if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)){
						fprintf(g_logFile, "serviceRequest: sendtcBuf (isverified) failed\n");
						chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
						return false;
				}
				fprintf(g_logFile,"************succesfully send the response*************** ");
				return true;
        }

	case RP2VM_GETVMREPORT:
        {
        	fprintf(g_logFile, "\nin case GETVMREPORT \n");
            if(!decodeRP2VM_GETVMREPORT(&str_rp_id, &an, (char**) av, inparams))
			{
				fprintf(g_logFile, "serviceRequest: decodeRP2VM_GETVMREPORT failed\n");
				g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
				return false;
			}
        	fprintf(g_logFile, "\ninparams before decode : %s\n",inparams);
        	fprintf(g_logFile, "\noutparams after decode : %s %s \n", av[0], av[1]);

			char * vm_manifest_dir = (char *) malloc(sizeof(char) * 1024);
			if(g_myService.GenerateSAMLAndGetDir(av[0],av[1], vm_manifest_dir))
			{
					fprintf(g_logFile, "RP2VM_GETVMREPORT : uuid does not exist\n");
						g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
						return false;
			}
			int vm_manifest_dir_size = strlen(vm_manifest_dir);
			outparamsize = PARAMSIZE;

			outparamsize = encodeRP2VM_GETVMREPORT(vm_manifest_dir_size, (byte *)vm_manifest_dir, outparamsize, outparams);
			if(outparamsize<0) {
				fprintf(g_logFile, "RP2VM_GETVMREPORT: encodeRP2VM_isverified buf too small\n");
				g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
				return false;
			}
			if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)){
				fprintf(g_logFile, "serviceRequest: sendtcBuf (isverified) failed\n");
				chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
				return false;
			}
			free(vm_manifest_dir);
			fprintf(g_logFile,"************succesfully send the response*************** ");
			return true;
        }

        default:
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
        return false;
    }
}

int create_domain(int argc, char **argv)
{
        int ret = -1;
        ret = g_rpdomid++;
        return ret;
}

requestData* create_request_data(int procid, int origprocid, u32 uReq, int inparamsize, byte *inparams)
{
	requestData *reqData;
	reqData = (requestData *)calloc(1,sizeof(requestData));
	reqData->procid = procid;
	reqData->origprocid = origprocid;
	reqData->uReq = uReq;
	reqData->inparamsize = inparamsize;
	reqData->inparams = (byte *)calloc(1,(sizeof(byte)*inparamsize));
	memcpy(reqData->inparams,inparams,inparamsize);
	return reqData;
}

void free_request_data(requestData *reqData)
{
	free(reqData->inparams);
	free(reqData);
}

void* async_service_request(void * reqData)
{
	bool res;
	requestData *req = (requestData *) reqData;

	pthread_mutex_lock(&g_myService.max_thread_lock);
	g_myService.threadCount++;
	pthread_mutex_unlock(&g_myService.max_thread_lock);

	res = serviceRequest(g_reqChannel, req->procid,req->uReq, req->origprocid, req->inparamsize, req->inparams);
	free_request_data(req);

	pthread_mutex_lock(&g_myService.max_thread_lock);
	g_myService.threadCount--;
	pthread_mutex_unlock(&g_myService.max_thread_lock);
	return NULL;
}

bool start_request_processing() {
	requestData *reqData;
	int				procid,origprocid,inparamsize;
	u32				uReq,uStatus;
	byte*				inparams;
	pthread_t srvreq_thread;
	int thread_count;
	int error;
	inparamsize = PARAMSIZE;
	inparams = (byte *)calloc(1, sizeof(byte)*inparamsize);
	if(!g_reqChannel.gettcBuf(&procid, &uReq, &uStatus,	&origprocid, &inparamsize, inparams)) {
		fprintf(g_logFile, "serviceRequest: gettcBuf failed\n");
		free(inparams);
		return false;
	}
	if(uStatus==TCIOFAILED) {
		g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
		free(inparams);
		return false;
	}

	pthread_mutex_lock(&g_myService.max_thread_lock);
	thread_count = g_myService.threadCount;
	pthread_mutex_unlock(&g_myService.max_thread_lock);

	if (thread_count <= g_max_thread_limit)
	{

		reqData = create_request_data(procid,origprocid, uReq, inparamsize, inparams);
		//detach state attribute of g_myService.pthreadInit is initialized with PTHREAD_CREATE_DETACHED,
		//means all the thread uses this thread attribute are created in detached state
		error=pthread_create(&srvreq_thread, &g_myService.pthreadInit, &async_service_request, (void *)reqData);
		//async_service_request( (void *)reqData);
		//error=0;
		if(error) {
			g_myService.printErrorMessagge(error);
			g_reqChannel.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
		}
	}
    free(inparams);
	return true;
}

int modmain(int an, char** av)
{
    int                 iRet= 0;
    bool                fInitKeys= false;
    const char*         szexecFile = av[0];
    int                 i;
    bool                fServiceStart;
    const char*		configfile = "./tcconfig.xml";

    for(i=0; i<an; i++) {
        if(strcmp(av[i], "-help")==0) {
            fprintf(g_logFile, "\nUsage: tcService.exe [-initKeys] ");
            return 0;
        }
        if(strcmp(av[i], "-initKeys")==0) {
            fInitKeys= true;
        }
         /*if(strcmp(av[i], "-directory")==0) {
            directory= av[++i];
        }*/
		if(strcmp(av[i], "-cfgfile")==0) {
            configfile= av[++i];
        }        
    }

	//populate_whitelist_hashes();
    g_servicepid = 0;//v: getpid();
    g_myService.maxThread=g_max_thread_limit;
    
    /*if(!g_myService.m_procTable.initprocTable(NUMPROCENTS)) {
        fprintf(g_logFile, "tcService main: Cant init proctable\n");
        iRet= 1;
        goto cleanup;
    }*/

#if 0
    ret= g_myService.initService(szexecFile, 0, NULL);
    if(ret!=TCSERVICE_RESULT_SUCCESS) {
        fprintf(g_logFile, "tcService main: initService failed %s\n", szexecFile);
        iRet= 1;
        goto cleanup;
    }
#endif

    // add self proctable entry
    g_myService.m_procTable.addprocEntry(g_servicepid, strdup(szexecFile), 0, NULL,
                                      0,NULL );
    while(!g_fterminateLoop) {
        fServiceStart= start_request_processing();
        UNUSEDVAR(fServiceStart);
    }

cleanup:

    closeLog();
    return iRet;
}


// ------------------------------------------------------------------------------


