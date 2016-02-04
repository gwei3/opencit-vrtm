

#include "logging.h"
#include "modtcService.h"
#include "channelcoding.h"
#include "xpathparser.h"
#include "base64.h"
#include "loadconfig.h"
#ifdef _WIN32
#include <processthreadsapi.h>
#include <bcrypt.h>
#include <shlwapi.h>
#include "hypervWMIcaller.h"
#elif __linux__
#include <sys/wait.h> /* for wait */
#include <sys/un.h>
#include <unistd.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <openssl/buffer.h>
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

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __linux__
#include "safe_lib.h"
#endif
#ifdef __cplusplus
}
#endif

#include <libxml/c14n.h>
#include <libxml/xmlreader.h>
#include <vector>
#include <map>

int g_sz_uuid;
int g_max_uuid;
tcServiceInterface      g_myService;
int                     g_servicepid= 0;
//extern bool			g_fterminateLoop;
u32                     g_fservicehashValid= false;
u32                     g_servicehashType= 0;
int                     g_servicehashSize= 0;
byte                    g_servicehash[32]= {
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                        };
#ifdef _WIN32
#define power_shell "powershell "
#define power_shell_prereq_command "-noprofile -executionpolicy bypass -file "
#define mount_script_path "/scripts/Mount-EXTVM.ps1"
#elif __linux__
#define mount_script_path "/scripts/mount_vm_image.sh"
#endif

#define ma_log "/measurement.log"
#define stripped_manifest_file "/manifest.xml"
uint32_t	g_rpdomid = 1000;
static int g_cleanup_service_status = 0;
static int g_docker_deletion_service_status = 0;
static int g_hyperv_vm_cleanup_service_status = 0;


#define NUMPROCENTS 200
#define LAUNCH_ALLOWED		"launch allowed"	 
#define LAUNCH_NOT_ALLOWED	"launch not allowed"
#define KEYWORD_UNTRUSTED	"untrusted"

#ifdef _WIN32
#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)
#endif

