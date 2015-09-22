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


#include "logging.h"
#include "modtcService.h"
#include "channelcoding.h"
#include "xpathparser.h"
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
#define mount_script "../scripts/mount_vm_image.sh"
#define ma_log "/measurement.log"
uint32_t	g_rpdomid = 1000;
static int g_cleanup_service_status = 0;


#define NUMPROCENTS 200
#define LAUNCH_ALLOWED		"launch allowed"	 
#define LAUNCH_NOT_ALLOWED	"launch not allowed"
#define KEYWORD_UNTRUSTED	"untrusted"


int cleanupService();
void* clean_vrtm_table(void *p);
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
    proc_ent.m_vm_status = VM_STATUS_STOPPED;
    strcpy(proc_ent.m_uuid, av[0]);
    memcpy(proc_ent.m_rgHash,hash,sizeHash);
    proc_table.insert(std::pair<int, serviceprocEnt>(procid, proc_ent));
    pthread_mutex_unlock(&loc_proc_table);
    LOG_INFO("Entry added for vRTM id %d\n",procid);
    /*if( getproctablesize() == 1) {
    	cleanupService();
    }*/
    return true;
}


//Step through linked list m_pMap and delete node matching procid
bool serviceprocTable::removeprocEntry(int procid)
{
	LOG_DEBUG("vRTM id to be removed : %d", procid);
	pthread_mutex_lock(&loc_proc_table);
	proc_table_map::iterator table_it = proc_table.find(procid);
	if( table_it == proc_table.end()) {
		pthread_mutex_unlock(&loc_proc_table);
		LOG_ERROR("Table entry can't be removed, given RPID : %d doesn't exist\n", procid);
		return false;
	}
	if(table_it->second.m_szexeFile) {
		free(table_it->second.m_szexeFile);
		table_it->second.m_szexeFile = NULL;
	}
	char file_to_del[1024] = {'\0'};
	sprintf(file_to_del, "rm -rf %s", table_it->second.m_vm_manifest_dir);
	system(file_to_del);
	proc_table.erase(table_it);
	pthread_mutex_unlock(&loc_proc_table);
	LOG_INFO("Table entry removed successfully for vRTM ID : %d\n",procid);
	return true;
}

