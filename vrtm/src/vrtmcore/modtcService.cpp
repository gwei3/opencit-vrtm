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
#include "vrtm_api_code.h"

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
//char 		g_mtwproxy_ip[64] = "127.0.0.1";
//int 		g_mtwproxy_on = 0;
//int 		g_mtwproxy_port = 16006;
#define NUMPROCENTS 200
#define LAUNCH_ALLOWED		"launch allowed"	 
#define LAUNCH_NOT_ALLOWED	"launch not allowed"
#define KEYWORD_UNTRUSTED	"untrusted"
// ---------------------------------------------------------------------------

bool uidfrompid(int pid, int* puid)
{
    char        szfileName[256];
    struct stat fattr;
    LOG_TRACE("");
    sprintf(szfileName, "/proc/%d/stat", pid);
    if((lstat(szfileName, &fattr))!=0) {
        LOG_DEBUG("uidfrompid: stat failed");
        return false;
    }
    *puid= fattr.st_uid;
    return true;
}
// ------------------------------------------------------------------------------
void serviceprocEnt::print()
{
	LOG_TRACE("Printing proc entry");
    LOG_INFO("vRTM id: %ld, ", (long int)m_procid);
    if(m_szexeFile!=NULL)
        LOG_INFO( "file: %s, ", m_szexeFile);
    	LOG_INFO("hash size: %d, ", m_sizeHash);
    PrintBytes("", m_rgHash, m_sizeHash);
}

serviceprocTable::serviceprocTable()
{
	//LOG_TRACE("Constructor called");
    loc_proc_table = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
}

serviceprocTable::~serviceprocTable()
{
	//LOG_TRACE("Destructor Called");
}

bool serviceprocTable::addprocEntry(int procid, const char* file, int an, char** av,
                                    int sizeHash, byte* hash)
{
    LOG_DEBUG("vRTM id : %d file : %s hash size : %d hash : %s", procid, file, sizeHash, hash);
    if(sizeHash>32) {
    	LOG_ERROR("Size of hash is more than 32 bytes. Hash size = %d", sizeHash);
        return false;
    }
    pthread_mutex_lock(&loc_proc_table);
    serviceprocEnt proc_ent;
    //proc_ent.m_procid = procid;
    proc_ent.m_szexeFile = strdup(file);
    proc_ent.m_sizeHash = sizeHash;
    memcpy(proc_ent.m_rgHash,hash,sizeHash);
    proc_table.insert(std::pair<int, serviceprocEnt>(procid, proc_ent));
    pthread_mutex_unlock(&loc_proc_table);
    LOG_INFO("Entry added for vRTM id %d\n",procid);
    return true;
}


//Step through linked list m_pMap and delete node matching procid
void   serviceprocTable::removeprocEntry(int procid)
{
	LOG_DEBUG("vRTM id to be removed : %d", procid);
	pthread_mutex_lock(&loc_proc_table);
	proc_table_map::iterator table_it = proc_table.find(procid);
	if( table_it == proc_table.end()) {
		pthread_mutex_unlock(&loc_proc_table);
		LOG_ERROR("Table entry can't be removed, given RPID : %d doesn't exist\n", procid);
		return;
	}
	proc_table.erase(table_it);
	pthread_mutex_unlock(&loc_proc_table);
	LOG_INFO("Table entry removed successfully for vRTM ID : %d\n",procid);
	return;
}

//Step through linked list m_pMap and delete node matching uuid
void   serviceprocTable::removeprocEntry(char* uuid)
{
	LOG_DEBUG(" UUID to be removed from vrtm map : %s", uuid);
	int proc_id = getprocIdfromuuid(uuid);
	if ( proc_id == NULL ) {
		LOG_ERROR("Entry removal failed from Table, UUID %s is not registered with vRTM", uuid);
		return;
	}
	removeprocEntry(proc_id);
    LOG_INFO("Entry removed from Table for UUID : %s\n", uuid);
	return;
}

bool serviceprocTable::updateprocEntry(int procid, char* uuid, char *vdi_uuid)
{
	LOG_DEBUG("vRTM map entry of vRTM id : %d to be updated with UUID : %s and VDIUUID : %s ", procid, uuid, vdi_uuid);
    pthread_mutex_lock(&loc_proc_table);
    proc_table_map::iterator table_it = proc_table.find(procid);
	if (table_it == proc_table.end()) {
		pthread_mutex_unlock(&loc_proc_table);
		LOG_ERROR("UUID %s can't be registered with vRTM, given rpid %d doesn't exist", uuid, procid);
		return false;
	}
    memset(table_it->second.m_uuid, 0, g_max_uuid);
    memset(table_it->second.m_vdi_uuid, 0, g_max_uuid);
    memcpy(table_it->second.m_uuid, uuid, g_sz_uuid);
    memcpy(table_it->second.m_vdi_uuid, vdi_uuid, g_sz_uuid);
    pthread_mutex_unlock(&loc_proc_table);
    LOG_INFO("UUID : %s is registered with vRTM successfully\n",table_it->second.m_uuid);
    return true;
}

