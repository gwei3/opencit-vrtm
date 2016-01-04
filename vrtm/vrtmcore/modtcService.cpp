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
#include "base64.h"
#ifdef __linux__
#include <sys/wait.h> /* for wait */
#include <sys/un.h>
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#ifdef LINUX
#include <linux/un.h>
#endif
#include <errno.h>

#include "tcconfig.h"
#include "tcpchan.h"
#include "vrtm_api_code.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <openssl/buffer.h>

#include <libxml/c14n.h>
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
static int g_cleanup_service_status = 0;


#define NUMPROCENTS 200
#define LAUNCH_ALLOWED		"launch allowed"	 
#define LAUNCH_NOT_ALLOWED	"launch not allowed"
#define KEYWORD_UNTRUSTED	"untrusted"


int cleanupService();
void* clean_vrtm_table(void *p);
// ---------------------------------------------------------------------------

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


int GetProperty(char *propertiesFile, char *reqProperty, char **storeProperty)
{
	char *line;
	int line_size = 512;
	char *key;
	char *value;

	std:: map<std::string, std::string> properties_map;

	FILE *fp = fopen(propertiesFile,"r");
	LOG_TRACE("Loading properties file %s", propertiesFile);
	if(fp == NULL)
	{
		LOG_ERROR("Failed to load properties file %s", propertiesFile);
		return -1;
	}
	while(true)
	{
		line = (char *)calloc(1,sizeof(char) * 512);
		if(line != NULL) {
			fgets(line,line_size,fp);
			if(feof(fp)) {
				free(line);
				break;
			}
			LOG_TRACE("Line read from properties file: %s", line);
			if( line[0] == '#' ) {
				LOG_DEBUG("Comment in properties file : %s", &line[1]);
				free(line);
				continue;
			}
			key=strtok(line,"=");
			value=strtok(NULL,"=");
			if(key != NULL && value != NULL) {
				std::string map_key (key);
				std::string map_value (value);
				LOG_TRACE("Parsed Key=%s and Value=%s", map_key.c_str(), map_value.c_str());
				std::pair<std::string, std::string> properties_pair (trim_copy(map_key," \t\n"),trim_copy(map_value," \t\n"));
				properties_map.insert(properties_pair);
			}
			free(line);
		}
		else {
			LOG_ERROR("Can't allocate memory to read a line");
			properties_map.clear();
			fclose(fp);
			return -1;
		}
	}

	fclose(fp);
	std::string reqValue;
	reqValue = properties_map[reqProperty];
	properties_map.clear();

	*storeProperty = (char *)reqValue.c_str();
	return 0;
}