//Step through linked list m_pMap and delete node matching uuid
bool serviceprocTable::removeprocEntry(char* uuid)
{
	LOG_DEBUG(" UUID to be removed from vrtm map : %s", uuid);
	int proc_id = getprocIdfromuuid(uuid);
	if ( proc_id == NULL ) {
		LOG_ERROR("Entry removal failed from Table, UUID %s is not registered with vRTM", uuid);
		return false;
	}
	LOG_INFO("vRTM ID %d will be removed for UUID %s", proc_id, uuid);
	return removeprocEntry(proc_id);
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
	if (verification_status == false && (strcmp(launch_policy, "Enforce") == 0)) {
		LOG_DEBUG("Updated the VM status to : %d ", VM_STATUS_CANCELLED);
		table_it->second.m_vm_status = VM_STATUS_CANCELLED;
	}
	pthread_mutex_unlock(&loc_proc_table);
	cleanupService();
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

int	serviceprocTable::getproctablesize(){
	int size;
	pthread_mutex_lock(&loc_proc_table);
	size = proc_table.size();
	pthread_mutex_unlock(&loc_proc_table);
	return size;
}

int serviceprocTable::getcancelledvmcount() {
	int count = 0;
	pthread_mutex_lock(&loc_proc_table);
	for( proc_table_map::iterator table_it = proc_table.begin(); table_it != proc_table.end() ; table_it++) {
		if (table_it->second.m_vm_status == VM_STATUS_CANCELLED) {
			count++;
		}
	}
	pthread_mutex_unlock(&loc_proc_table);
	LOG_DEBUG("Number of VM with cancelled status : %d ", count);
	return count;
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
    
	char xmlstr[8192]={0};
	char tpm_signkey_passwd[100]={0};
	char tempfile[200]={0};
	char filepath[200]={0};
	char command0[400]={0};
	char manifest_dir[400]={0};
	FILE * fp = NULL;
	FILE * fp1 = NULL;
	
    LOG_DEBUG("Generating SAML Report for UUID: %s and getting manifest Directory against nonce : %s", vm_uuid, nonce);
	
	int proc_id = m_procTable.getprocIdfromuuid(vm_uuid);
	if (proc_id == NULL) {
		LOG_ERROR("UUID : %s is not registered with vRTM\n", vm_uuid);
		return TCSERVICE_RESULT_FAILED;
	}
	serviceprocEnt * pEnt = m_procTable.getEntfromprocId(proc_id);
	LOG_INFO("Match found for given UUID \n");
	if( pEnt->m_vm_status == VM_STATUS_STOPPED) {
		LOG_INFO("Can't generate report. VM with UUID : %s is in stopped state.");
		//TODO
		return TCSERVICE_RESULT_FAILED;
	}
	sprintf(vm_manifest_dir, "%s%s/", g_trust_report_dir,vm_uuid); 
	LOG_DEBUG("Manifest Dir : %s", vm_manifest_dir);
	
		
	strcpy(manifest_dir,vm_manifest_dir);

	// Generate Signed  XML  in same vm_manifest_dir
	//sprintf(manifest_dir,"/var/lib/nova/instances/%s/",vm_uuid);
	sprintf(filepath,"%ssigned_report.xml",manifest_dir);					


	fp1 = fopen(filepath,"w");
	if (fp1 == NULL) {
		LOG_ERROR("Can't write report in signed_report.xml file");
		return TCSERVICE_RESULT_FAILED;
	}
	sprintf(xmlstr,"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
	fprintf(fp1,"%s",xmlstr);
    LOG_DEBUG("XML content : %s", xmlstr);

	sprintf(xmlstr,"<VMQuote><nonce>%s</nonce><vm_instance_id>%s</vm_instance_id><digest_alg>%s</digest_alg><cumulative_hash>%s</cumulative_hash><Signature xmlns=\"http://www.w3.org/2000/09/xmldsig#\"><SignedInfo><CanonicalizationMethod Algorithm=\"http://www.w3.org/2001/10/xml-exc-c14n#\"/><SignatureMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#rsa-sha1\"/><Reference URI=\"\"><Transforms><Transform Algorithm=\"http://www.w3.org/2000/09/xmldsig#enveloped-signature\"/><Transform Algorithm=\"http://www.w3.org/2001/10/xml-exc-c14n#\"/></Transforms><DigestMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#sha1\"/><DigestValue>",nonce, vm_uuid,"SHA256", pEnt->m_vm_manifest_hash);
	fprintf(fp1,"%s",xmlstr);
	fclose(fp1);
    LOG_DEBUG("XML content : %s", xmlstr);

	// Calculate the Digest Value       


	sprintf(xmlstr,"<VMQuote><nonce>%s</nonce><vm_instance_id>%s</vm_instance_id><digest_alg>%s</digest_alg><cumulative_hash>%s</cumulative_hash></VMQuote>",nonce, vm_uuid,"SHA256", pEnt->m_vm_manifest_hash);
	sprintf(tempfile,"%sus_xml.xml",manifest_dir);
	fp = fopen(tempfile,"w");
	if (fp == NULL) {
		LOG_ERROR("can't open the file us_xml.xml");
		return TCSERVICE_RESULT_FAILED;
	}
	fprintf(fp,"%s",xmlstr);
	fclose(fp);

	sprintf(command0,"xmlstarlet c14n  %sus_xml.xml | openssl dgst -binary -sha1  | openssl enc -base64 | xargs echo -n >> %ssigned_report.xml", manifest_dir,manifest_dir);
	LOG_DEBUG("command generated to calculate hash: %s", command0);
	system(command0);
						

	fp1 = fopen(filepath,"a");
	if (fp1 == NULL) {
		LOG_ERROR("can't open the file signed_report.xml");
		return TCSERVICE_RESULT_FAILED;
	}
	sprintf(xmlstr,"</DigestValue></Reference></SignedInfo><SignatureValue>");
	fprintf(fp1,"%s",xmlstr);
    LOG_DEBUG("XML content : %s", xmlstr);
						
						
	// Calculate the Signature Value


	sprintf(tempfile,"%sus_can.xml",manifest_dir);
	fp = fopen(tempfile,"w");
	if (fp == NULL) {
		LOG_ERROR("can't open the file us_can.xml");
		return TCSERVICE_RESULT_FAILED;
	}
	sprintf(xmlstr,"<SignedInfo xmlns=\"http://www.w3.org/2000/09/xmldsig#\"><CanonicalizationMethod Algorithm=\"http://www.w3.org/2001/10/xml-exc-c14n#\"></CanonicalizationMethod><SignatureMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#rsa-sha1\"></SignatureMethod><Reference URI=\"\"><Transforms><Transform Algorithm=\"http://www.w3.org/2000/09/xmldsig#enveloped-signature\"></Transform><Transform Algorithm=\"http://www.w3.org/2001/10/xml-exc-c14n#\"></Transform></Transforms><DigestMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#sha1\"></DigestMethod><DigestValue>");

	fprintf(fp,"%s",xmlstr); 
	fclose(fp1);
	fclose(fp);
							  
	sprintf(command0,"xmlstarlet c14n  %sus_xml.xml | openssl dgst -binary -sha1  | openssl enc -base64 | xargs echo -n  >> %sus_can.xml", manifest_dir,manifest_dir);
	system(command0);
				 
	sprintf(xmlstr,"</DigestValue></Reference></SignedInfo>");
	fp = fopen(tempfile,"a");
	fprintf(fp,"%s",xmlstr);
	fclose(fp);



	// Store the TPM signing key password          
	sprintf(command0,"cat /opt/trustagent/configuration/trustagent.properties | grep signing.key.secret | cut -d = -f 2 | xargs echo -n > %ssign_key_passwd", manifest_dir);
	LOG_DEBUG("TPM signing key password :%s \n", command0);
	system(command0); 
					   
	sprintf(tempfile,"%ssign_key_passwd",manifest_dir);
	fp = fopen(tempfile,"r");
	if ( fp == NULL) {
		LOG_ERROR("can't open the file sign_key_passwd");
		return TCSERVICE_RESULT_FAILED;
	}
	fscanf(fp, "%s", tpm_signkey_passwd);
	fclose(fp);                


				 
	// Sign the XML
	sprintf(command0,"xmlstarlet c14n %sus_can.xml | openssl dgst -sha1 -binary -out %shash.input",manifest_dir,manifest_dir);
	system(command0);

	sprintf(command0,"/opt/trustagent/bin/tpm_signdata -i %shash.input -k /opt/trustagent/configuration/signingkey.blob -o %shash.sig -q %s -x",manifest_dir,manifest_dir,tpm_signkey_passwd);
	LOG_DEBUG("Signing Command : %s", command0);
	system(command0);

	sprintf(command0,"openssl enc -base64 -in %shash.sig |xargs echo -n >> %ssigned_report.xml",manifest_dir,manifest_dir); 
	system(command0);

					   

	fp1 = fopen(filepath,"a");
	sprintf(xmlstr,"</SignatureValue><KeyInfo><X509Data><X509Certificate>");
	LOG_DEBUG("XML content : %s", xmlstr);
	fprintf(fp1,"%s",xmlstr);
	fclose(fp1);
					

				 
	// Append the X.509 certificate
	sprintf(command0,"openssl x509 -in /opt/trustagent/configuration/signingkey.pem -text | awk '/-----BEGIN CERTIFICATE-----/,/-----END CERTIFICATE-----/' |  sed '1d;$d' >> %ssigned_report.xml",manifest_dir);
	LOG_DEBUG("Command to generate certificate : %s", command0);
	system(command0);
					
					   

	fp1 = fopen(filepath,"a");
	sprintf(xmlstr,"</X509Certificate></X509Data></KeyInfo></Signature></VMQuote>");
	fprintf(fp1,"%s",xmlstr);
	fclose(fp1);
					
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
    if(!g_myService.m_procTable.removeprocEntry(uuid))
        return TCSERVICE_RESULT_FAILED;
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

TCSERVICE_RESULT tcServiceInterface::UpdateAppStatus(char *uuid, int status) {
	LOG_TRACE("VM UUID : %s , status to be updated : %d", uuid, status);
	int procid = g_myService.m_procTable.getprocIdfromuuid(uuid);
	if (procid == NULL) {
		return TCSERVICE_RESULT_FAILED;
	}
	serviceprocEnt *procEnt = g_myService.m_procTable.getEntfromprocId(procid);
	if (procEnt->m_vm_status == VM_STATUS_CANCELLED) {
		LOG_INFO("Not updating VM status since VM is in cancelled status");
		return TCSERVICE_RESULT_SUCCESS;
	}
	if (status == VM_STATUS_DELETED) {
		if (g_myService.m_procTable.removeprocEntry(uuid)) {
			return TCSERVICE_RESULT_SUCCESS;
		}
		else
			return TCSERVICE_RESULT_FAILED;
	}
	procEnt->m_vm_status = status;
	LOG_INFO("Current VM status : %d", procEnt->m_vm_status);
	procEnt->m_status_upadation_time = time(NULL);
	struct tm cur_time = *localtime(&(procEnt->m_status_upadation_time));
	LOG_DEBUG("Status updation time = %d : %d : %d : %d : %d : %d", cur_time.tm_year, cur_time.tm_mon,
			cur_time.tm_mday, cur_time.tm_hour, cur_time.tm_min, cur_time.tm_sec);
	LOG_INFO("Successfully updated the VM with UUID : %s status to %d", uuid, status);
	return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT 	tcServiceInterface::CleanVrtmTable(unsigned long entry_max_age, int vm_status, int* deleted_entries) {
	*deleted_entries = 0;
	std::vector<int> keys_to_del;
	pthread_mutex_lock(&m_procTable.loc_proc_table);
	for( proc_table_map::iterator table_it = m_procTable.proc_table.begin(); table_it != m_procTable.proc_table.end(); table_it++ ){
		LOG_DEBUG("Comparing Current entry VM status : %d against vm status : %d ", table_it->second.m_vm_status, vm_status);
		if(table_it->second.m_vm_status == vm_status ) {
			int entry_age = difftime(time(NULL), table_it->second.m_status_upadation_time);
			if( entry_age >= entry_max_age) {
				keys_to_del.push_back(table_it->first);
			}
		}
	}
	pthread_mutex_unlock(&m_procTable.loc_proc_table);
	for(std::vector<int>::iterator iter = keys_to_del.begin() ; iter != keys_to_del.end(); iter++) {
			if (m_procTable.removeprocEntry(*iter))
				(*deleted_entries)++;
	}
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

TCSERVICE_RESULT tcServiceInterface::get_xpath_values(std::map<unsigned char *, char *> xpath_map, xmlChar* namespace_list, char* xml_file) {
	int p_size = 0;
	int parser_status = 0;
	xmlDocPtr Doc = NULL;
	xmlXPathContextPtr xpathCtx = NULL;
	char* elements_buf[1]; //array of one char * because we are passing exact xpath of element
	std::map<xmlChar *, char *>::iterator xpath_map_it;
	// Initialize xpath parser
	if (setup_xpath_parser(&Doc, &xpathCtx, xml_file) < 0 ) {
		LOG_ERROR("Couldn't setup xpath parser for file : \"%s\"", xml_file);
		parser_status = 1;
		goto return_parser_status;
	}
	LOG_TRACE("xpath parser is ready");

	for (xpath_map_it = xpath_map.begin(); xpath_map_it != xpath_map.end() ; xpath_map_it++) {
		p_size = 0;
		//check number of elements present with the xpath
		p_size = parse_xpath(xpathCtx, xpath_map_it->first, namespace_list, NULL, p_size );
		if (p_size  < 0) {
			LOG_ERROR("Error occured in parsing the xpath : \"%s\" in file : %s", xpath_map_it->first,xml_file);
			parser_status = 1;
			goto return_parser_status;
		}
		else if(p_size > 1) {
			LOG_ERROR("xpath : \"%s\" have more than one value in file : \"%s\"", xpath_map_it->first, xml_file);
			parser_status = 1;
			goto return_parser_status;
		}
		else {
			elements_buf[0] = xpath_map_it->second;
			p_size = parse_xpath(xpathCtx, xpath_map_it->first, namespace_list, elements_buf, 1);
			if (p_size < 0) {
				LOG_ERROR("Error occured while parsing the xpath : %s in file : %s", xpath_map_it->first, xml_file);
				parser_status = 1;
				goto return_parser_status;
			}
			else {
				LOG_DEBUG("Successfully parsed xpath : \"%s\" in file : \"%s\"", xpath_map_it->first, xml_file);
				LOG_DEBUG("Number of values returned for xpath : %d", p_size);
			}
		}
	}
	return_parser_status:
		teardown_xpath_parser(Doc, xpathCtx);
		LOG_TRACE("xpath parser destroyed");
		if (parser_status)
			return TCSERVICE_RESULT_FAILED;
		else
			return TCSERVICE_RESULT_SUCCESS;
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
    char    nohash_manifest_file[2048] = {0};
    char    cumulativehash_file[2048] = {0};
    char*   config_file = NULL;
    char *  vm_image_id = NULL;
    char*   vm_customer_id = NULL;
    char*   vm_manifest_hash = NULL;
    char*   vm_manifest_signature = NULL;
    char    vm_manifest_dir[2048] ={0};
    bool 	verification_status = false;
    char	vm_uuid[UUID_SIZE];
    int 	start_app_status = 0;
    char 	command[512]={0};
	FILE*   fp1=NULL;
	char    extension[20]={0};
	char    popen_command[250]={0};
	char    xml_command[]="xmlstarlet sel -t -m \"//@DigestAlg\" -v \".\" -n ";
	char    measurement_file[2048]={0};
	char 	mount_path[64];

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
        		//Extract UUID of VM
        		char *uuid_ptr = strrchr(vm_manifest_dir, '/');
        		strcpy(vm_uuid, uuid_ptr + 1);
        		LOG_TRACE("Extracted UUID : %s", vm_uuid);

        		sprintf(nohash_manifest_file, "%s%s", nohash_manifest_file, "/manifestlist.xml");
        		//Create Trust Report directory and copy relevant files
				char trust_report_dir[1024];
				strcpy(trust_report_dir, g_trust_report_dir);
				strcat(trust_report_dir, vm_uuid);
				strcat(trust_report_dir, "/");
				mkdir(trust_report_dir, 0766);
				char cmd[2048];
				sprintf(cmd,"cp -p %s %s/",manifest_file, trust_report_dir );
				system(cmd);
				memset(cmd,0, 2048);
				sprintf(cmd, "cp -p %s %s/", nohash_manifest_file, trust_report_dir);
				system(cmd);
        		strcpy(vm_manifest_dir, trust_report_dir);
        		LOG_DEBUG("VM Manifest Dir : %s", vm_manifest_dir);
				sprintf(manifest_file,"%s%s", trust_report_dir, "/trustpolicy.xml");
				LOG_DEBUG("Manifest path %s ", manifest_file);
				sprintf(nohash_manifest_file, "%s%s", trust_report_dir, "/manifestlist.xml");
				LOG_DEBUG("Manifest list path 2%s\n",nohash_manifest_file);
				
				//Read the digest algorithm from manifestlist.xml
				sprintf(popen_command,"%s%s",xml_command,nohash_manifest_file);
				fp1=popen(popen_command,"r");
				fgets(extension, sizeof(extension)-1, fp1);
				sprintf(measurement_file,"%s.%s","/measurement",extension);
				pclose(fp1);
				if(measurement_file[strlen(measurement_file) - 1] == '\n') 
					measurement_file[strlen(measurement_file) - 1] = '\0';
				LOG_DEBUG("Extension : %s",extension);
				
				strcpy(cumulativehash_file, trust_report_dir);
        		sprintf(cumulativehash_file, "%s%s", cumulativehash_file, measurement_file);
        		LOG_DEBUG("Cumulative hash file : %s", cumulativehash_file);
        }
    }

       //create domain process shall check the whitelist
		child = procid;

//	char * nohash_manifest_file ="/root/nohash_manifest.xml"; // Need to be passed by policy agent
        char launchPolicy[10] = {'\0'};
        char goldenImageHash[65] = {'\0'};
        FILE *fq ;

    	xmlChar namespace_list[] =			"a=mtwilson:trustdirector:policy:1.1 b=http://www.w3.org/2000/09/xmldsig#";
    	xmlChar xpath_customer_id[] = 		"/a:TrustPolicy/a:Director/a:CustomerId";
    	xmlChar xpath_launch_policy[] = 	"/a:TrustPolicy/a:LaunchControlPolicy";
    	xmlChar xpath_image_id[] = 			"/a:TrustPolicy/a:Image/a:ImageId";
    	xmlChar xpath_image_hash[] = 		"/a:TrustPolicy/a:Image/a:ImageHash";
    	xmlChar xpath_image_signature[] = 	"/a:TrustPolicy/b:Signature/b:SignatureValue";
    	char* launch_policy_buff = NULL;

    	/*
    	 * extract Launch Policy, CustomerId, ImageId, VM hash, and Manifest signature value from formatted manifestlist.xml
    	 * by specifying fixed xpaths with namespaces
    	 */
    	std::map<xmlChar *, char *> xpath_map;
    	launch_policy_buff = (char *)malloc(sizeof(char)* 64);
    	xpath_map.insert(std::pair<xmlChar *, char *>(xpath_launch_policy, launch_policy_buff));
    	vm_customer_id = (char *)malloc(sizeof(char) * CUSTOMER_ID_SIZE);
    	xpath_map.insert(std::pair<xmlChar*, char *>(xpath_customer_id, vm_customer_id));
    	vm_image_id = (char *) malloc(sizeof(char) * IMAGE_ID_SIZE);
    	xpath_map.insert(std::pair<xmlChar*, char *>(xpath_image_id, vm_image_id));
    	vm_manifest_hash = (char *) malloc(sizeof(char)* MANIFEST_HASH_SIZE);
    	xpath_map.insert(std::pair<xmlChar*, char *>(xpath_image_hash, vm_manifest_hash));
    	vm_manifest_signature = (char *) malloc(sizeof(char) * MANIFEST_SIGNATURE_SIZE);
    	xpath_map.insert(std::pair<xmlChar*, char *>(xpath_image_signature, vm_manifest_signature));

    	if (TCSERVICE_RESULT_FAILED == get_xpath_values(xpath_map, namespace_list, manifest_file)) {
    		//TODO write a remove directory function using dirint.h header file
    		char remove_file[1024] = { '\0' };
    		sprintf(remove_file, "rm -rf %s", vm_manifest_dir);
    		system(remove_file);
    		start_app_status = 1;
    		goto return_response;
		}
		if (strcmp(launch_policy_buff, "MeasureOnly") == 0) {
			strcpy(launchPolicy, "Audit");
		}
		else if (strcmp(launch_policy_buff, "MeasureAndEnforce") ==0) {
			strcpy(launchPolicy, "Enforce");
		}
		free(launch_policy_buff);
		if (strcmp(launchPolicy, "Audit") != 0 && strcmp(launchPolicy, "Enforce") !=0) {
			LOG_INFO("Launch policy is neither Audit nor Enforce so vm verification is not not carried out");
			char remove_file[1024] = {'\0'};
			sprintf(remove_file,"rm -rf %s", vm_manifest_dir);
			system(remove_file);
			start_app_status = 0;
			goto return_response;
		}
		strcpy(goldenImageHash, vm_manifest_hash);

        //mount the disk, then pass the path and manifest file for measurement to MA(Measurement Agent)
        sprintf(mount_path,"%s%s", g_mount_path, vm_uuid);
        //create a directory under /mnt/vrtm/VM_UUID to mount the VM disk
        LOG_DEBUG("Mount location : %s", mount_path);
        if ( mkdir(mount_path,766) != 0 && errno != EEXIST ) {
        	LOG_ERROR("can't create directory to mount the image ");
        	start_app_status = 1;
        	goto return_response;
        }
        /*
         * call mount script to mount the VM disk as :
         * ../scripts/mount_vm_image.sh <disk> <mount_path>
         */
        sprintf(command, mount_script " %s %s > %s/%s 2>&1", disk_file, mount_path, vm_manifest_dir, ma_log);
        LOG_DEBUG("Command to mount the image : %s", command);
        i = system(command);
        LOG_DEBUG("system call to mount image exit status : %d", i);
        if ( i != 0) {
        	LOG_ERROR("Error in mounting the image for measurement. For more info please look into file %s/%s", vm_manifest_dir, ma_log);
        	start_app_status = 1;
        	goto return_response;
        }
        LOG_DEBUG("Image Mounted successfully");
        /*
         * call MA to measure the VM as :
         * ./verfier manifestlist.xml MOUNT_LOCATION IMVM
         */
        sprintf(command, "./verifier %s %s/mount/ IMVM >> %s/%s 2>&1", nohash_manifest_file, mount_path, vm_manifest_dir, ma_log);
        LOG_DEBUG("Command to launch MA : %s", command);
        i = system(command);
        LOG_DEBUG("system call to verifier exit status : %d", i);
        if ( i != 0 ) {
        	LOG_ERROR("Measurement agent failed to execute successfully. Please check Measurement log in file %s/%s", vm_manifest_dir, ma_log);
        	start_app_status = 1;
        	goto return_response;
        }
        LOG_DEBUG("MA executed successfully");
        /*
         * unmount image by calling mount script with UN_MOUNT mode after the measurement as :
         * ../scripts/mount_vm_image.sh MOUNT_PATH
         */
        sprintf(command, mount_script " %s/mount >> %s/%s 2>&1", mount_path, vm_manifest_dir, ma_log);
        LOG_DEBUG("Command to unmount the image : %s", command);
        i = system(command);
        LOG_DEBUG("system call for unmounting exit status : %d", i);
        if ( i != 0 ) {
        	LOG_ERROR("Error in unmounting the vm image. Please check log file : %s/%s", vm_manifest_dir, ma_log);
        	start_app_status = 1;
        	goto return_response;
        }
        LOG_DEBUG("Unmount of image Successfull");
        // Only call verfier when measurement is required
// Open measurement log file at a specified location
        fq = fopen(cumulativehash_file, "rb");
        if(!fq) 
		{
        	LOG_ERROR("Error returned by verifer in generating cumulative hash, please check Measurement log in file %s/%s\n", vm_manifest_dir, ma_log);
        	//return TCSERVICE_RESULT_FAILED; // measurement failed  (verifier failed to measure)
			start_app_status = 1;
			goto return_response;
		}

        char imageHash[65];
        //int flag=0;

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
            //flag=1;
        }
		else if ((strcmp(launchPolicy, "Audit") == 0)) {
			LOG_INFO("IMVM Verification Failed, but continuing with VM launch as MeasureOnly launch policy is used");
			verification_status = false;
			//flag=1;
		}
		else {
			LOG_ERROR("IMVM Verification Failed, not continuing with VM launch as MeasureAndEnforce launch policy is used");
			verification_status = false;
			//flag=0;
		}

//////////////////////////////////////////////////////////////////////////////////////////////////////

    //The code below does the work of converting 64 byte hex (imageHash) to 32 byte binary (rgHash)
    //same as in rpchannel/channelcoding.cpp:ascii2bin(),
    {
		int c = 0;
		strcpy(vm_manifest_hash, imageHash);
		int len = strlen(imageHash);
		int iSize = 0;
		for (c= 0; c < len; c = c+2) {
			sscanf(&imageHash[c], "%02x", &rgHash[c/2]);
			iSize++;
		}
		LOG_TRACE("Adding proc table entry for measured VM");
		int temp_proc_id = g_myService.m_procTable.getprocIdfromuuid(vm_uuid);
		int vm_data_size = 0;
		char* vm_data[1];
		vm_data[0] = vm_uuid;
		vm_data_size++;
		if ( temp_proc_id == NULL) {
			if(!g_myService.m_procTable.addprocEntry(child, kernel_file, vm_data_size, vm_data, size, rgHash)) {
				LOG_ERROR( "StartApp: cant add to vRTM Map\n");
				//return TCSERVICE_RESULT_FAILED;
				start_app_status = 1;
				goto return_response;
			}
		}
		else {
			child = temp_proc_id;
		}
    }
    LOG_TRACE("Updating proc table entry");
   if(!g_myService.m_procTable.updateprocEntry(child, vm_image_id, vm_customer_id, vm_manifest_hash, vm_manifest_signature,launchPolicy,verification_status, vm_manifest_dir)) {
	   	LOG_ERROR("SartApp : can't update proc table entry\n");
        //return TCSERVICE_RESULT_FAILED;
	   	start_app_status = 1;
	   	goto return_response;
    }

    // free all allocated variable
    
    return_response :
    	if ( !vm_image_id ) free(vm_image_id);
        if ( !vm_customer_id ) free(vm_customer_id);
        if ( !vm_manifest_hash ) free(vm_manifest_hash);
        if ( !vm_manifest_signature ) free(vm_manifest_signature);
		for ( i = 0; i < an; i++) {
			if( av[i] ) {
				free (av[i]);
				av[i] = NULL;
			}
		}
    	if (start_app_status) {
			char remove_file[1024] = {'\0'};
			sprintf(remove_file,"rm -rf %s", vm_manifest_dir);
			system(remove_file);
			*poutsize = sizeof(int);
			*((int*)out) = -1;
    		return TCSERVICE_RESULT_FAILED;
    	}
    	else {
			*poutsize = sizeof(int);
    		if (verification_status == false && (strcmp(launchPolicy, "Enforce") == 0)) {
    			*((int*)out) = -1;
    		}
    		else {
    			*((int*)out) = child;
    		}
			return TCSERVICE_RESULT_SUCCESS;
    	}
}


// ------------------------------------------------------------------------------


bool  serviceRequest(int procid, u32 uReq, int inparamsize, byte* inparams, int *outparamsize, byte* outparams)
{
    int                 an = 0;
    char*               av[32];
    char*               method_name = NULL;
    int 				fr_var;
    bool ret_val = false;

	//outparams[PARAMSIZE] = {0};
	//outparams = (byte *) calloc(1,sizeof(byte)*PARAMSIZE);
    LOG_TRACE("Entering serviceRequest");

    *outparamsize = PARAMSIZE;
    char response[64];
    int response_size = sizeof(int);
	LOG_DEBUG("Input Parameters before switch case : %s",inparams);
    switch(uReq) {

      case VM2RP_STARTAPP:
        if(!decodeVM2RP_STARTAPP(&method_name, &an, 
                    (char**) av, inparams)) {
        	LOG_ERROR( "Failed to decode the input XML for Start App");
            //outparams = NULL;
            *outparamsize = 0;
            ret_val=false;
            goto cleanup;
        }
        if(g_myService.StartApp(procid,an,av,outparamsize,outparams)) {
        	LOG_ERROR("Start App failed. Method name: %s", method_name);
        	//outparams = NULL;
        	*outparamsize = 0;
        	ret_val = false;
                goto cleanup;
        }
        else {
        	//response = *((int *)outparams);
        	//memcpy(response, outparams, response_size);
        	sprintf(response, "%d", *((int *)outparams));
        	response_size = strlen(response);

        	*outparamsize = PARAMSIZE;
        	memset(outparams, 0, *outparamsize);
        }
        *outparamsize = encodeRP2VM_STARTAPP((byte *)response, response_size, *outparamsize, outparams);
        LOG_DEBUG("Encoded resonse : %s", outparams);
        if (*outparamsize < 0 ) {
        	*outparamsize = 0;
			ret_val = false;
			goto cleanup;
        }
        LOG_INFO("Start App successful");
        ret_val = true;
        break;

      case VM2RP_SETVM_STATUS:
 
        LOG_TRACE( "serviceRequest, RP2VM_SETVM_STATUS, decoding");
        if(!decodeVM2RP_SETVM_STATUS(&method_name, &an, (char**) av, inparams)) {
        	LOG_ERROR( "Failed to decode the input XML for Set UUID");
            //outparams = NULL;
            *outparamsize = 0;
            ret_val = false;
            goto cleanup;
        }
        *outparamsize= PARAMSIZE;
        
        LOG_DEBUG("In SET_UUID : params after decoding : %s , %s",av[0], av[1]);
        response_size = sizeof(int);
		if(g_myService.UpdateAppStatus(av[0], atoi(av[1]))
				!=TCSERVICE_RESULT_SUCCESS) {
			LOG_ERROR( "Updating status of VM for UUID : %s failed", av[0]);
			//response = -1;
			//*((int *)response) = -1;
			sprintf(response,"%d", -1);
		}
		else {
			//response = 0;
			//*((int *) response) = 0;
			sprintf(response,"%d", 0);
		}
		response_size = strlen(response);
		LOG_DEBUG("Response : %s response size : %d", response, response_size);
		*outparamsize = encodeRP2VM_SETVM_STATUS((byte *)response, response_size, *outparamsize, outparams);
		LOG_INFO("Encoded Response : %s", outparams);
		if(*outparamsize < 0 ) {
			LOG_ERROR("Failed to send response. Encoded data too small.");
			*outparamsize = 0;
			ret_val = false;
			goto cleanup;
		}
		LOG_INFO("VM Status is updated in vRTM table for UUID %s to %s successfully", av[0], av[1]);
        ret_val = true;
        break;
        
        /*case VM2RP_TERMINATEAPP:
            *outparamsize = 0;
            LOG_TRACE("decoding the input XML for terminate app");
            if(!decodeVM2RP_TERMINATEAPP(&method_name, &an, (char**) av, inparams)) {
                LOG_ERROR( "Failed to decode the input XML for terminate app");
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
	    }*/
	    LOG_INFO( "Deregister VM with vRTM ID %s succussfully", av[0]);
	    ret_val = true;
	    break;

			/***********new API ******************/
        case VM2RP_GETRPID:
        {
        	*outparamsize = 0;
        	if(!decodeRP2VM_GETRPID(&method_name, &an, (char**) av, inparams) || an == 0)
        	{
        		LOG_ERROR( "failed to decode the input XML");
                        ret_val = false;
                        goto cleanup;
        	}
            LOG_DEBUG("Input parameter after decoding. UUID: %s",av[0]);
        	char rpid[16] = {0};
        	int rpidsize=16;
        	if(g_myService.GetRpId(av[0],(byte *)rpid,&rpidsize)!=TCSERVICE_RESULT_SUCCESS)
        	{
        		LOG_ERROR( "Failed to get vRTM ID for given UUID %s", av[0]);
			ret_val = false;
                        goto cleanup;
        	}
            LOG_DEBUG("Found vRTM ID %s for UUID %s",rpid, av[0]);

        	//then encode the result
        	*outparamsize = encodeRP2VM_GETRPID(rpidsize,(byte *)rpid,*outparamsize,outparams);
            LOG_DEBUG("after encode : %s",outparams);
        	if(*outparamsize < 0) {
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
            *outparamsize = 0;
            if(!decodeRP2VM_GETVMMETA(&method_name, &an, (char**) av,inparams) || an == 0)
            {
                LOG_ERROR( "failed to decode the input XML");
                *outparamsize = 0;
                ret_val = false;
                goto cleanup;
            }
            //call to getVMMETA
            LOG_DEBUG("Decoded data vRTM ID : %s", av[0]);
            byte vm_rpcustomerId[CUSTOMER_ID_SIZE];
            byte vm_rpimageId[IMAGE_ID_SIZE];
            byte vm_rpmanifestHash[MANIFEST_HASH_SIZE];
            byte vm_rpmanifestSignature[MANIFEST_SIGNATURE_SIZE];

            int vm_rpimageIdsize, vm_rpcustomerIdsize,vm_rpmanifestHashsize,vm_rpmanifestSignaturesize;
            int in_procid = atoi((char *)av[0]);
            if(g_myService.GetVmMeta(in_procid,vm_rpimageId, &vm_rpimageIdsize,vm_rpcustomerId, &vm_rpcustomerIdsize,
                vm_rpmanifestHash, &vm_rpmanifestHashsize,vm_rpmanifestSignature, &vm_rpmanifestSignaturesize))
            {
                LOG_ERROR( "Failed to send VM meta data, vRTM ID does not exist");
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
            *outparamsize = encodeRP2VM_GETVMMETA(numOfMetaComp,metaMap,*outparamsize,outparams);
            LOG_DEBUG("after encode : %s\n",outparams);
            if(*outparamsize<0) {
                LOG_ERROR("Failed to Send VM Metadata, Encoded data to small");
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
            if(!decodeRP2VM_ISVERIFIED(&method_name, &an, (char**) av,inparams) || an == 0)
            {
                LOG_ERROR( "failed to decode the input XML");
                *outparamsize = 0;
                ret_val = false;
                goto cleanup;
            }
            LOG_DEBUG("outparams after decode : %s",outparams);

            char verificationstat[8];
            int  verificationstatsize=8;
            int verification_status;
            if(g_myService.IsVerified(av[0],&verification_status))
            {
                LOG_ERROR("Failed to send verification status, uuid does not exist");
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
				*outparamsize = 0;
				ret_val = false;
				goto cleanup;
			}
        	LOG_DEBUG( "outparams after decode : %s %s ", av[0], av[1]);

			char vm_manifest_dir[1024];
			if(g_myService.GenerateSAMLAndGetDir(av[0],av[1], vm_manifest_dir))
			{
					LOG_ERROR( "Failed to GET VMREPORT given uuid does not exist");
					*outparamsize = 0;
					ret_val = false;
					goto cleanup;
			}

			int vm_manifest_dir_size = strlen(vm_manifest_dir);
			*outparamsize = encodeRP2VM_GETVMREPORT(vm_manifest_dir_size, (byte *)vm_manifest_dir, *outparamsize, outparams);
			if(outparamsize<0) {
				LOG_ERROR("Failed to Send VM Report and manifest Dir, Encoded data to small");
				*outparamsize = 0;
				ret_val = false;
				goto cleanup;
			}
			LOG_INFO("Successfully send the VM Report and manifest Directory ");
			ret_val = true;
			break;
        }

        default:
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

void* clean_vrtm_table(void *){
	LOG_TRACE("");
	while(g_myService.m_procTable.getcancelledvmcount()) {
		int cleaned_entries;
		sleep(g_entry_cleanup_interval);
		g_myService.CleanVrtmTable(g_cancelled_vm_max_age, VM_STATUS_CANCELLED, &cleaned_entries);
		LOG_INFO("Number of VM entries with cancelled status removed from vRTM table : %d", cleaned_entries);
	}
	g_cleanup_service_status = 0;
	LOG_DEBUG("Cleanup thread exiting...");
	return NULL;
}

int cleanupService() {
	pthread_t tid;
	pthread_attr_t attr;
	LOG_TRACE("");
	if (g_cleanup_service_status == 1) {
		LOG_INFO("Clean-up Service already running");
		return 0;
	}
	pthread_attr_init(&attr);
	if (!pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
		pthread_create(&tid, &attr, clean_vrtm_table, (void *)NULL);
		LOG_INFO("Successfully created the thread for entries cleanup");
		g_cleanup_service_status = 1;
		return 0;
	}
	else {
		LOG_ERROR("Can't set cleanup thread attribute to detatchstate");
		LOG_ERROR("Failed to spawn the vRTM entry clean up thread");
		return 1;
	}
}