bool serviceprocTable::updateprocEntry(int procid, char* vm_image_id, char* vm_customer_id, char* vm_manifest_hash,
									char* vm_manifest_signature, char *launch_policy, bool verification_status, char * vm_manifest_dir) {
    //geting the procentry related to this procid
	LOG_DEBUG("vRTM map entry of vRTM id : %d to be updated with vm image id : %s vm customer id : %s vm hash : %s"
			" vm manifest signature : %s launch policy : %s verification status : %d vm manifest dir : %s", procid, vm_image_id,
			vm_customer_id, vm_manifest_hash, vm_manifest_signature, launch_policy, verification_status, vm_manifest_dir);
	pthread_mutex_lock(&loc_proc_table);
	proc_table_map::iterator table_it = proc_table.find(procid);
	if( table_it == proc_table.end()) {
		pthread_mutex_unlock(&loc_proc_table);
		LOG_ERROR("Couldn't update the given data in table, rpid %d doesn't exist", procid);
		return false;
	}
	strcpy(table_it->second.m_vm_image_id,vm_image_id);
	table_it->second.m_size_vm_image_id = strlen(table_it->second.m_vm_image_id);
	strcpy(table_it->second.m_vm_customer_id, vm_customer_id);
	table_it->second.m_size_vm_customer_id = strlen(table_it->second.m_vm_customer_id);
	strcpy(table_it->second.m_vm_manifest_signature, vm_manifest_signature);
	table_it->second.m_size_vm_manifest_signature = strlen(table_it->second.m_vm_manifest_signature);
	strcpy(table_it->second.m_vm_manifest_hash, vm_manifest_hash);
	table_it->second.m_size_vm_manifest_hash = strlen(table_it->second.m_vm_manifest_hash);
	strcpy(table_it->second.m_vm_manifest_dir, vm_manifest_dir);
	table_it->second.m_size_vm_manifest_dir = strlen(table_it->second.m_vm_manifest_dir);
	strcpy(table_it->second.m_vm_launch_policy, launch_policy);
	table_it->second.m_size_vm_launch_policy = strlen(table_it->second.m_vm_launch_policy);
	table_it->second.m_vm_verfication_status = verification_status;
	pthread_mutex_unlock(&loc_proc_table);
	LOG_INFO("Data updated against vRTM ID : %d in the Table\n", procid);
    return true;
}

serviceprocEnt* serviceprocTable::getEntfromprocId(int procid) {
	LOG_DEBUG("Looking for Map Entry of vRTM Id : %d", procid);
	pthread_mutex_lock(&loc_proc_table);
	proc_table_map::iterator table_it = proc_table.find(procid);
	if(table_it == proc_table.end()) {
		pthread_mutex_unlock(&loc_proc_table);
		LOG_WARN("Entry with vRTM Id %d not found", procid);
		return NULL;
	}
	pthread_mutex_unlock(&loc_proc_table);
	LOG_DEBUG("Entry with vRTM Id %d found", procid);
	return &(table_it->second);
}

int serviceprocTable::getprocIdfromuuid(char* uuid) {
	LOG_DEBUG("Finding vRTM Id of map entry w having UUID : %s", uuid);
	pthread_mutex_lock(&loc_proc_table);
	for (proc_table_map::iterator table_it = proc_table.begin(); table_it != proc_table.end() ; table_it++ ) {
		if(strcmp(table_it->second.m_uuid,uuid) == 0 ) {
			pthread_mutex_unlock(&loc_proc_table);
			LOG_INFO("vRTM ID of entry with UUID is : %d", table_it->first);
			return (table_it->first);
		}
	}
	pthread_mutex_unlock(&loc_proc_table);
	LOG_WARN("UUID %s is not registered with vRTM", uuid);
	return NULL;
}


void serviceprocTable::print()
{
	pthread_mutex_lock(&loc_proc_table);
	for(proc_table_map::iterator table_it = proc_table.begin(); table_it != proc_table.end() ; table_it++ ) {
		table_it->second.print();
	}
	LOG_INFO("Table has %d entries",proc_table.size());
    pthread_mutex_unlock(&loc_proc_table);
    return;
}


// -------------------------------------------------------------------
void tcServiceInterface::printErrorMessagge(int error) {
        if ( error == EAGAIN ){
                LOG_ERROR( "Insufficient resources to create another thread");
        }
        if ( error == EINVAL) {
        	LOG_ERROR( "Invalid settings in pthreadInit");
        }
        if ( error == EPERM ) {
        	LOG_ERROR( "No permission to set the scheduling policy and parameters specified in pthreadInit");
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
		//LOG_ERROR( "Asynchronous measured launch will not work %s", strerror(errno) );
		THREAD_ENABLED = false;
		return;
	}
	THREAD_ENABLED = true;
	// if you want result back from thread replace this line
	pthread_attr_setdetachstate (&pthreadInit, PTHREAD_CREATE_DETACHED);
}


tcServiceInterface::~tcServiceInterface()
{
	//LOG_INFO("*************************************destructor called**********");
	pthread_attr_destroy (&pthreadInit);
}