int extractCert(char *certBuffer) {
	char *line;
	int line_size = 512;

	FILE *fp = fopen("temp.txt", "r");
	if(fp < 0) {
		LOG_ERROR("Unable to open temp.txt file\n");
	    return -1;
	}

	while(true)
	{
		line = (char *)calloc(1,sizeof(char) * line_size);
		if(line != NULL) {
			fgets(line,line_size,fp);
			if(feof(fp)) {
				free(line);
				break;
			}

			if( strstr(line, "BEGIN") || strstr(line, "END")) {
				free(line);
				continue;
			}

			strcat(certBuffer, line);
			free(line);
		}
		else {
			LOG_ERROR("Can't allocate memory to read a line");
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);
	return 0;
}

int appendCert(char *certBuffer) {
	X509 *cert;
	BIO *inbio, *outbio;

	inbio = BIO_new_file("/opt/trustagent/configuration/signingkey.pem", "r");
	outbio = BIO_new_file("temp.txt", "w");

	if(!(cert = PEM_read_bio_X509(inbio, NULL, 0, NULL))) {
		LOG_ERROR("Error loading certificate into memory");
		return -1;
	}

	if(!PEM_write_bio_X509(outbio, cert)) {
		LOG_ERROR("Error writing certificate data in PEM format");
		return -1;
	}

	X509_free(cert);
	BIO_free_all(inbio);
	BIO_free_all(outbio);

	if(extractCert(certBuffer) == -1) {
		LOG_ERROR("Unable to extract Certificate from signingkey.pem");
		return -1;
	}
	return 0;
}

int calculateHash(char *xml_file, char *hash_str) {
	int len;
	char buf[256] = {0};
	unsigned char hash[SHA_DIGEST_LENGTH + 1] = {0};

	FILE *fd = fopen(xml_file, "rb");
  	if(fd < 0){
      	LOG_ERROR("Unable to open file '%s'\n",xml_file);
      	return -1;
  	}
	SHA_CTX sha1;
	SHA1_Init(&sha1);
	while ((len = fread(buf, 1, 256, fd)))
      		SHA1_Update(&sha1, buf, len);
	SHA1_Final(hash, &sha1);

	strcpy(hash_str, (char *)hash);
	fclose(fd);
	return 0;
}

int canonicalizeXml(char *infile, char *outfile) {
	xmlDocPtr Doc;
	Doc = xmlParseFile(infile);
	int nbytes = xmlC14NDocSave(Doc, NULL, 0, NULL, 0, outfile, -1);
	if(nbytes < 0) {
		LOG_ERROR("Unable to canonicalize file '%s'\n",infile);
		return -1;
	}
	xmlFreeDoc(Doc);
	return 0;
}


// Need to get nonce also as input
TCSERVICE_RESULT tcServiceInterface::GenerateSAMLAndGetDir(char *vm_uuid,char *nonce, char* vm_manifest_dir)
{
	char *b64_str;
	char *tpm_str;
	char cert[2048]={0};
	char xmlstr[8192]={0};
	char outfile[200]={0};
	char tempfile[200]={0};
	char filepath[200]={0};
	char command0[400]={0};
	char hash_str[512]={0};
	char signature[1024]={0};
	char reqProperty[100]={0};
	char manifest_dir[400]={0};
	char propertiesFile[100]={0};
	char tpm_signkey_passwd[100]={0};
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
	sprintf(outfile, "%stemp.xml", manifest_dir);

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
	//fclose(fp1);
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


	if(canonicalizeXml(tempfile, outfile) == -1) {
		return TCSERVICE_RESULT_FAILED;
	}
	if(calculateHash(outfile, hash_str) == -1) {
		LOG_ERROR("Unable to calculate hash of file '%s'\n", infile);
		return TCSERVICE_RESULT_FAILED;
	}
	LOG_DEBUG("Calculated Hash : %s", hash_str);
	if(Base64Encode(hash_str, &b64_str) != 0) {
		LOG_ERROR("Unable to encode the calculated hash");
		return TCSERVICE_RESULT_FAILED;
	}
	LOG_DEBUG("Encoded Hash : %s", b64_str);
	fprintf(fp1,"%s", b64_str);
/*
	sprintf(command0,"xmlstarlet c14n  %sus_xml.xml | openssl dgst -binary -sha1  | openssl enc -base64 | xargs echo -n >> %ssigned_report.xml", manifest_dir,manifest_dir);
	LOG_DEBUG("command generated to calculate hash: %s", command0);
	system(command0);
*/

	//fp1 = fopen(filepath,"a");
	sprintf(xmlstr,"</DigestValue></Reference></SignedInfo><SignatureValue>");
	fprintf(fp1,"%s",xmlstr);
    LOG_DEBUG("XML content : %s", xmlstr);
    fclose(fp1);


    // Calculate the Signature Value
	sprintf(tempfile,"%sus_can.xml",manifest_dir);
	fp = fopen(tempfile,"w");
	if (fp == NULL) {
		LOG_ERROR("can't open the file us_can.xml");
		return TCSERVICE_RESULT_FAILED;
	}
	sprintf(xmlstr,"<SignedInfo xmlns=\"http://www.w3.org/2000/09/xmldsig#\"><CanonicalizationMethod Algorithm=\"http://www.w3.org/2001/10/xml-exc-c14n#\"></CanonicalizationMethod><SignatureMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#rsa-sha1\"></SignatureMethod><Reference URI=\"\"><Transforms><Transform Algorithm=\"http://www.w3.org/2000/09/xmldsig#enveloped-signature\"></Transform><Transform Algorithm=\"http://www.w3.org/2001/10/xml-exc-c14n#\"></Transform></Transforms><DigestMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#sha1\"></DigestMethod><DigestValue>");
	fprintf(fp,"%s",xmlstr); 
	//fclose(fp);

	fprintf(fp,"%s", b64_str);
/*
	sprintf(command0,"xmlstarlet c14n  %sus_xml.xml | openssl dgst -binary -sha1  | openssl enc -base64 | xargs echo -n  >> %sus_can.xml", manifest_dir,manifest_dir);
	system(command0);
*/
	//fp = fopen(tempfile,"a");
	sprintf(xmlstr,"</DigestValue></Reference></SignedInfo>");
	fprintf(fp,"%s",xmlstr);
	fclose(fp);


	// Store the TPM signing key password
	strcpy(reqProperty, "signing.key.secret");
	strcpy(propertiesFile, "/opt/trustagent/configuration/trustagent.properties");
	if(GetProperty(propertiesFile, reqProperty, &tpm_str) == -1) {
		LOG_ERROR("Unable to get tpm_signkey_passwd");
		return TCSERVICE_RESULT_FAILED;
	}
	strcpy(tpm_signkey_passwd, tpm_str);
	LOG_DEBUG("Signing Key : %s", tpm_signkey_passwd);
/*
	sprintf(command0,"cat /opt/trustagent/configuration/trustagent.properties | grep signing.key.secret | cut -d = -f 2 | xargs echo -n > %ssign_key_passwd", manifest_dir);
	LOG_DEBUG("TPM signing key password :%s \n", command0);
	system(command0); 
					   
	sprintf(tempfile,"%ssign_key_passwd",manifest_dir);
	fp = fopen(tempfile,"r"); 
	fscanf(fp, "%s", tpm_signkey_passwd);
	fclose(fp);                
*/
				 
	// Sign the XML
	if(canonicalizeXml(infile, outfile) == -1) {
		return TCSERVICE_RESULT_FAILED;
	}
	if(calculateHash(outfile, hash_str) == -1) {
		LOG_ERROR("Unable to calculate hash of file '%s'\n", infile);
		return TCSERVICE_RESULT_FAILED;
	}
	LOG_DEBUG("Calculated Hash : %s", hash_str);
/*
	sprintf(command0,"xmlstarlet c14n %sus_can.xml | openssl dgst -sha1 -binary -out %shash.input",manifest_dir,manifest_dir);
	system(command0);
*/

	sprintf(tempfile,"%shash.input",manifest_dir);
	fp = fopen(tempfile,"wb");
	if ( fp == NULL) {
		LOG_ERROR("can't open the file hash.input");
		return TCSERVICE_RESULT_FAILED;
	}
	fprintf(fp, "%s", hash_str);
	fclose(fp);


	sprintf(command0,"/opt/trustagent/bin/tpm_signdata -i %shash.input -k /opt/trustagent/configuration/signingkey.blob -o %shash.sig -q %s -x",manifest_dir,manifest_dir,tpm_signkey_passwd);
	LOG_DEBUG("Signing Command : %s", command0);
	system(command0);


	sprintf(tempfile,"%shash.sig",manifest_dir);
	fp = fopen(tempfile, "rb");
	if ( fp == NULL) {
		LOG_ERROR("can't open the file hash.sig");
		return TCSERVICE_RESULT_FAILED;
	}
	fgets(signature, 1024, fp);
	fclose(fp);


	if(Base64Encode(signature, &b64_str) != 0) {
		LOG_ERROR("Unable to encode the calculated hash");
		return TCSERVICE_RESULT_FAILED;
	}
	LOG_DEBUG("Encoded Signature : %s", b64_str);

	fp1 = fopen(filepath,"a");
	if (fp1 == NULL) {
		LOG_ERROR("can't open the file signed_report.xml");
		return TCSERVICE_RESULT_FAILED;
	}
	fprintf(fp1,"%s", b64_str);
/*
	sprintf(command0,"openssl enc -base64 -in %shash.sig |xargs echo -n >> %ssigned_report.xml",manifest_dir,manifest_dir); 
	system(command0);
*/

	//fp1 = fopen(filepath,"a");
	sprintf(xmlstr,"</SignatureValue><KeyInfo><X509Data><X509Certificate>");
	LOG_DEBUG("XML content : %s", xmlstr);
	fprintf(fp1,"%s",xmlstr);
	//fclose(fp1);
					

	// Append the X.509 certificate
	if(appendCert(cert) == -1) {
		LOG_ERROR("Unable to append Certificate");
		return TCSERVICE_RESULT_FAILED;
	}
	LOG_DEBUG("Extracted Certificate : %s", cert);
/*
	sprintf(command0,"openssl x509 -in /opt/trustagent/configuration/signingkey.pem -text | awk '/-----BEGIN CERTIFICATE-----/,/-----END CERTIFICATE-----/' |  sed '1d;$d' >> %ssigned_report.xml",manifest_dir);
	LOG_DEBUG("Command to generate certificate : %s", command0);
	system(command0);
*/
					   

	//fp1 = fopen(filepath,"a");
	fprintf(fp1, "%s", cert);
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

int extractDigAlg(char *file, char *extension) {
	char *line;
	int line_size = 512;

	FILE *fp = fopen(file, "r");
	if(fp < 0) {
		LOG_ERROR("ERROR: Unable to open file %s\n", file);
	    return -1;
	}

	while (true) {
		line = (char *)calloc(1,sizeof(char) * line_size);
		if(line != NULL) {
			fgets(line,line_size,fp);

			if (feof(fp)) {
				free(line);
				break;
			}

			if (strstr(line, "DigestAlg=") != NULL){
				tagEntry(strstr(line, "DigestAlg="));
				return 0;
			}
			free(line);
		}
		else {
			LOG_ERROR("Can't allocate memory to read a line");
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);
	return -1;
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
    char	vm_uuid[UUID_SIZE];
    int 	start_app_status = 0;
    char 	command[512]={0};
	FILE*   fp1=NULL;
	char    extension[20]={0};
	char    popen_command[250]={0};
	char    xml_command[]="xmlstarlet sel -t -m \"//@DigestAlg\" -v \".\" -n ";
	char    measurement_file[2048]={0};
	std::string line_str;
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
#ifdef __linux__
				mkdir(trust_report_dir, 0766);
#elif _WIN32
				if (CreateDirectory((LPCSTR)trust_report_dir, NULL) == 0) {
					if (GetLastError() == ERROR_ALREADY_EXISTS) {
						LOG_WARN("Directory already exist");
					}
					else if (GetLastError() == ERROR_PATH_NOT_FOUND) {
						LOG_ERROR("\nIntermediate Directory not found\n");
					}
				}
#endif
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
				//sprintf(popen_command,"%s%s",xml_command,nohash_manifest_file);
				if(extractDigAlg(nohash_manifest_file, extension) == -1) {
					LOG_ERROR("Unable to retrieve Digest Algorithm");
					return TCSERVICE_RESULT_FAILED;
				}
				strcpy(extension, NodeValue);
/*
#ifdef __linux__
				fp1=popen(popen_command,"r");
#elif _WIN32
				fp1 = _popen(popen_command, "r");
#endif
*/
				//fgets(extension, sizeof(extension)-1, fp1);
				sprintf(measurement_file,"%s.%s","/measurement",extension);
/*
#ifdef __linux__
				pclose(fp1);
#elif _WIN32
				_pclose(fp1);
#endif
*/
				if(measurement_file[strlen(measurement_file) - 1] == '\n') 
					measurement_file[strlen(measurement_file) - 1] = '\0';
				LOG_DEBUG("Extension : %s",extension);
				
				sprintf(formatted_manifest_file, "%s%s", trust_report_dir, "/fmanifest.xml");
				LOG_DEBUG("Formatted manifest file %s", formatted_manifest_file);
        		strncpy(cumulativehash_file, manifest_file, strlen(manifest_file)-strlen("/trustpolicy.xml"));
        		sprintf(cumulativehash_file, "%s%s", cumulativehash_file, measurement_file);
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
        FILE *fq ;
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
        //fp=fopen(formatted_manifest_file,"r");
		std::ifstream fp(formatted_manifest_file);
		if (!fp) {
			start_app_status = 1;
			goto return_response;
		}		
		
		while (getline(fp, line_str))  //same as getline(fp, line_str).good() or fp.good()
		{
			line = (char *)malloc(sizeof(char)*line_str.length());
			memset(line, 0, line_str.length());
			strcpy(line, line_str.c_str());			
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
        		//fclose(fp);
        		LOG_INFO("Launch policy is neither Audit nor Enforce so vm verification is not not carried out");
        		//return TCSERVICE_RESULT_SUCCESS;
        		char remove_file[1024] = {'\0'};
				sprintf(remove_file,"rm -rf %s", vm_manifest_dir);
				system(remove_file);
        		start_app_status = 0;
        		goto return_response;
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
				//return TCSERVICE_RESULT_FAILED;
				start_app_status = 1;
				goto return_response;
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
                //return TCSERVICE_RESULT_FAILED;
            	start_app_status = 1;
            	goto return_response;
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
				//return TCSERVICE_RESULT_FAILED;
				start_app_status = 1;
				goto return_response;
			}
				strcpy(vm_customer_id,NodeValue);
          }
		if(strstr(line,"SignatureValue")!= NULL){
				temp = tagEntry(line);
				LOG_DEBUG("<Manifest Signature  =\"%s\">",NodeValue);
				vm_manifest_signature = (char *)malloc(sizeof(char)*(strlen(NodeValue) + 1));
				if(vm_manifest_signature == NULL) {
					LOG_ERROR("StartApp : Error in allocating memory for vm_manifest_hash");
					//return TCSERVICE_RESULT_FAILED;
					start_app_status = 1;
					goto return_response;
				}
			   strcpy(vm_manifest_signature,NodeValue);
			  }
		} // end of file parsing
        free(line);
        line = NULL;
        //fclose(fp);
		fp.close();
// Only call verfier when measurement is required
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
    free(vm_image_id);
    free(vm_customer_id);
    free(vm_manifest_hash);
    free(vm_manifest_signature);
    

    return_response :
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
#ifdef __linux__
		sleep(g_entry_cleanup_interval);
#elif _WIN32
		DWORD g_entry_cleanup_interval_msec = g_entry_cleanup_interval * 1000;
		Sleep(g_entry_cleanup_interval_msec);
#endif
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