void cleanupService();
void* clean_vrtm_table(void *p);
void* clean_deleted_docker_instances(void *p);
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
                                    int sizeHash, byte* hash, int instance_type)
{
    LOG_DEBUG("vRTM id : %d file : %s hash size : %d hash : %s, instance type : %d", procid, file, sizeHash, hash, instance_type);
    if(sizeHash>32) {
    	LOG_ERROR("Size of hash is more than 32 bytes. Hash size = %d", sizeHash);
        return false;
    }
    pthread_mutex_lock(&loc_proc_table);
    serviceprocEnt proc_ent;
    //proc_ent.m_procid = procid;
	proc_ent.m_szexeFile = strdup(file);
    proc_ent.m_sizeHash = sizeHash;
	if (instance_type == INSTANCE_TYPE_VM)
		//in case of windows we keep it as started, cause there is no explicit notifier,
		//like in case of KVM(vrtm_listener)
#ifdef _WIN32
		proc_ent.m_vm_status = VM_STATUS_STARTED;
#else
        proc_ent.m_vm_status = VM_STATUS_STOPPED;
#endif
    else
        proc_ent.m_vm_status = VM_STATUS_STARTED;
    strcpy_s(proc_ent.m_uuid, sizeof(proc_ent.m_uuid), av[0]);
    memcpy_s(proc_ent.m_rgHash,RG_HASH_SIZE,hash,sizeHash);
    proc_ent.m_instance_type = instance_type;
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

	remove_dir(table_it->second.m_vm_manifest_dir);
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
    memcpy_s(table_it->second.m_uuid, g_max_uuid, uuid, g_sz_uuid);
	memset(table_it->second.m_vdi_uuid, 0, g_max_uuid);
    memcpy_s(table_it->second.m_vdi_uuid, g_max_uuid, vdi_uuid, g_sz_uuid);
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
	strcpy_s(table_it->second.m_vm_image_id, IMAGE_ID_SIZE, vm_image_id);
	table_it->second.m_size_vm_image_id = strnlen_s(table_it->second.m_vm_image_id, IMAGE_ID_SIZE);
	strcpy_s(table_it->second.m_vm_customer_id, CUSTOMER_ID_SIZE, vm_customer_id);
	table_it->second.m_size_vm_customer_id = strnlen_s(table_it->second.m_vm_customer_id, CUSTOMER_ID_SIZE);
	strcpy_s(table_it->second.m_vm_manifest_signature, MANIFEST_SIGNATURE_SIZE, vm_manifest_signature);
	table_it->second.m_size_vm_manifest_signature = strnlen_s(table_it->second.m_vm_manifest_signature, MANIFEST_SIGNATURE_SIZE);
	strcpy_s(table_it->second.m_vm_manifest_hash, MANIFEST_HASH_SIZE, vm_manifest_hash);
	table_it->second.m_size_vm_manifest_hash = strnlen_s(table_it->second.m_vm_manifest_hash, MANIFEST_HASH_SIZE);
	strcpy_s(table_it->second.m_vm_manifest_dir, MANIFEST_DIR_SIZE, vm_manifest_dir);
	table_it->second.m_size_vm_manifest_dir = strnlen_s(table_it->second.m_vm_manifest_dir, MANIFEST_DIR_SIZE);
	strcpy_s(table_it->second.m_vm_launch_policy, LAUNCH_POLICY_SIZE, launch_policy);
	table_it->second.m_size_vm_launch_policy = strnlen_s(table_it->second.m_vm_launch_policy, LAUNCH_POLICY_SIZE);
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

int serviceprocTable::getactivedockeruuid(std::set<std::string> & uuid_list) {
	pthread_mutex_lock(&loc_proc_table);
	for( proc_table_map::iterator table_it = proc_table.begin(); table_it != proc_table.end() ; table_it++) {
		if (table_it->second.m_instance_type == INSTANCE_TYPE_DOCKER && table_it->second.m_vm_status != VM_STATUS_CANCELLED) {
			uuid_list.insert(std::string(table_it->second.m_uuid));
		}
	}
	pthread_mutex_unlock(&loc_proc_table);
	LOG_DEBUG("Number of active docker instances in vrtm table: %d ", uuid_list.size());
	return uuid_list.size();
}

int serviceprocTable::getactivevmsuuid(std::set<std::string> &active_vms) {
	pthread_mutex_lock(&loc_proc_table);
	for (proc_table_map::iterator table_it = proc_table.begin(); table_it != proc_table.end(); table_it++) {
		if (table_it->second.m_instance_type == INSTANCE_TYPE_VM && table_it->second.m_vm_status != VM_STATUS_CANCELLED) {
			active_vms.insert(std::string(table_it->second.m_uuid));
		}
	}
	pthread_mutex_unlock(&loc_proc_table);
	LOG_DEBUG("Number of VM entries in vRTM table which are not in cancelled state : %d", active_vms.size());
	return active_vms.size();
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
	snprintf((char *)rpidbuf,MAX_LEN,"%d",proc_id);
	*rpidsize = strnlen_s((char *)rpidbuf, MAX_LEN);
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
	memcpy_s(vm_imageId,IMAGE_ID_SIZE,pEnt->m_vm_image_id,pEnt->m_size_vm_image_id + 1);
	LOG_TRACE("VM image id copied : %s",vm_imageId);
	*vm_imageIdsize = pEnt->m_size_vm_image_id ;
	memcpy_s(vm_customerId,CUSTOMER_ID_SIZE,pEnt->m_vm_customer_id,pEnt->m_size_vm_customer_id + 1);
    LOG_TRACE("Customer ID: %s", pEnt->m_vm_customer_id);
	*vm_customerIdsize = pEnt->m_size_vm_customer_id ;
	memcpy_s(vm_manifestHash,MANIFEST_HASH_SIZE,pEnt->m_vm_manifest_hash, pEnt->m_size_vm_manifest_hash + 1);
    LOG_TRACE("Manifest Hash: %s", pEnt->m_vm_manifest_hash);
	*vm_manifestHashsize = pEnt->m_size_vm_manifest_hash ;
	memcpy_s(vm_manifestSignature,MANIFEST_SIGNATURE_SIZE,pEnt->m_vm_manifest_signature,pEnt->m_size_vm_manifest_signature + 1);
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

#ifdef _WIN32
void cleanup_CNG_api_args(BCRYPT_ALG_HANDLE * handle_Alg, BCRYPT_HASH_HANDLE *handle_Hash_object, PBYTE* hashObject_ptr, PBYTE* hash_ptr) {
	int err = 0;
	if (*handle_Alg) {
		BCryptCloseAlgorithmProvider(*handle_Alg, 0);
	}
	if (*handle_Hash_object) {
		BCryptDestroyHash(*handle_Hash_object);
	}
	if (*hashObject_ptr) {
		free(*hashObject_ptr);
	}
	if (*hash_ptr) {
		free(*hash_ptr);
	}
}

int setup_CNG_api_args(BCRYPT_ALG_HANDLE * handle_Alg, BCRYPT_HASH_HANDLE *handle_Hash_object, PBYTE* hashObject_ptr, int * hashObject_size, PBYTE* hash_ptr, int * hash_size) {
	int status = 0;
	DWORD out_data_size;

	// Open algorithm
	status = BCryptOpenAlgorithmProvider(handle_Alg, BCRYPT_SHA1_ALGORITHM, NULL, 0);
	if (!NT_SUCCESS(status)) {
		cleanup_CNG_api_args(handle_Alg, handle_Hash_object, hashObject_ptr, hash_ptr);
		return status;
	}

	//calculate the size of buffer of hashobject
	status = BCryptGetProperty(*handle_Alg, BCRYPT_OBJECT_LENGTH, (PBYTE)hashObject_size, sizeof(DWORD), &out_data_size, 0);
	if (!NT_SUCCESS(status)) {
		cleanup_CNG_api_args(handle_Alg, handle_Hash_object, hashObject_ptr, hash_ptr);
		return status;
	}

	*hashObject_ptr = (PBYTE)malloc(*hashObject_size);
	if (hashObject_ptr == NULL) {
		cleanup_CNG_api_args(handle_Alg, handle_Hash_object, hashObject_ptr, hash_ptr);
		return -1;
	}
	//calculate the size of buffer of hash
	status = BCryptGetProperty(*handle_Alg, BCRYPT_HASH_LENGTH, (PBYTE)hash_size, sizeof(DWORD), &out_data_size, 0);
	if (!NT_SUCCESS(status)) {
		cleanup_CNG_api_args(handle_Alg, handle_Hash_object, hashObject_ptr, hash_ptr);
		return status;
	}
	*hash_ptr = (PBYTE)malloc(*hash_size);
	if (*hash_ptr == NULL) {
		cleanup_CNG_api_args(handle_Alg, handle_Hash_object, hashObject_ptr, hash_ptr);
		return -1;
	}
	//create hashobject 
	status = BCryptCreateHash(*handle_Alg, handle_Hash_object, *hashObject_ptr, *hashObject_size, NULL, 0, 0);
	if (!NT_SUCCESS(status)) {
		cleanup_CNG_api_args(handle_Alg, handle_Hash_object, hashObject_ptr, hash_ptr);
		return status;
	}
	return status;
}

/*
*next_available_logical_dirve(): generate a next valid dirve name in case of windows where drive can be mounted
*return: next available valid Drive letter in char
*/
char next_available_logical_drive() {
	int sizeof_drive_buf = 128;
	char drive_name_present[128];
	memset(drive_name_present, 0, sizeof_drive_buf);
	int total_drive_size = GetLogicalDriveStrings(sizeof_drive_buf, drive_name_present);
	if (total_drive_size == 0) {
		return NULL;
	}
	char drives[24];
	memset(drives, 0, 24);
	int drives_count = 0;
	int i, j = -1;
	for (i = 0; i < total_drive_size; i++) {
		if (drive_name_present[i] == '\0') {
			//drives[drives_count] = (char *)malloc(sizeof(char) * (i - j));
			//memcpy(drives[drives_count], drive_name_present, i - j);
			drives[drives_count] = drive_name_present[j + 1];
			j = i;
			drives_count++;
		}
	}

	char drive_char = NULL, drive_str[2];
	drive_str[0] = 'D';
	drive_str[1] = '\0';
	for (drive_char = 'D'; drive_char <= 90; drive_char++, drive_str[0] = drive_char) {
		if (strstr(drives, drive_str) == NULL) {
			return drive_char;
		}
	}
	return NULL;
}
#endif

int extractCert(char *pem_file, char *certBuffer, int certBuffer_size) {
	char *line;
	int line_size = 4096;

	FILE *fp = fopen(pem_file, "r");
	if(fp == NULL) {
		LOG_ERROR("Unable to open file '%s'\n", pem_file);
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

			strcat_s(certBuffer, certBuffer_size, line);
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

int appendCert(char *certBuffer, char *manifest_dir, int certBuffer_size) {
#ifdef _WIN32
	char infile[1048] = { 0 };
	char outfile[1048] = { 0 };
	char command[2304] = { 0 };

	snprintf(infile, sizeof(infile), "%stemp.der", manifest_dir);
	snprintf(outfile, sizeof(outfile), "%stemp.pem", manifest_dir);
	snprintf(command, sizeof(command), "CertUtil -decode /opt/trustagent/configuration/signingkey.pem %s && CertUtil -encode %s %s", infile, infile, outfile);
	LOG_DEBUG("CertUtil command : %s", command);

	FILE *fp = _popen(command, "r");
	if (fp != NULL) {
		_pclose(fp);
	}
#elif __linux__
	X509 *cert;
	BIO *inbio, *outbio;

	inbio = BIO_new_file("/opt/trustagent/configuration/signingkey.pem", "r");
	outbio = BIO_new_file("temp.pem", "w");

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
#endif
	if (extractCert(outfile, certBuffer, certBuffer_size) != 0) {
		LOG_ERROR("Unable to extract Certificate from signingkey.pem");
		return -1;
	}
	return 0;
}

int calculateHash(char *xml_file, char *hash_str, int hash_str_size) {
	const int bufSize = 65000;
	char *buffer = (char *)malloc(bufSize);
	int bytesRead = 0;
	if (!buffer) return -1;

	FILE *fd = fopen(xml_file, "rb");
	if (fd == NULL){
		LOG_ERROR("Unable to open file '%s'\n", xml_file);
		return -1;
	}

#ifdef _WIN32
	BCRYPT_ALG_HANDLE       handle_Alg = NULL;
	BCRYPT_HASH_HANDLE      handle_Hash_object = NULL;
	NTSTATUS                status = STATUS_UNSUCCESSFUL;
	int						out_data_size = 0,
		hash_size = 0,
		hashObject_size = 0;
	PBYTE                   hashObject_ptr = NULL;
	PBYTE                   hash_ptr = NULL;

	status = setup_CNG_api_args(&handle_Alg, &handle_Hash_object, &hashObject_ptr, &hashObject_size, &hash_ptr, &hash_size);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Could not inititalize CNG args Provider : 0x%x", status);
		return -1;
	}
	while ((bytesRead = fread(buffer, 1, bufSize, fd))) {
		// calculate hash of bytes read
		status = BCryptHashData(handle_Hash_object, (PUCHAR)buffer, bytesRead, 0);
		if (!NT_SUCCESS(status)) {
			cleanup_CNG_api_args(&handle_Alg, &handle_Hash_object, &hashObject_ptr, &hash_ptr);
			return -1;
		}
	}

	//Dump the hash in variable and finish the Hash Object handle
	status = BCryptFinishHash(handle_Hash_object, hash_ptr, hash_size, 0);
	strncpy_s(hash_str, hash_str_size, (char *)hash_ptr, 20);
	cleanup_CNG_api_args(&handle_Alg, &handle_Hash_object, &hashObject_ptr, &hash_ptr);
#elif __linux__
	unsigned char hash[SHA_DIGEST_LENGTH];
	SHA_CTX sha1;
	SHA1_Init(&sha1);
	const int bufSize = 32768;
	char *buffer = (char *)malloc(bufSize);

	int bytesRead = 0;
	if (!buffer) return -1;
	while ((bytesRead = fread(buffer, 1, bufSize, fd)))
		SHA1_Update(&sha1, buffer, bytesRead);
	SHA1_Final(hash, &sha1);

	strncpy_s(hash_str, MAX_LEN, (char *)hash, SHA_DIGEST_LENGTH);
#endif
	fclose(fd);
	free(buffer);
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
TCSERVICE_RESULT tcServiceInterface::GenerateSAMLAndGetDir(char *vm_uuid, char *nonce, char* vm_manifest_dir)
{
	char *b64_str;
	char cert[2048]={0};
	char xmlstr[8192]={0};
	char outfile[1048]={0};
	char tempfile[1048]={0};
	char filepath[1048]={0};
	char command0[2304]={0};
	char manifest_dir[1024]={0};
	char hash_str[512]={0};
	char signature[1024]={0};
	char propertiesFile[100]={0};
	char tpm_signkey_passwd[100]={0};
	FILE * fp = NULL;
	FILE * fp1 = NULL;
	
	std::map<std::string, std::string> properties_map;

    LOG_DEBUG("Generating SAML Report for UUID: %s and getting manifest Directory against nonce : %s", vm_uuid, nonce);
	
	int proc_id = m_procTable.getprocIdfromuuid(vm_uuid);
	if (proc_id == NULL) {
		LOG_ERROR("UUID : %s is not registered with vRTM\n", vm_uuid);
		return TCSERVICE_RESULT_FAILED;
	}
	serviceprocEnt * pEnt = m_procTable.getEntfromprocId(proc_id);
	if ( pEnt != NULL) {
		LOG_INFO("Match found for given UUID \n");
		if( pEnt->m_vm_status == VM_STATUS_STOPPED) {
			LOG_INFO("Can't generate report. VM with UUID : %s is in stopped state.");
			return TCSERVICE_RESULT_FAILED;
		}
#ifdef _WIN32
		else if (pEnt->m_vm_status == VM_STATUS_STARTED) {
			LOG_INFO("Current status of VM in vrtm is running, will quickly confirm once with hyper-v");
			//TODO: MAKE a api call to check the status of VM on hyper-v wheather it is active or not
			std::map<std::string, int> vm_in_ques;
			vm_in_ques.insert(std::pair<std::string, int>(pEnt->m_uuid, VM_STATUS_STARTED));
			if (get_hyperv_vms_status(vm_in_ques) == 0) {
				if (vm_in_ques[pEnt->m_uuid] == VM_STATUS_STOPPED) {
					LOG_INFO("Actual status of VM is stopped, So report will not be generated");
					vm_in_ques.clear();
					return TCSERVICE_RESULT_FAILED;
				}
			}
			else {
				LOG_ERROR("Can't get the latest status of VM with WMI call");
				LOG_WARN("vrtm might be sending stale or false report");
				vm_in_ques.clear();
			}
		}
#endif
	}
	snprintf(vm_manifest_dir, MANIFEST_DIR_SIZE, "%s%s/", g_trust_report_dir, vm_uuid); 
	LOG_DEBUG("Manifest Dir : %s", vm_manifest_dir);
	strcpy_s(manifest_dir, sizeof(manifest_dir), vm_manifest_dir);
	snprintf(outfile, sizeof(outfile), "%stemp.xml", manifest_dir);

	// Generate Signed  XML  in same vm_manifest_dir
	//snprintf(manifest_dir,sizeof(manifest_dir),"/var/lib/nova/instances/%s/",vm_uuid);
	snprintf(filepath, sizeof(filepath), "%ssigned_report.xml", manifest_dir);

	fp1 = fopen(filepath,"w");
	if (fp1 == NULL) {
		LOG_ERROR("Can't write report in signed_report.xml file");
		return TCSERVICE_RESULT_FAILED;
	}
	snprintf(xmlstr,sizeof(xmlstr),"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
	fprintf(fp1,"%s",xmlstr);
    LOG_DEBUG("XML content : %s", xmlstr);

	snprintf(xmlstr,sizeof(xmlstr),"<VMQuote><nonce>%s</nonce><vm_instance_id>%s</vm_instance_id><digest_alg>%s</digest_alg><cumulative_hash>%s</cumulative_hash><Signature xmlns=\"http://www.w3.org/2000/09/xmldsig#\"><SignedInfo><CanonicalizationMethod Algorithm=\"http://www.w3.org/2001/10/xml-exc-c14n#\"/><SignatureMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#rsa-sha1\"/><Reference URI=\"\"><Transforms><Transform Algorithm=\"http://www.w3.org/2000/09/xmldsig#enveloped-signature\"/><Transform Algorithm=\"http://www.w3.org/2001/10/xml-exc-c14n#\"/></Transforms><DigestMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#sha1\"/><DigestValue>",nonce, vm_uuid,"SHA256", pEnt->m_vm_manifest_hash);
	fprintf(fp1,"%s",xmlstr);
	//fclose(fp1);
    LOG_DEBUG("XML content : %s", xmlstr);

	// Calculate the Digest Value       
	snprintf(xmlstr,sizeof(xmlstr),"<VMQuote><nonce>%s</nonce><vm_instance_id>%s</vm_instance_id><digest_alg>%s</digest_alg><cumulative_hash>%s</cumulative_hash></VMQuote>",nonce, vm_uuid,"SHA256", pEnt->m_vm_manifest_hash);
	snprintf(tempfile,sizeof(tempfile),"%sus_xml.xml",manifest_dir);
	fp = fopen(tempfile,"w");
	if (fp == NULL) {
		LOG_ERROR("can't open the file us_xml.xml");
		return TCSERVICE_RESULT_FAILED;
	}
	fprintf(fp,"%s",xmlstr);
	fclose(fp);


	if(canonicalizeXml(tempfile, outfile) != 0) {
		return TCSERVICE_RESULT_FAILED;
	}
	if(calculateHash(outfile, hash_str, sizeof(hash_str)) != 0) {
		LOG_ERROR("Unable to calculate hash of file '%s'\n", tempfile);
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
	snprintf(command0,sizeof(command0),"xmlstarlet c14n  %sus_xml.xml | openssl dgst -binary -sha1  | openssl enc -base64 | xargs echo -n >> %ssigned_report.xml", manifest_dir,manifest_dir);
	LOG_DEBUG("command generated to calculate hash: %s", command0);
	system(command0);
*/

	//fp1 = fopen(filepath,"a");
	snprintf(xmlstr,sizeof(xmlstr),"</DigestValue></Reference></SignedInfo><SignatureValue>");
	fprintf(fp1,"%s",xmlstr);
    LOG_DEBUG("XML content : %s", xmlstr);
    //fclose(fp1);


    // Calculate the Signature Value
	snprintf(tempfile, sizeof(tempfile), "%sus_can.xml", manifest_dir);
	fp = fopen(tempfile,"w");
	if (fp == NULL) {
		LOG_ERROR("can't open the file us_can.xml");
		return TCSERVICE_RESULT_FAILED;
	}
	snprintf(xmlstr,sizeof(xmlstr),"<SignedInfo xmlns=\"http://www.w3.org/2000/09/xmldsig#\"><CanonicalizationMethod Algorithm=\"http://www.w3.org/2001/10/xml-exc-c14n#\"></CanonicalizationMethod><SignatureMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#rsa-sha1\"></SignatureMethod><Reference URI=\"\"><Transforms><Transform Algorithm=\"http://www.w3.org/2000/09/xmldsig#enveloped-signature\"></Transform><Transform Algorithm=\"http://www.w3.org/2001/10/xml-exc-c14n#\"></Transform></Transforms><DigestMethod Algorithm=\"http://www.w3.org/2000/09/xmldsig#sha1\"></DigestMethod><DigestValue>");
	fprintf(fp,"%s",xmlstr); 
	//fclose(fp);

	fprintf(fp,"%s", b64_str);
/*
	snprintf(command0,sizeof(command0),"xmlstarlet c14n  %sus_xml.xml | openssl dgst -binary -sha1  | openssl enc -base64 | xargs echo -n  >> %sus_can.xml", manifest_dir,manifest_dir);
	system(command0);
*/
	//fp = fopen(tempfile,"a");
	snprintf(xmlstr,sizeof(xmlstr),"</DigestValue></Reference></SignedInfo>");
	fprintf(fp,"%s",xmlstr);
	fclose(fp);

	snprintf(command0,sizeof(command0),"tagent config \"signing.key.secret\" > %ssign_key_passwd", manifest_dir);
	LOG_DEBUG("TPM signing key password :%s \n", command0);
	system(command0); 					   
	snprintf(tempfile, sizeof(tempfile), "%ssign_key_passwd",manifest_dir);
	fp = fopen(tempfile,"r");
	if ( fp == NULL) {
		LOG_ERROR("can't open the file sign_key_passwd");
		return TCSERVICE_RESULT_FAILED;
	}
	//fscanf(fp, "%%%ds", sizeof(tpm_signkey_passwd),tpm_signkey_passwd);
	fgets( tpm_signkey_passwd, sizeof(tpm_signkey_passwd), fp);
	tpm_signkey_passwd[ sizeof(tpm_signkey_passwd) - 1 ] = '\0';
	fclose(fp);
	// to remove the newline character at the end
	if ( tpm_signkey_passwd[strnlen_s(tpm_signkey_passwd, sizeof(tpm_signkey_passwd)) - 1 ] == '\n' ) {
		tpm_signkey_passwd[strnlen_s(tpm_signkey_passwd, sizeof(tpm_signkey_passwd)) - 1 ] = '\0';
	}
	snprintf(tempfile, sizeof(tempfile), "%sus_can.xml",manifest_dir);				 
	// Sign the XML
	if(canonicalizeXml(tempfile, outfile) != 0) {
		return TCSERVICE_RESULT_FAILED;
	}
	if (calculateHash(outfile, hash_str, sizeof(hash_str)) != 0) {
		LOG_ERROR("Unable to calculate hash of file '%s'\n", tempfile);
		return TCSERVICE_RESULT_FAILED;
	}
	LOG_DEBUG("Calculated Hash : %s", hash_str);
/*
	snprintf(command0,sizeof(command0),"xmlstarlet c14n %sus_can.xml | openssl dgst -sha1 -binary -out %shash.input",manifest_dir,manifest_dir);
	system(command0);
*/

	snprintf(tempfile,sizeof(tempfile),"%shash.input",manifest_dir);
	fp = fopen(tempfile,"wb");
	if ( fp == NULL) {
		LOG_ERROR("can't open the file hash.input");
		return TCSERVICE_RESULT_FAILED;
	}
	fprintf(fp, "%s", hash_str);
	fclose(fp);

	snprintf(command0,sizeof(command0),"/opt/trustagent/bin/tpm_signdata -i %shash.input -k /opt/trustagent/configuration/signingkey.blob -o %shash.sig -q %s -x",manifest_dir,manifest_dir,tpm_signkey_passwd);
	LOG_DEBUG("Signing Command : %s", command0);
	system(command0);


	snprintf(tempfile,sizeof(tempfile),"%shash.sig",manifest_dir);
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

	//fp1 = fopen(filepath,"a");
	fprintf(fp1, "%s", b64_str);
	//fclose(fp1);
/*
	snprintf(command0,sizeof(command0),"openssl enc -base64 -in %shash.sig |xargs echo -n >> %ssigned_report.xml",manifest_dir,manifest_dir); 
	system(command0);
*/

	//fp1 = fopen(filepath,"a");
	snprintf(xmlstr,sizeof(xmlstr),"</SignatureValue><KeyInfo><X509Data><X509Certificate>");
	LOG_DEBUG("XML content : %s", xmlstr);
	fprintf(fp1,"%s",xmlstr);
	//fclose(fp1);
					

	// Append the X.509 certificate
	if(appendCert(cert, manifest_dir, sizeof(cert)) != 0) {
		LOG_ERROR("Unable to append Certificate");
		return TCSERVICE_RESULT_FAILED;
	}
	LOG_DEBUG("Extracted Certificate : %s", cert);
/*
	snprintf(command0,sizeof(command0),"openssl x509 -in /opt/trustagent/configuration/signingkey.pem -text | awk '/-----BEGIN CERTIFICATE-----/,/-----END CERTIFICATE-----/' |  sed '1d;$d' >> %ssigned_report.xml",manifest_dir);
	LOG_DEBUG("Command to generate certificate : %s", command0);
	system(command0);
*/
					   

	//fp1 = fopen(filepath,"a");
	fprintf(fp1, "%s", cert);
	snprintf(xmlstr,sizeof(xmlstr),"</X509Certificate></X509Data></KeyInfo></Signature></VMQuote>");
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
	char uuid[UUID_SIZE] = {0};
	char vuuid[UUID_SIZE] = {0};
	int  rp_id = -1; 
	if ((str_rp_id == NULL) || (in_uuid == NULL) || (out == NULL)){
		LOG_ERROR("Can't Register UUID with vRTM either vRTM ID or UUID is NULL");
 		return TCSERVICE_RESULT_FAILED;
	}
	rp_id = atoi(str_rp_id);
	int inuuid_len = strnlen_s(in_uuid, g_max_uuid);
	int invdiuuid_len = strnlen_s(vdi_uuid, g_max_uuid);
	memset(uuid, 0, g_max_uuid);
    memcpy_s(uuid, g_max_uuid, in_uuid, inuuid_len);
	memset(vuuid, 0, g_max_uuid);	
	memcpy_s(vuuid, g_max_uuid, vdi_uuid, invdiuuid_len);
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
	if ( procEnt != NULL ) {
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
		LOG_INFO("Current VM status : %d", procEnt->m_vm_status);
		procEnt->m_status_upadation_time = time(NULL);
		struct tm * cur_time_ptr = localtime(&(procEnt->m_status_upadation_time));
		if ( cur_time_ptr != NULL ) {
			struct tm cur_time = *cur_time_ptr;
			LOG_DEBUG("Status updation time = %d : %d : %d : %d : %d : %d", cur_time.tm_year, cur_time.tm_mon,
					cur_time.tm_mday, cur_time.tm_hour, cur_time.tm_min, cur_time.tm_sec);
		}
		else {
			return TCSERVICE_RESULT_FAILED;
		}
		procEnt->m_vm_status = status;
		LOG_INFO("Successfully updated the VM with UUID : %s status to %d", uuid, status);
		return TCSERVICE_RESULT_SUCCESS;
	}
	return TCSERVICE_RESULT_FAILED;
}


TCSERVICE_RESULT 	tcServiceInterface::CleanVrtmTable(unsigned long entry_max_age, int vm_status, int* deleted_entries) {
	*deleted_entries = 0;
	std::vector<int> keys_to_del;
	pthread_mutex_lock(&m_procTable.loc_proc_table);
	for( proc_table_map::iterator table_it = m_procTable.proc_table.begin(); table_it != m_procTable.proc_table.end(); table_it++ ){
		LOG_DEBUG("Comparing Current entry VM status : %d against vm status : %d ", table_it->second.m_vm_status, vm_status);
		if(table_it->second.m_vm_status == vm_status ) {
			unsigned long entry_age = difftime(time(NULL), table_it->second.m_status_upadation_time);
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

TCSERVICE_RESULT 	tcServiceInterface::CleanVrtmTable(std::set<std::string> & uuid_list, int* deleted_entries) {
	FILE *fp = NULL;
	*deleted_entries = 0;
	char command[65] = {0};
	char *line;
	int line_size = 65;

	snprintf(command, sizeof(command), "docker ps -q --no-trunc");
	LOG_DEBUG("Docker command : %s", command);
#ifdef _WIN32
	fp=_popen(command,"r");
#elif __linux__
	fp=popen(command,"r");
#endif
	if (fp != NULL) {
		while(true) {
			line = (char *) calloc(1,sizeof(char) * line_size);
			fgets(line,line_size,fp);
			if(feof(fp)) {
				free(line);
				break;
			}
			if(line[0] != '\n') {
				LOG_DEBUG("Running Docker container Id : %s",line);
				uuid_list.erase(std::string(line));
			}
			free(line);
		}
#ifdef _WIN32
		_pclose(fp);
#elif __linux__
		pclose(fp);
#endif
	}

	for(std::set<std::string>::iterator iter = uuid_list.begin() ; iter != uuid_list.end(); iter++) {
		snprintf(command, sizeof(command), "%s", (*iter).c_str());
		LOG_DEBUG("Entry to be removed : %s", command);
		if(m_procTable.removeprocEntry(command))
			(*deleted_entries)++;
	}
	return TCSERVICE_RESULT_SUCCESS;
}

TCSERVICE_RESULT tcServiceInterface::CleanVrtmTable_and_update_vm_status(std::set<std::string> & vms, int* deleted_vm_count, int *inactive) {
#ifdef _WIN32
	std::map<std::string, int> hyperv_vms;
	for (std::set<std::string>::iterator iter = vms.begin(); iter != vms.end(); iter++) {
		//all vm are in started or running state in vrtm for now
		hyperv_vms.insert(std::pair<std::string, int>((*iter), VM_STATUS_UNKNOWN));
	}
	//TODO: make api call to find actual state of VMs
	if (get_hyperv_vms_status(hyperv_vms) == 0) {
	//iterate of all VM, if their actual state is deleted remove them and if it stopped update their status
		for (std::map<std::string, int>::iterator iter = hyperv_vms.begin(); iter != hyperv_vms.end(); iter++) {
			if (iter->second == VM_STATUS_DELETED) {
				//VM is deleted or not present on hyper-v
				std::vector<char> vm_uuid(iter->first.begin(), iter->first.end());
				vm_uuid.push_back('\0');
				if (m_procTable.removeprocEntry(&vm_uuid[0]) == true) {
					LOG_INFO("Succesfully removed the entry from vrtm table for VM with uuid : %s", &vm_uuid[0]);
					*deleted_vm_count++;
				}
				else {
					LOG_ERROR("Can't remove the entry from vrtm table for VM with uuid : %s", &vm_uuid[0]);
				}
			}
			else if (iter->second == VM_STATUS_STOPPED) {
				//VM is present but it is not in enabled state on hyper-v
				std::vector<char> vm_uuid(iter->first.begin(), iter->first.end());
				vm_uuid.push_back('\0');
				if (TCSERVICE_RESULT_SUCCESS == UpdateAppStatus(&vm_uuid[0], VM_STATUS_STOPPED)) {
					LOG_INFO("Succesfully updated the status of VM with uuid : %s in vrtm table", &vm_uuid[0]);
					*inactive++;
				}
				else {
					LOG_ERROR("Can't update the status of VM with UUID: %s", &vm_uuid[0]);
				}
			}
			else {
				LOG_DEBUG("VM with UUID : %s is in running state", iter->first.c_str());
			}
		}
	}
	else {
		LOG_ERROR("Couldn't get VM state with WMI call, will try again after %d time", g_entry_cleanup_interval);
		hyperv_vms.clear();
		return TCSERVICE_RESULT_FAILED;
	}
	hyperv_vms.clear();
#endif
	return TCSERVICE_RESULT_SUCCESS;
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
	byte    rgHash[SHA256DIGESTBYTESIZE + 1] = { '\0' };
    int     child= 0;
    int     i;
    char    kernel_file[1024] = {0};
    char    ramdisk_file[1024] = {0};
    char    disk_file[1024] = {0};
    char    manifest_file[1024] = {0};
    char    nohash_manifest_file[1024] = {0};
    char    cumulativehash_file[1024] = {0};
    char*   config_file = NULL;
    char *  vm_image_id = NULL;
    char*   vm_customer_id = NULL;
    char*   vm_manifest_hash = NULL;
    char*   vm_manifest_signature = NULL;
	char*	launch_policy_buff = NULL;
	char*	digest_alg_buff = NULL;
    char    vm_manifest_dir[1024] ={0};
    bool 	verification_status = false;
    char	vm_uuid[UUID_SIZE] = {'\0'};
    int 	start_app_status = 0;
    char 	command[2304]={0};
	FILE*   fp1=NULL;
	char    extension[10]={0};
	char    popen_command[1048]={0};
	char    xml_command[]="xmlstarlet sel -t -m \"//@DigestAlg\" -v \".\" -n ";
	char    measurement_file[2048]={0};
	bool	keep_measurement_log = false;
	int		verifier_exit_status=1;
   //create domain process shall check the whitelist
	child = procid;
	char 	mount_path[128];
	char	mount_script[128];
	int 	instance_type = INSTANCE_TYPE_VM;

#ifdef _WIN32
	//STARTUPINFO si;
	//PROCESS_INFORMATION pi;
	char next_logical_drive_char;
	int sleep_count = 0;
	// dirctory which will be the working directory of powershell
	//char current_dir_of_power_shell[] = "./";
#endif

//	char * nohash_manifest_file ="/root/nohash_manifest.xml"; // Need to be passed by policy agent
	char launchPolicy[10] = {'\0'};
	char imageHash[65] = {'\0'};
	char goldenImageHash[65] = {'\0'};
	FILE *fq ;
	std::map<xmlChar *, char *> xpath_map;

	xmlChar namespace_list[] =			"a=mtwilson:trustdirector:policy:1.1 b=http://www.w3.org/2000/09/xmldsig#";
	xmlChar xpath_customer_id[] = 		"/a:TrustPolicy/a:Director/a:CustomerId";
	xmlChar xpath_launch_policy[] = 	"/a:TrustPolicy/a:LaunchControlPolicy";
	xmlChar xpath_image_id[] = 			"/a:TrustPolicy/a:Image/a:ImageId";
	xmlChar xpath_image_hash[] = 		"/a:TrustPolicy/a:Image/a:ImageHash";
	xmlChar xpath_image_signature[] = 	"/a:TrustPolicy/b:Signature/b:SignatureValue";
	xmlChar xpath_digest_alg[] =		"/a:TrustPolicy/a:Whitelist/@DigestAlg";


    LOG_TRACE("Start VM App");
    if(an>30) {
    	LOG_ERROR("Number of arguments passed are more than limit 30");
        return TCSERVICE_RESULT_FAILED;
    }

    for ( i = 0; i < an; i++) {

        LOG_TRACE( "arg parsing %d \n", i);
        if( av[i] && strcmp(av[i], "-kernel") == 0 ){
            strcpy_s(kernel_file, sizeof(kernel_file), av[++i]);
            LOG_DEBUG("Kernel File Name : %s", kernel_file);
        }

        if( av[i] && strcmp(av[i], "-ramdisk") == 0 ){
            strcpy_s(ramdisk_file, sizeof(ramdisk_file), av[++i]);
            LOG_DEBUG("RAM disk : %s", ramdisk_file);
        }

        if( av[i] && strcmp(av[i], "-config") == 0 ){
            config_file = av[++i];
            LOG_DEBUG("config file : %s",config_file);
        }

        if( av[i] && strcmp(av[i], "-uuid") == 0 ){
        	strcpy(vm_uuid, av[++i]);
        	LOG_DEBUG("uuid : %s",vm_uuid);
        }

        if( av[i] && strcmp(av[i], "-disk") == 0 ){
        	strcpy_s(disk_file, sizeof(disk_file), av[++i]);
            LOG_DEBUG("Disk : %s",disk_file );
        }

        if ( av[i] && strcmp(av[i], "-docker_instance") == 0) {
        	instance_type = INSTANCE_TYPE_DOCKER;
        	LOG_DEBUG("Instance type : Docker instance, %d", instance_type);
        }
        if ( av[i] && strcmp(av[i], "-mount_path") == 0) {
        	strcpy(mount_path, av[++i]);
        	LOG_DEBUG("Mounted image path : %s", mount_path);
        }
    }

	if(vm_uuid[0] == 0) {
		LOG_ERROR("uuid is not present");
		return TCSERVICE_RESULT_FAILED;
	}


        snprintf(vm_manifest_dir, sizeof(vm_manifest_dir), "%s%s", g_trust_report_dir, vm_uuid);
		LOG_DEBUG("VM Manifest Dir : %s", vm_manifest_dir);
		snprintf(manifest_file, sizeof(manifest_file), "%s%s", vm_manifest_dir, "/trustpolicy.xml");
		LOG_DEBUG("Manifest path %s ", manifest_file);
		snprintf(nohash_manifest_file, sizeof(nohash_manifest_file), "%s%s", vm_manifest_dir, "/manifest.xml");
		LOG_DEBUG("Manifest list path 2%s\n",nohash_manifest_file);
#ifdef _WIN32
		if (PathFileExists(manifest_file)==0){
			LOG_ERROR("trustpolicy.xml doesn't exist at  %s", manifest_file);
			LOG_ERROR( "cant continue without reading trustpolicy values");
			start_app_status = 1;
			goto return_response;
		}

		if (PathFileExists(nohash_manifest_file)==0){
			LOG_ERROR("manifestlist.xml doesn't exist at  %s", nohash_manifest_file);
			LOG_ERROR( "cant continue without reading digest algorithm");
			start_app_status = 1;
			goto return_response;
		}
#elif __linux__
		if(access(manifest_file, F_OK)!=0){
			LOG_ERROR("trustpolicy.xml doesn't exist at  %s", manifest_file);
			LOG_ERROR( "cant continue without reading trustpolicy values");
			start_app_status = 1;
			goto return_response;
		}

		if(access(nohash_manifest_file, F_OK)!=0){
			LOG_ERROR("manifestlist.xml doesn't exist at  %s", nohash_manifest_file);
			LOG_ERROR( "cant continue without reading digest algorithm");
			start_app_status = 1;
			goto return_response;
		}
#endif
    	/*
    	 * extract Launch Policy, CustomerId, ImageId, VM hash, and Manifest signature value from formatted manifestlist.xml
    	 * by specifying fixed xpaths with namespaces
    	 */
    	launch_policy_buff = (char *)calloc(1, sizeof(char)* LARGE_CHAR_ARR_SIZE);
    	vm_customer_id = (char *)calloc(1, sizeof(char) * LARGE_CHAR_ARR_SIZE);
    	vm_image_id = (char *) calloc(1, sizeof(char) * LARGE_CHAR_ARR_SIZE);
    	vm_manifest_hash = (char *) calloc(1, sizeof(char)* LARGE_CHAR_ARR_SIZE);
    	vm_manifest_signature = (char *) calloc(1, sizeof(char) * LARGE_CHAR_ARR_SIZE);
		digest_alg_buff = (char *)calloc(1, sizeof(char) * LARGE_CHAR_ARR_SIZE);

    	if ( launch_policy_buff && vm_customer_id && vm_image_id && vm_manifest_hash && vm_manifest_signature && digest_alg_buff) {
    		xpath_map.insert(std::pair<xmlChar *, char *>(xpath_launch_policy, launch_policy_buff));
    		xpath_map.insert(std::pair<xmlChar*, char *>(xpath_customer_id, vm_customer_id));
    		xpath_map.insert(std::pair<xmlChar*, char *>(xpath_image_id, vm_image_id));
    		xpath_map.insert(std::pair<xmlChar*, char *>(xpath_image_hash, vm_manifest_hash));
    		xpath_map.insert(std::pair<xmlChar*, char *>(xpath_image_signature, vm_manifest_signature));
			xpath_map.insert(std::pair<xmlChar*, char *>(xpath_digest_alg, digest_alg_buff));
			if (TCSERVICE_RESULT_FAILED == get_xpath_values(xpath_map, namespace_list, manifest_file)) {
	    		LOG_ERROR("Function get_xpath_values failed");
				//TODO write a remove directory function using dirint.h header file
				start_app_status = 1;
				goto return_response;
			}
			if (strcmp(launch_policy_buff, "MeasureOnly") == 0) {
				strcpy_s(launchPolicy, sizeof(launchPolicy), "Audit");
			}
			else if (strcmp(launch_policy_buff, "MeasureAndEnforce") == 0) {
				strcpy_s(launchPolicy, sizeof(launchPolicy), "Enforce");
			}
			free(launch_policy_buff);
			if (strcmp(launchPolicy, "Audit") != 0 && strcmp(launchPolicy, "Enforce") != 0) {
				LOG_INFO("Launch policy is neither Audit nor Enforce so vm verification is not not carried out");
				remove_dir(vm_manifest_dir);
				start_app_status = 0;
				goto return_response;
			}
			strcpy_s(goldenImageHash, sizeof(goldenImageHash), vm_manifest_hash);			
			strcpy_s(extension, sizeof(extension), digest_alg_buff);
			free(digest_alg_buff);
			LOG_DEBUG("Extension : %s", extension);
			snprintf(cumulativehash_file, sizeof(cumulativehash_file), "%s/measurement.%s", vm_manifest_dir, extension);
			LOG_DEBUG("Cumulative hash file : %s", cumulativehash_file);

		if (instance_type == INSTANCE_TYPE_VM) {
			snprintf(mount_script, sizeof(mount_script), "%s" mount_script_path, g_vrtm_root);
#ifdef _WIN32
			//try to get next available drive letter, if not available wait for 5 sec
			while (1) {
				next_logical_drive_char = next_available_logical_drive();
				if (next_logical_drive_char == NULL && sleep_count < 5) {
					LOG_DEBUG("\n%s", "wait till some drive is unmounted");
					Sleep(1000);
					sleep_count++;
				}
				else if (sleep_count == 5 && next_logical_drive_char == NULL) {
					LOG_ERROR("\n%s", "can't get drive letter for disk");
					start_app_status = 1;
					goto return_response;
				}
				else {
					LOG_DEBUG("\n%s : %c", "next available drive char", next_logical_drive_char);
					snprintf(mount_path, sizeof(mount_path), "%c:", next_logical_drive_char);
					break;
				}
			}

			snprintf(command, sizeof(command), power_shell power_shell_prereq_command "%s -Path %s -DriveLetter %s -Mount > %s%s-%d 2>&1", mount_script, disk_file, mount_path, vm_manifest_dir, ma_log, child);
			LOG_DEBUG("Command to mount the image : %s", command);
			/*ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));
			i = CreateProcess( NULL, command, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, current_dir_of_power_shell, &si, &pi);
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(si.hStdError);
			CloseHandle(si.hStdInput);
			CloseHandle(si.hStdOutput);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);*/
			i = system(command);
			LOG_DEBUG("system call to mount image exit status : %d", i);
			keep_measurement_log = true;
			if (i != 0) {
				//LOG_ERROR("CreateProcess failed (%d).", GetLastError());
				LOG_ERROR("Error in mounting the image for measurement. For more info please look into file %s%s-%d", vm_manifest_dir, ma_log, child);
				start_app_status = 1;
				goto return_response;
			}
#elif __linux__
			//mount the disk, then pass the path and manifest file for measurement to MA(Measurement Agent)
			snprintf(mount_path, sizeof(mount_path), "%s%s-%d", g_mount_path, vm_uuid, child);
			//create a directory under /mnt/vrtm/VM_UUID to mount the VM disk
			LOG_DEBUG("Mount location : %s", mount_path);
			if (make_dir(mount_path) != 0) {
				start_app_status = 1;
				goto return_response;
			}
			/*
			 * call mount script to mount the VM disk as :
			 * <ID> is mount LVMs
			 * ../scripts/mount_vm_image.sh <disk> <mount_path> <ID>
			 */
			snprintf(command, sizeof(command), "%s %s %s %d > %s/%s-%d 2>&1", mount_script, disk_file, mount_path, child, vm_manifest_dir, ma_log, child);
			LOG_DEBUG("Command to mount the image : %s", command);
			i = system(command);
			LOG_DEBUG("system call to mount image exit status : %d", i);
			keep_measurement_log = true;
			if ( i != 0) {
				LOG_ERROR("Error in mounting the image for measurement. For more info please look into file %s/%s-%d", vm_manifest_dir, ma_log, child);
				start_app_status = 1;
				goto return_response;
			}
#endif
			LOG_DEBUG("Image Mounted successfully");
			/*
			 * call MA to measure the VM as :
			 * ./verfier manifestlist.xml MOUNT_LOCATION IMVM
			 */
#ifdef _WIN32
			snprintf(command, sizeof(command), "verifier.exe %s %s IMVM >> %s/%s-%d 2>&1", nohash_manifest_file, mount_path, vm_manifest_dir, ma_log, child);
#elif __linux__
			snprintf(command, sizeof(command), "./verifier %s %s/mount/ IMVM >> %s/%s-%d 2>&1", nohash_manifest_file, mount_path, vm_manifest_dir, ma_log, child);
#endif
			LOG_DEBUG("Command to launch MA : %s", command);
			verifier_exit_status = system(command);
			LOG_DEBUG("system call to verifier exit status : %d", verifier_exit_status);
			if ( verifier_exit_status != 0 ) {
				LOG_ERROR("Measurement agent failed to execute successfully. Please check Measurement log in file %s/%s-%d", vm_manifest_dir, ma_log, child);
			}
			/*
			 * unmount image by calling mount script with UN_MOUNT mode after the measurement as :
			 * ../scripts/mount_vm_image.sh MOUNT_PATH
			 */
#ifdef _WIN32
			snprintf(command, sizeof(command), power_shell power_shell_prereq_command "%s -Path %s -DriveLetter %s -Umount >> %s%s-%d 2>&1", mount_script, disk_file, mount_path, vm_manifest_dir, ma_log, child);
			LOG_DEBUG("Command to unmount the image : %s", command);
			/*ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));
			i = CreateProcess(NULL, command, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, current_dir_of_power_shell, &si, &pi);
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(si.hStdError);
			CloseHandle(si.hStdInput);
			CloseHandle(si.hStdOutput);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);*/ 
			i = system(command);
			LOG_DEBUG("system call for unmounting exit status : %d", i);
			if (i != 0) {
				//LOG_ERROR("CreateProcess failed (%d).", GetLastError());
				LOG_ERROR("Error in unmounting the vm image. Please check log file : %s%s-%d", vm_manifest_dir, ma_log, child);
				start_app_status = 1;
				goto return_response;
			}
#elif __linux__
			snprintf(command, sizeof(command), "%s %s >> %s/%s-%d 2>&1", mount_script, mount_path, vm_manifest_dir, ma_log, child);
			LOG_DEBUG("Command to unmount the image : %s", command);
			i = system(command);
			LOG_DEBUG("system call for unmounting exit status : %d", i);
			if ( i != 0 ) {
				LOG_ERROR("Error in unmounting the vm image. Please check log file : %s/%s-%d", vm_manifest_dir, ma_log, child);
				start_app_status = 1;
				goto return_response;
			}
#endif
			LOG_DEBUG("Unmount of image Successfull");
			if ( verifier_exit_status != 0 ) {
				start_app_status = 1;
				goto return_response;
			}
			LOG_DEBUG("MA executed successfully");
			// Only call verfier when measurement is required
		}
		else if( instance_type == INSTANCE_TYPE_DOCKER) {
			/*
			 * call MA to measure the VM as :
			 * ./verfier manifestlist.xml MOUNT_LOCATION IMVM
			 */
			keep_measurement_log = true;
			LOG_TRACE("Instace type docker : %d", instance_type);
			snprintf(command, sizeof(command), "./verifier %s %s/ IMVM >> %s/%s-%d 2>&1", nohash_manifest_file, mount_path, vm_manifest_dir, ma_log, child);
			LOG_DEBUG("Command to launch MA : %s", command);
			i = system(command);
			LOG_DEBUG("system call to verifier exit status : %d", i);
			if ( i != 0 ) {
				LOG_ERROR("Measurement agent failed to execute successfully. Please check Measurement log in file %s/%s-%d", vm_manifest_dir, ma_log, child);
				start_app_status = 1;
				goto return_response;
			}
			LOG_DEBUG("MA executed successfully");
		}
		// Open measurement log file at a specified location
		fq = fopen(cumulativehash_file, "rb");
		if(!fq)
		{
			LOG_ERROR("Cumulative hash file not found, please check Measurement log in file %s/%s-%d\n", vm_manifest_dir, ma_log, child);
			//return TCSERVICE_RESULT_FAILED; // measurement failed  (verifier failed to measure)
			start_app_status = 1;
			goto return_response;
		}

		char imageHash[65] = {'\0'};
		//int flag=0;
			if (fq != NULL) {
				char line[512];
				if(fgets(line,sizeof(line),fq)!= NULL)  {
					strncpy_s(imageHash, sizeof(imageHash), line, 64);
				}
			}
			fclose(fq);
			LOG_DEBUG("Calculated hash : %s and Golden Hash : %s",imageHash, goldenImageHash);
			if (strcmp(imageHash, goldenImageHash) == 0) {
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
				strcpy_s(vm_manifest_hash, MANIFEST_HASH_SIZE, imageHash);
				/*int len = strnlen_s(imageHash,sizeof(imageHash));
				int iSize = 0;
				for (c= 0; c < len; c = c+2) {
					sscanf(&imageHash[c], "%02x", (unsigned int *)&rgHash[c/2]);
					iSize++;
				}*/
				LOG_TRACE("Adding proc table entry for measured VM");
				int temp_proc_id = g_myService.m_procTable.getprocIdfromuuid(vm_uuid);
				int vm_data_size = 0;
				char* vm_data[1];
				vm_data[0] = vm_uuid;
				vm_data_size++;
				if ( temp_proc_id == NULL) {
					if(!g_myService.m_procTable.addprocEntry(child, kernel_file, vm_data_size, vm_data, size, rgHash, instance_type)) {
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
    	}
    	else {
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
			if ( keep_measurement_log == false ) {
				//TODO write a remove directory function using dirint.h header file
				LOG_TRACE("will remove reports directory %s", vm_manifest_dir);
				remove_dir(vm_manifest_dir);
			}
			else {
				LOG_TRACE("will not remove reports directory %s", vm_manifest_dir);
			}
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
	char response[64] = { '\0' };
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
        	snprintf(response, sizeof(response), "%d", *((int *)outparams));
        	response_size = strnlen_s(response,sizeof(response));

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
			snprintf(response, sizeof(response), "%d", -1);
		}
		else {
			//response = 0;
			//*((int *) response) = 0;
			snprintf(response, sizeof(response), "%d", 0);
		}
		response_size = strnlen_s(response,sizeof(response));
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
            snprintf(verificationstat, verificationstatsize, "%d",verification_status);
            verificationstatsize = strnlen_s((char *)verificationstat, verificationstatsize);
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

			int vm_manifest_dir_size = strnlen_s(vm_manifest_dir,sizeof(vm_manifest_dir));
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
            xmlFree(method_name);
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

void* clean_deleted_docker_instances(void *){
	std::set<std::string> uuid_list;
	LOG_TRACE("");
	while(g_myService.m_procTable.getactivedockeruuid(uuid_list)) {
		int cleaned_entries;
#ifdef __linux__
		sleep(g_entry_cleanup_interval);
#elif _WIN32
		DWORD g_entry_cleanup_interval_msec = g_entry_cleanup_interval * 1000;
		Sleep(g_entry_cleanup_interval_msec);
#endif
		g_myService.CleanVrtmTable(uuid_list, &cleaned_entries);
		LOG_INFO("Number of Docker instances removed from vRTM table : %d", cleaned_entries);
		uuid_list.clear();
	}
	g_docker_deletion_service_status = 0;
	LOG_DEBUG("Docker Deletion Service thread exiting...");
	return NULL;
}

#ifdef _WIN32
void* clean_and_update_hyperv_vm_status(void *) {
	std::set<std::string> active_vms;
	LOG_TRACE("");
	while (g_myService.m_procTable.getactivevmsuuid(active_vms)) {
		int removed_instances_count;
		int stopped_instances_count;
#ifdef __linux__
		sleep(g_entry_cleanup_interval_);
#elif _WIN32
		DWORD g_entry_cleanup_interval_msec = g_entry_cleanup_interval * 1000;
		Sleep(g_entry_cleanup_interval_msec);
#endif
		g_myService.CleanVrtmTable_and_update_vm_status(active_vms, &removed_instances_count, &stopped_instances_count);
		active_vms.clear();
	}
	g_hyperv_vm_cleanup_service_status = 0;
	return (void *)NULL;
}
#endif

void cleanupService() {
	pthread_t tid, tid_d, tid_hyperv;
	pthread_attr_t attr, attr_d, attr_hyperv;
	LOG_TRACE("");
	if (g_cleanup_service_status == 1) {
		LOG_INFO("Clean-up Service already running");
		//return 0;
	}
	else {
		pthread_attr_init(&attr);
		if (!pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
			pthread_create(&tid, &attr, clean_vrtm_table, (void *)NULL);
			LOG_INFO("Successfully created the thread for entries cleanup");
			g_cleanup_service_status = 1;
			pthread_attr_destroy(&attr);
			//return 0;
		}
		else {
			LOG_ERROR("Can't set cleanup thread attribute to detatchstate");
			LOG_ERROR("Failed to spawn the vRTM entry clean up thread");
			pthread_attr_destroy(&attr);
			//return 1;
		}
	}

	if(g_docker_deletion_service_status == 1) {
		LOG_INFO("Docker deletion Service already running");
		//return 0;
	}
	else {
		pthread_attr_init(&attr_d);
		if (!pthread_attr_setdetachstate(&attr_d, PTHREAD_CREATE_DETACHED)) {
			pthread_create(&tid_d, &attr_d, clean_deleted_docker_instances, (void *)NULL);
			LOG_INFO("Successfully created the thread for cleaning deleted docker instances");
			g_docker_deletion_service_status = 1;
			pthread_attr_destroy(&attr_d);
			//return 0;
		}
		else {
			LOG_ERROR("Can't set docker deletion thread attribute to detachstate");
			LOG_ERROR("Failed to spawn the Docker Deletion service thread");
			pthread_attr_destroy(&attr_d);
			//return 1;
		}
	}

#ifdef _WIN32
	if (g_hyperv_vm_cleanup_service_status) {
		LOG_INFO("Hyper-V VM status checking service is already running");
	}
	else {
		pthread_attr_init(&attr_hyperv);
		if (!pthread_attr_setdetachstate(&attr_hyperv, PTHREAD_CREATE_DETACHED)) {
			if (pthread_create(&tid_hyperv, &attr_hyperv, clean_and_update_hyperv_vm_status, (void *)NULL)) {
				LOG_ERROR("Failed to spawn thread for hyper-v vm cleanup and status updation");
			}
			else {
				LOG_INFO("Successfully spawn thread for hyper-v vm cleanup and status updation");
				g_hyperv_vm_cleanup_service_status = 1;
			}
			pthread_attr_destroy(&attr_d);
		}
		else {
			LOG_ERROR("Can't set Hyper-V VM cleanup and VM status update thread attribute to detachstate");
			LOG_ERROR("Failed to spawn thread for Hyper-V VM cleanup and status updation");
			pthread_attr_destroy(&attr_hyperv);
		}
	}
#endif
}			