TCSERVICE_RESULT tcServiceInterface::initService(const char* execfile, int an, char** av)
{
#if 0
    u32     hashType= 0;
    int     sizehash= SHA256DIGESTBYTESIZE;
    byte    rgHash[SHA256DIGESTBYTESIZE];

    if(!getfileHash(execfile, &hashType, &sizehash, rgHash)) {
        LOG_ERROR("initService: getfileHash failed %s\n", execfile);
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TEST
    LOG_INFO( "initService size hash %d\n", sizehash);
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
	LOG_DEBUG("Finding vRTM ID for given VMUUID : %s", vm_uuid);
	int proc_id = m_procTable.getprocIdfromuuid(vm_uuid);
    if(proc_id == NULL)
	{
		LOG_ERROR("Match not found for UUID %s", vm_uuid);
		return TCSERVICE_RESULT_FAILED;
	}

    LOG_INFO("match found for UUID %s", vm_uuid);
	sprintf((char *)rpidbuf,"%d",proc_id);
	*rpidsize = strlen((char *)rpidbuf);
	return TCSERVICE_RESULT_SUCCESS;
}

//*************************return vmeta for given procid(rpid)*************/

TCSERVICE_RESULT tcServiceInterface::GetVmMeta(int procId, byte *vm_imageId, int * vm_imageIdsize,
    						byte * vm_customerId, int * vm_customerIdsize, byte * vm_manifestHash, int * vm_manifestHashsize,
    						byte * vm_manifestSignature, int * vm_manifestSignaturesize)
{
	LOG_DEBUG("Finding map entry for vRTM ID : %d", procId);
    serviceprocEnt *pEnt = m_procTable.getEntfromprocId(procId);
    if(pEnt == NULL ) {
    	LOG_ERROR("vRTM ID %d is not registered in vRTM core", procId);
    	return TCSERVICE_RESULT_FAILED;
    }
	LOG_DEBUG("Match found for given RPid");
	LOG_TRACE("VM image id : %s",pEnt->m_vm_image_id);
	memcpy(vm_imageId,pEnt->m_vm_image_id,pEnt->m_size_vm_image_id + 1);
	LOG_TRACE("VM image id copied : %s",vm_imageId);
	*vm_imageIdsize = pEnt->m_size_vm_image_id ;
	memcpy(vm_customerId,pEnt->m_vm_customer_id,pEnt->m_size_vm_customer_id + 1);
    LOG_TRACE("Customer ID: %s", pEnt->m_vm_customer_id);
	*vm_customerIdsize = pEnt->m_size_vm_customer_id ;
	memcpy(vm_manifestHash,pEnt->m_vm_manifest_hash, pEnt->m_size_vm_manifest_hash + 1);
    LOG_TRACE("Manifest Hash: %s", pEnt->m_vm_manifest_hash);
	*vm_manifestHashsize = pEnt->m_size_vm_manifest_hash ;
	memcpy(vm_manifestSignature,pEnt->m_vm_manifest_signature,pEnt->m_size_vm_manifest_signature + 1);
    LOG_TRACE("Manifest Signature: %s", pEnt->m_vm_manifest_signature);
	*vm_manifestSignaturesize = pEnt->m_size_vm_manifest_signature ;
	return TCSERVICE_RESULT_SUCCESS;
}

/****************************return verification status of vm with attestation policy*********************** */
TCSERVICE_RESULT tcServiceInterface::IsVerified(char *vm_uuid, int* verification_status)
{
	LOG_DEBUG("Finding verification status of UUID : %s", vm_uuid);
	pthread_mutex_lock(&m_procTable.loc_proc_table);
	for (proc_table_map::iterator table_it = m_procTable.proc_table.begin(); table_it != m_procTable.proc_table.end() ; table_it++ ) {
		if(strcmp(table_it->second.m_uuid,vm_uuid) == 0 ) {
			*verification_status = table_it->second.m_vm_verfication_status;
			pthread_mutex_unlock(&m_procTable.loc_proc_table);
			LOG_INFO("Verification status for UUID %s is %d", vm_uuid, verification_status);
			return TCSERVICE_RESULT_SUCCESS;
		}
	}
	pthread_mutex_unlock(&m_procTable.loc_proc_table);
	LOG_WARN("Match not found for given UUID %s", vm_uuid);
	return TCSERVICE_RESULT_FAILED;
}

// Need to get nonce also as input
TCSERVICE_RESULT tcServiceInterface::GenerateSAMLAndGetDir(char *vm_uuid,char *nonce, char* vm_manifest_dir)
{
	LOG_DEBUG("Generating SAML Report for UUID: %s and getting manifest Directory against nonce : %s", vm_uuid, nonce);
	int proc_id = m_procTable.getprocIdfromuuid(vm_uuid);
	if (proc_id == NULL) {
		LOG_ERROR("UUID : %s is not registered with vRTM\n", vm_uuid);
		return TCSERVICE_RESULT_FAILED;
	}
	serviceprocEnt * pEnt = m_procTable.getEntfromprocId(proc_id);
	LOG_INFO("Match found for given UUID \n");
	sprintf(vm_manifest_dir, "/var/lib/nova/instances/%s/", vm_uuid);
	LOG_DEBUG("Manifest Dir : %s", vm_manifest_dir);
	////OLD CODE HERE
	char xmlstr[8192] = { 0 };
	sprintf(xmlstr,
			"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><VMQuote><nonce>%s</nonce><vm_instance_id>%s</vm_instance_id><digest_alg>%s</digest_alg><cumulative_hash>%s</cumulative_hash></VMQuote>",
			nonce, vm_uuid, "SHA256", pEnt->m_vm_manifest_hash);
	char tempfile1[200];
	LOG_DEBUG("XML content : %s", xmlstr);
	sprintf(tempfile1, "%sus_xml.xml", vm_manifest_dir);
	FILE * fp = fopen(tempfile1, "w");

	fprintf(fp, "%s", xmlstr);
	fclose(fp);
	char command1[500] = { 0 };
	sprintf(command1, "openssl dgst -sha1 -binary -out %shash.input %s",
			vm_manifest_dir, tempfile1);
	LOG_DEBUG("command generated to calculate hash: %s", command1);
	system(command1);
	//two JB's commands tht use hash.in and store signature in hash.out
	system(
			"export SIGNING_KEY_PASSWORD=$(cat /opt/trustagent/configuration/trustagent.properties | grep signing.key.secret | cut -d = -f 2)");
	char command2[500] = { 0 };
	sprintf(command2,
			"/opt/trustagent/bin/tpm_signdata -i %shash.input -k "
			"/opt/trustagent/configuration/signingkey.blob -o %shash.sig 495b2740ddbca3fdbc2c9f61066b4683608c565f -x",
			vm_manifest_dir, vm_manifest_dir);
	char command3[800] = { 0 };
	sprintf(command3,
			"export SIGNING_KEY_PASSWORD=$(cat /opt/trustagent/configuration/trustagent.properties | "
			"grep signing.key.secret | cut -d = -f 2); "
			"/opt/trustagent/bin/tpm_signdata -i %shash.input -k "
			"/opt/trustagent/configuration/signingkey.blob -o %shash.sig -q SIGNING_KEY_PASSWORD -t -x",
			vm_manifest_dir, vm_manifest_dir);

	LOG_DEBUG("COMMAND2 :%s \n", command2);
	LOG_DEBUG("COMMAND3 :%s \n", command3);

	system(command3);

	char convertHashInputToBase64[500] = { 0 };
	sprintf(convertHashInputToBase64,
			"openssl base64 -in %shash.input -out %shash.input.b64",
			vm_manifest_dir, vm_manifest_dir);
	LOG_DEBUG("Command to convert Hash to base64 : %s", convertHashInputToBase64);
	system(convertHashInputToBase64);

	char convertHashSigToBase64[500] = { 0 };
	sprintf(convertHashSigToBase64,
			"openssl base64 -A -in %shash.sig -out %shash.sig.b64",
			vm_manifest_dir, vm_manifest_dir);
	LOG_DEBUG("Command to convert signature to base64 : %s", convertHashSigToBase64);
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
	char command[500];

	sprintf(command,
			"openssl x509 -in %s -text | awk '/-----BEGIN CERTIFICATE-----/,/-----END CERTIFICATE-----/' > %sfile",
			path, vm_manifest_dir);
	LOG_DEBUG("Command to generate certificate : %s", command);
	system(command);
	memset(command, '\0', strlen(command));
	sprintf(command,
			"grep -vwE \"(-----BEGIN CERTIFICATE-----|-----END CERTIFICATE-----)\" %sfile  > %sfile2",
			vm_manifest_dir, vm_manifest_dir);
	LOG_DEBUG("Command to extract certificate Content : %s", command);
	system(command);
	char *file_contents;
	long input_file_size;
	char tempfile4[200];
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
    //This log comment cause crash
    //LOG_INFO("\n\nSingned xml report : %s\n", xmlstr);
	fprintf(fp, "%s", xmlstr);
	fclose(fp);
	//system ("rm us_xml.xml hash.sig hash.input");

	return TCSERVICE_RESULT_SUCCESS;
}
TCSERVICE_RESULT tcServiceInterface::TerminateApp(char* uuid, int* psizeOut, byte* out)
{
	//remove entry from table.
    LOG_TRACE("Terminate VM");
	if ((uuid == NULL) || (psizeOut == NULL) || (out == NULL)){
		LOG_ERROR("Can't remove entry for given NULL UUID");
		return TCSERVICE_RESULT_FAILED;
	}
    g_myService.m_procTable.removeprocEntry(uuid);
    *psizeOut = sizeof(int);
    *((int*)out) = (int)1;
	return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::UpdateAppID(char* str_rp_id, char* in_uuid, char *vdi_uuid, int* psizeOut, byte* out)
{
	//remove entry from table.
    LOG_TRACE("Map UUID %s and VDI UUID %s with ID %s", in_uuid, vdi_uuid, str_rp_id);
	char uuid[48] = {0};
	char vuuid[48] = {0};
	int  rp_id = -1; 
	if ((str_rp_id == NULL) || (in_uuid == NULL) || (out == NULL)){
		LOG_ERROR("Can't Register UUID with vRTM either vRTM ID or UUID is NULL");
 		return TCSERVICE_RESULT_FAILED;
	}
	rp_id = atoi(str_rp_id);
	int inuuid_len = strlen(in_uuid);
	int invdiuuid_len = strlen(vdi_uuid);
	memset(uuid, 0, g_max_uuid);
    memcpy(uuid, in_uuid, inuuid_len);
	memset(vuuid, 0, g_max_uuid);	
	memcpy(vuuid, vdi_uuid, invdiuuid_len);
	if ( !g_myService.m_procTable.updateprocEntry(rp_id, uuid, vuuid) ) {
		return TCSERVICE_RESULT_FAILED;
	}
	LOG_DEBUG("UUID %s and VDI UUID %s are updated against vRTM ID %d", uuid, vuuid, rp_id);
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
        LOG_TRACE("Current Node value : %s", NodeValue);
        return start;
}


TCSERVICE_RESULT tcServiceInterface::StartApp(int procid, int an, char** av, int* poutsize, byte* out)
{
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
    LOG_TRACE("Start VM App");
    if(an>30) {
    	LOG_ERROR("Number of arguments passed are more than limit 30");
        return TCSERVICE_RESULT_FAILED;
    }


    for ( i = 1; i < an; i++) {

        LOG_TRACE( "arg parsing %d \n", i);
        if( av[i] && strcmp(av[i], "-kernel") == 0 ){
            strcpy(kernel_file, av[++i]);
            LOG_DEBUG("Kernel File Name : %s", kernel_file);
        }

        if( av[i] && strcmp(av[i], "-ramdisk") == 0 ){
            strcpy(ramdisk_file, av[++i]);
            LOG_DEBUG("RAM disk : %s", ramdisk_file);
        }

        if( av[i] && strcmp(av[i], "-config") == 0 ){
            config_file = av[++i];
            LOG_DEBUG("config file : %s",config_file);
        }

        if( av[i] && strcmp(av[i], "-disk") == 0 ){
        	strcpy(disk_file, av[++i]);
            LOG_DEBUG("Disk : %s",disk_file );
        }
        if( av[i] && strcmp(av[i], "-manifest") == 0 ){
                        strcpy(manifest_file, av[++i]);
			//Create path for just list of files to be passes to verifier
        		LOG_DEBUG( "Manifest file : %s\n", manifest_file);
		        strncpy(nohash_manifest_file, manifest_file, strlen(manifest_file)-strlen("/trustpolicy.xml"));
        		LOG_DEBUG( "Manifest list path %s\n", nohash_manifest_file);
        		strcpy(vm_manifest_dir, nohash_manifest_file);
        		sprintf(formatted_manifest_file, "%s%s", nohash_manifest_file, "/fmanifest.xml");
        		sprintf(nohash_manifest_file, "%s%s", nohash_manifest_file, "/manifestlist.xml");
        		LOG_DEBUG("Manifest list path 2%s\n",nohash_manifest_file);
        		strncpy(cumulativehash_file, manifest_file, strlen(manifest_file)-strlen("/trustpolicy.xml"));
        		sprintf(cumulativehash_file, "%s%s", cumulativehash_file, "/measurement.sha256");
        		LOG_DEBUG("Cumulative hash file : %s", cumulativehash_file);
        }
    }

    //v: this code will be replaced by IMVM call flow
//////////////////////////////////////////////////////////////////////////////////////////////////////
       //create domain process shall check the whitelist
		child = procid;
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

       		LOG_TRACE("Reading a line");
        	if(strstr(line,"<LaunchControlPolicy")!= NULL){
        		LOG_DEBUG("Found tag");
        		temp = tagEntry(line);
        		LOG_DEBUG("<Policy=\"%s\">",NodeValue);
        		if (strcmp(NodeValue, "MeasureOnly") == 0) {
                    strcpy(launchPolicy, "Audit");
                }
                else if (strcmp(NodeValue, "MeasureAndEnforce") ==0) {
                    strcpy(launchPolicy, "Enforce");
                }
        	if (strcmp(launchPolicy, "Audit") != 0 && strcmp(launchPolicy, "Enforce") !=0) {
        		fclose(fp);
        		LOG_INFO("Launch policy is neither Audit nor Enforce so vm verification is not not carried out");
        		return TCSERVICE_RESULT_SUCCESS;
        	}
        }

        if(strstr(line,"<ImageHash")!= NULL){
        	LOG_DEBUG( "Found tag imagehash ");
            temp = tagEntry(line);
            LOG_DEBUG("<Image Hash=\"%s\"> ",NodeValue);
            strcpy(goldenImageHash, NodeValue);
            vm_manifest_hash = (char *)malloc(sizeof(char)*(strlen(NodeValue) + 1));
			if(vm_manifest_hash == NULL) {
				LOG_ERROR("StartApp : Error in allocating memory for vm_manifest_hash");
				return TCSERVICE_RESULT_FAILED;
			}
			strcpy(vm_manifest_hash,NodeValue);
        }


        if(strstr(line,"<ImageId")!= NULL){
        	LOG_DEBUG("Found image  id tag");
            temp = tagEntry(line);
            LOG_DEBUG("<Image Id =\"%s\">",NodeValue);
            vm_image_id = (char *)malloc(sizeof(char)*(strlen(NodeValue) + 1));
            if(vm_image_id == NULL) {
            	LOG_ERROR("StartApp : Error in allocating memory for vm_image_id");
                return TCSERVICE_RESULT_FAILED;
           }
           strcpy(vm_image_id,NodeValue);
        }

        if(strstr(line,"<CustomerId")!= NULL){
        	LOG_DEBUG("found custoimer id tag");
        	temp = tagEntry(line);
        	LOG_DEBUG("Processed custoimer id tag");
        	LOG_DEBUG("<Customer Id =\"%s\">\n",NodeValue);
        	vm_customer_id = (char *)malloc(sizeof(char)*(strlen(NodeValue) + 1));
			if(vm_customer_id == NULL) {
				LOG_DEBUG(" StartApp : Error in allocating memory for vm_customer_id");
				return TCSERVICE_RESULT_FAILED;
			}
				strcpy(vm_customer_id,NodeValue);
          }
		if(strstr(line,"SignatureValue")!= NULL){
				temp = tagEntry(line);
				LOG_DEBUG("<Manifest Signature  =\"%s\">",NodeValue);
				vm_manifest_signature = (char *)malloc(sizeof(char)*(strlen(NodeValue) + 1));
				if(vm_manifest_signature == NULL) {
					LOG_ERROR("StartApp : Error in allocating memory for vm_manifest_hash");
					return TCSERVICE_RESULT_FAILED;
				}
			   strcpy(vm_manifest_signature,NodeValue);
			  }
		} // end of file parsing
        free(line);
        line = NULL;
        fclose(fp);
// Only call verfier when measurement is required
        char command[512]={0};
        // append rpid to /tmp/imvm-result_"rpid".out
        sprintf(command,"./verifier %s %s IMVM  > /tmp/imvm-result_%d.out 2>&1", nohash_manifest_file, disk_file,child);
        LOG_DEBUG("Command to execute verifier binary : %s", command);
        system(command);
// Open measurement log file at a specified location
        fq = fopen(cumulativehash_file, "rb");
        if(!fq) 
		{
        	LOG_ERROR("Error returned by verifer in generating cumulative hash, please check imvm-result.out for more logs\n");
        	free(vm_image_id);
			free(vm_customer_id);
			free(vm_manifest_hash);
			free(vm_manifest_signature);
        	return TCSERVICE_RESULT_FAILED; // measurement failed  (verifier failed to measure)
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
        LOG_DEBUG("Calculated hash : %s and Golden Hash : %s",imageHash, goldenImageHash);
        if (strcmp(imageHash, goldenImageHash) ==0) {
        	LOG_INFO("IMVM Verification Successfull");
            verification_status = true;
            flag=1;
        }
		else if ((strcmp(launchPolicy, "Audit") == 0)) {
			LOG_INFO("IMVM Verification Failed, but continuing with VM launch as MeasureOnly launch policy is used");
			flag=1;
		}
		else {
			LOG_ERROR("IMVM Verification Failed, not continuing with VM launch as MeasureAndEnforce launch policy is used");
			flag=0;
		}

        if (flag == 0) {
        	LOG_INFO("IMVM Verification Failed");
            free(vm_image_id);
			free(vm_customer_id);
			free(vm_manifest_hash);
			free(vm_manifest_signature);
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
    LOG_TRACE("Adding proc table entry for measured VM");
    if(!g_myService.m_procTable.addprocEntry(child, kernel_file, 0, (char**) NULL, size, rgHash)) {
    	LOG_ERROR( "StartApp: cant add to vRTM Map\n");
        return TCSERVICE_RESULT_FAILED;
    }
    LOG_TRACE("Updating proc table entry");
   if(!g_myService.m_procTable.updateprocEntry(child, vm_image_id, vm_customer_id, vm_manifest_hash, vm_manifest_signature,launchPolicy,verification_status, vm_manifest_dir)) {
	   	LOG_ERROR("SartApp : can't update proc table entry\n");
        return TCSERVICE_RESULT_FAILED;
    }

    for ( i = 0; i < an; i++) {
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


bool  serviceRequest(int procid, u32 uReq, int inparamsize, byte* inparams, int *outparamsize, byte* outparams)
{
    int                 an = 0;
    char*               av[32];
    char*               method_name;
    int 				fr_var;
    bool ret_val = false;

	//outparams[PARAMSIZE] = {0};
	//outparams = (byte *) calloc(1,sizeof(byte)*PARAMSIZE);
    LOG_TRACE("Entering serviceRequest");

    *outparamsize = PARAMSIZE;

	LOG_DEBUG("Input Parameters before switch case : %s",inparams);
    switch(uReq) {

      case VM2RP_STARTAPP:
        an= 20;
        if(!decodeVM2RP_STARTAPP(&method_name, &an, 
                    (char**) av, inparams)) {
        	LOG_ERROR( "Failed to decode the input XML for Start App");
            //outparams = NULL;
            *outparamsize = 0;
            ret_val=false;
            goto cleanup;
        }
        //*outparamsize= PARAMSIZE;
        if(g_myService.StartApp(procid,an,av,outparamsize,outparams)) {
        	LOG_ERROR("Start App failed. Method name: %s", method_name);
        	//outparams = NULL;
        	*outparamsize = 0;
        	ret_val = false;
                goto cleanup;
        }
        LOG_INFO("Start App successful");
        ret_val = true;
        break;

      case VM2RP_SETUUID:
 
        LOG_TRACE( "serviceRequest, RP2VM_SETUUID, decoding");
        an= 20;
        if(!decodeVM2RP_SETUUID(&method_name, &an, (char**) av, inparams)) {
        	LOG_ERROR( "Failed to decode the input XML for Set UUID");
            //outparams = NULL;
            *outparamsize = 0;
            ret_val = false;
            goto cleanup;
        }
        *outparamsize= PARAMSIZE;
        
        LOG_DEBUG("In SET_UUID : params after decoding : %s , %s, %s",av[0], av[1], av[2]);
		if (av[0] ){
			if(g_myService.UpdateAppID(av[0], av[1], av[2], outparamsize, outparams)
					!=TCSERVICE_RESULT_SUCCESS) {
				LOG_ERROR( "Set UUID failed. Method name: %s", method_name);
				//outparams = NULL;
				*outparamsize = 0;
                                ret_val = false;
                                goto cleanup;
			}
			
		}
		LOG_INFO("UUID %s is registered with vRTM ID %s successful", av[1], av[0]);
        ret_val = true;
        break;
        
      case VM2RP_TERMINATEAPP:
        LOG_TRACE( "decoding the input XML for terminate app");
        if(!decodeVM2RP_TERMINATEAPP(&method_name, &an, (char**) av, inparams)) {
            LOG_ERROR( "Failed to decode the input XML for terminate app");
            //outparams = NULL;
            *outparamsize = 0;
            ret_val = false;
            goto cleanup;
        }
        LOG_DEBUG("Data after decoding : %s ", av[0]);
        if(av[0]) {
            if(g_myService.TerminateApp(av[0], outparamsize, outparams)
                               !=TCSERVICE_RESULT_SUCCESS) {
                LOG_ERROR("failed to deregister VM of given vRTM ID : %s", inparams);
                *outparamsize = 0;
                ret_val = false;
                goto cleanup;
            }
        }
        
        //outparam will be success or failure
        
        LOG_INFO( "Deregister VM with vRTM ID %s succussfully", av[0]);
        ret_val = true;
        break;

			/***********new API ******************/
        case VM2RP_GETRPID:
        {
        	*outparamsize = PARAMSIZE;
        	if(!decodeRP2VM_GETRPID(outparamsize,outparams,inparams))
        	{
        		LOG_ERROR( "failed to decode the input XML");
        		//outparams = NULL;
        		*outparamsize = 0;
                        ret_val = false;
                        goto cleanup;
        	}
            LOG_DEBUG("outparams after decoding: %s",outparams);
        	char uuid[50];
        	char rpid[16];
        	int rpidsize=16;
        	memcpy(uuid, outparams,*outparamsize+1);
        	//call getRPID(outparams,&inparams)
        	if(g_myService.GetRpId(uuid,(byte *)rpid,&rpidsize))
        	{
        		LOG_ERROR( "Failed to get vRTM ID for given UUID");
        		//outparams = NULL;
        		*outparamsize = 0;
			ret_val = false;
                        goto cleanup;
        	}
            LOG_DEBUG("vRTM ID : %s",rpid);

        	//then encode the result
        	*outparamsize = PARAMSIZE;
        	*outparamsize = encodeRP2VM_GETRPID(rpidsize,(byte *)rpid,*outparamsize,outparams);
            LOG_DEBUG("after encode : %s",outparams);
        	if(*outparamsize<0) {
				LOG_ERROR( "Failed to send the vRTM ID ,Encoded data too small");
				//outparams = NULL;
				*outparamsize = 0;
				ret_val = false;
                                goto cleanup;
        	}
            LOG_INFO("Successfully sent vRTM ID for given UUID");
            ret_val = true;
            break;
        }
        case VM2RP_GETVMMETA:
        {
        	*outparamsize = PARAMSIZE;
			if(!decodeRP2VM_GETVMMETA(outparamsize,outparams,inparams))
			{
				LOG_ERROR( "failed to decode the input XML");
				//outparams = NULL;
				*outparamsize = 0;
				ret_val = false;
                                goto cleanup;
			}
			//call to getVMMETA
			LOG_DEBUG("Decoded data : %s", outparams);
			byte vm_rpcustomerId[CUSTOMER_ID_SIZE];
			byte vm_rpimageId[IMAGE_ID_SIZE];
			byte vm_rpmanifestHash[MANIFEST_HASH_SIZE];
			byte vm_rpmanifestSignature[MANIFEST_SIGNATURE_SIZE];

			int vm_rpimageIdsize, vm_rpcustomerIdsize,vm_rpmanifestHashsize,vm_rpmanifestSignaturesize;
			int in_procid = atoi((char *)outparams);
			if(g_myService.GetVmMeta(in_procid,vm_rpimageId, &vm_rpimageIdsize,vm_rpcustomerId, &vm_rpcustomerIdsize,
					vm_rpmanifestHash, &vm_rpmanifestHashsize,vm_rpmanifestSignature, &vm_rpmanifestSignaturesize))
			{
				LOG_ERROR( "Failed to send VM meta data, vRTM ID does not exist");
				//outparams = NULL;
				*outparamsize = 0;
                                ret_val = false;
                                goto cleanup;
			}
			//create a map of data vmmeta data
			LOG_DEBUG("vmimage id : %s",vm_rpimageId);
			LOG_DEBUG("vmcustomer id : %s",vm_rpcustomerId);
			LOG_DEBUG("vmmanifest hash : %s",vm_rpmanifestHash);
			LOG_DEBUG("vm manifest signature : %s",vm_rpmanifestSignature);
			byte * metaMap[4];
			metaMap[0] = vm_rpimageId;
			metaMap[1] = vm_rpcustomerId;
			metaMap[2] = vm_rpmanifestHash;
			metaMap[3] = vm_rpmanifestSignature;
			int numOfMetaComp = 4;
			//encode the vmMeta data
			*outparamsize = PARAMSIZE;
			*outparamsize = encodeRP2VM_GETVMMETA(numOfMetaComp,metaMap,*outparamsize,outparams);
			LOG_DEBUG("after encode : %s\n",outparams);
			if(*outparamsize<0) {
				LOG_ERROR("Failed to Send VM Metadata, Encoded data to small");
				//outparams = NULL;
				*outparamsize = 0;
                                ret_val = false;
                                goto cleanup;
 			}
			LOG_INFO("VM Meta data Successfully sent");
			ret_val = true;
			break;
        }

        case VM2RP_ISVERIFIED:
        {
                LOG_TRACE("Processing ISVerified API request");
                if(!decodeRP2VM_ISVERIFIED(outparamsize,outparams,inparams))
				{
                		LOG_ERROR( "failed to decode the input XML");
						//outparams = NULL;
						*outparamsize = 0;
						ret_val = false;
						goto cleanup;
				}
                LOG_DEBUG("outparams after decode : %s",outparams);

				char uuid[UUID_SIZE];
				char verificationstat[8];
				int  verificationstatsize=8;
				memcpy(uuid,outparams, *outparamsize+1);
				int verification_status;
				if(g_myService.IsVerified(uuid,&verification_status))
				{
						LOG_ERROR("Failed to send verification status, uuid does not exist");
						//outparams = NULL;
						*outparamsize = 0;
						ret_val = false;
						goto cleanup;
				}
				sprintf(verificationstat,"%d",verification_status);
				verificationstatsize = strlen((char *)verificationstat);
				*outparamsize = PARAMSIZE;

				*outparamsize = encodeRP2VM_ISVERIFIED(verificationstatsize, (byte *)verificationstat, *outparamsize, outparams);
				if(*outparamsize<0) {
						LOG_ERROR("Failed to Send Verification status, Encoded data to small");
						//outparams = NULL;
						*outparamsize = 0;
						ret_val = false;
						goto cleanup;
				}
				LOG_INFO("successfully sent verification status of given UUID");
				ret_val = true;
				break;
        }

	case VM2RP_GETVMREPORT:
        {
        	LOG_TRACE("in case GETVMREPORT");
            if(!decodeRP2VM_GETVMREPORT(&method_name, &an, (char**) av, inparams))
			{
            	LOG_ERROR( "failed to decode the input XML");
				//outparams = NULL;
				*outparamsize = 0;
				ret_val = false;
				goto cleanup;
			}
        	LOG_DEBUG( "outparams after decode : %s %s ", av[0], av[1]);

			char vm_manifest_dir[1024];
			if(g_myService.GenerateSAMLAndGetDir(av[0],av[1], vm_manifest_dir))
			{
					LOG_ERROR( "Failed to GET VMREPORT given uuid does not exist");
					//outparams = NULL;
					*outparamsize = 0;
					ret_val = false;
					goto cleanup;
			}

			int vm_manifest_dir_size = strlen(vm_manifest_dir);
			*outparamsize = PARAMSIZE;

			*outparamsize = encodeRP2VM_GETVMREPORT(vm_manifest_dir_size, (byte *)vm_manifest_dir, *outparamsize, outparams);
			if(outparamsize<0) {
				LOG_ERROR("Failed to Send VM Report and manifest Dir, Encoded data to small");
				//outparams = NULL;
				*outparamsize = 0;
				ret_val = false;
				goto cleanup;
			}
			LOG_INFO("Successfully send the VM Report and manifest Directory ");
			ret_val = true;
			break;
        }

        default:
        	//outparams = NULL;
        	*outparamsize = 0;
                ret_val = false;
		break;
    }
    cleanup:
        for( fr_var = 0 ; fr_var < an ; fr_var++ ) {
            if(av[fr_var]){
                free(av[fr_var]);
                av[fr_var]=NULL;
            }
        }
        if(method_name) {
            free(method_name);
            method_name = NULL;
        }
        return ret_val;
}
