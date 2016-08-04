/*
VRTM Proxy acts as a proxy for QEMU. It intercepts the call from libvirt to QEMU to contact VRTMCore for VM image measurement.
VRTM-Proxy binary is installed with the name qemu, so that when libvirt calls qemu, actually the VRTM-proxy binary is called.
VRTM-proxy will call qemu with VM launch options after the VM image measurement is successfull.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include "tcpchan.h"
#include "channelcoding.h"
#include "parser.h"
#include "vrtm_api_code.h"
#include "logging.h"
#include "log_vrtmchannel.h"
#include "loadconfig.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "safe_lib.h"
#ifdef __cplusplus
}
#endif

#define QEMU_SYSTEM_PATH            "/usr/bin/qemu-system-x86_64_orig"
#define VRTMCORE_DEFAULT_IP_ADDR    "127.0.0.1"
//#define PARAMSIZE                   8192
#define TCSERVICESTARTAPPFROMAPP    15
#define VRTMCORE_DEFAULT_PORT       16005
#define VRTM_LISTENER_IP_ADDR       "127.0.0.1"
#define VRTM_LISTENER_SERVICE_PORT  16004
#define VM_NAME_MAXLEN              2048
#define g_config_file "/opt/vrtm/configuration/vRTM.cfg"
#define log_properties_file "/opt/vrtm/configuration/vrtm_proxylog.properties"
#ifndef byte
typedef unsigned char byte;
#endif

int rp_fd = -1;
//int rp_listener_fd = -1;


int channel_open() {
    int fd = -1;
    LOG_TRACE("");

#ifndef USE_DRV_CHANNEL //not defined
    fd =  ch_open(NULL, 0); 
#else
    fd = open("/dev/chandu", O_RDWR);

#endif
    if(fd < 0) {
            LOG_ERROR("Can't connect with vRTM: %s", strerror(errno));
            return -1;
    }   

    //ch_register(fd);

    return fd;
}


int conn_open() {
    LOG_TRACE("Open connection with vRTM");
	//struct sockaddr_in rp_listener_server;
	rp_fd = channel_open();
	if( rp_fd < 0 ) {
		LOG_ERROR("can't connect to vRTM");
		return 1;
	}
    LOG_TRACE("Connection established with vRTM");
	return 0;	
}


/*
 Send kernel, ramdisk, disk and manifest path to VRTMCore and get the response
 Extract dom id from the response
*/
int get_rpcore_response(char* kernel_path, char* ramdisk_path, char* disk_path, 
                        char* uuid) {
    
    int         err = -1;
    int         retval = -1;
    int         size = PARAMSIZE;
    byte        rgBuf[PARAMSIZE];
    tcBuffer*   pReq= (tcBuffer*) rgBuf;
    int         an = 0;
    char*       av[20] = {0};
    int response_size;
    byte response[1024] = {'\0'};

    LOG_TRACE("Prepare request to send to vRTM");
    av[an++] = "-kernel";
    av[an++] = kernel_path;
    av[an++] = "-ramdisk";
    av[an++] = ramdisk_path;
    av[an++] = "-disk";
    av[an++] = disk_path;
    av[an++] = "-uuid";
    av[an++] = uuid;
    av[an++] = "-config";
    av[an++] = "config";
 
    size = sizeof(tcBuffer);
    size = encodeVM2RP_STARTAPP("foo", an, av, PARAMSIZE -size, &rgBuf[size]);

    //pReq->m_procid = 0;
    pReq->m_reqID = VM2RP_STARTAPP;
    pReq->m_ustatus = 0;
    //pReq->m_origprocid = 0;
    pReq->m_reqSize = size;

    LOG_TRACE("Sending request. size: %d", size + sizeof(tcBuffer));
    
    err = ch_write(rp_fd, rgBuf, size + sizeof(tcBuffer) );

    if (err < 0){
        LOG_ERROR( "Socket write error %s", strerror(errno));
        goto fail;
    }

    memset_s(rgBuf, sizeof(rgBuf), 0);
    LOG_TRACE("Request sent successfully");
    
again:  
    err = ch_read(rp_fd, rgBuf, sizeof(rgBuf));
    if (err < 0) {
        LOG_TRACE("Read error: %d", errno);
        if (errno == 11) {
            LOG_TRACE("Retrying to read from socket");
            sleep(1);
            goto again;
        }

        LOG_ERROR("Read error:%d  %s", errno, strerror(errno));
        goto fail;
    }

    LOG_ERROR( "Response from server status %d return %s",
            pReq->m_ustatus, &rgBuf[sizeof(tcBuffer)]);
    if ( !decodeRP2VM_STARTAPP(&response_size, response, (byte *)&rgBuf[sizeof(tcBuffer)] ) ) {
    	LOG_ERROR("Failed to decode the response");
    	goto fail;
    }
    if(pReq->m_ustatus == 0 ) {
    	if (atoi( (char *)response) > 0)
    		LOG_INFO( "VM entry creation in vRTM was successful");
    	else {
    		LOG_ERROR( "VM entry creation in vRTM failed");
    		goto fail;
    	}
    }
    else {
    	LOG_ERROR( "Error occur on VRTM in processing request");
        goto fail;
    }
    retval = atoi( (char *)response);

fail:
    LOG_TRACE("Closing channel");
    if ( rp_fd >= 0)
        ch_close (rp_fd);
    return retval;
}


int main(int argc, char** argv) {
    int     i, index = -1;
    char    *kernel_path = NULL, *initrd_path = NULL, *vm_name;
    char    *disk_start_ptr, *disk_end_ptr;
    char    *drive_data, disk_path[PATH_MAX], manifest_path[PATH_MAX];
    char    trust_report_dir[1024] ={0};
    char	uuid[65];
    std:: map<std::string, std::string> config_map;

    char    kernel_args[4096] = {0};
    int     rp_domid = -1;
    char    *vrtmcore_ip, *s_vrtmcore_port;
    int     vrtmcore_port;
    bool    has_drive = false, has_smbios = false;
    bool    is_launch_request = false;
    bool    kernel_provided = false;
    int     vm_pid;
    //const char* log_properties_file = "/opt/dcg_security-vrtm-cleanup-2.0/vrtmcore/config/rp_proxylog.properties";
    // instantiate the logger
    if( initLog(log_properties_file, "proxy") ){
       	return 1;
    }
    // set same logger instance in rp_channel
    set_logger_vrtmchannel(rootLogger);

    LOG_TRACE("Starting vRTM proxy");
    vm_pid = getpid();
    LOG_DEBUG("vRTM proxy process id is %d\n", vm_pid);

    /* 
    log the original qemu command. Two possibilities here:
    I. Start a qemu process and talk QMP over stdio to discover what capabilities the binary 
       supports. In this case just start the qemu process without any modification.
    II. Start a qemu process for new VM. In this case first send request to VRTMCore and proceed only
        if VRTMCore allows the launch.
    */
    LOG_DEBUG( "Original qemu command:");
    for (i=0; i<argc; i++) {
        LOG_DEBUG("\n%s%s\n", " ", argv[i]);
    }

    argv[0] = QEMU_SYSTEM_PATH;

    LOG_TRACE("Parsing arguments to identify launch request and required parameters");
    for (i = 0; i < argc; i++) {
        // based on the drive flag, determine if its a VM launch request
        // Possible issue: if there are multiple drives (cd, disk, floppy) then need to check 
        // the type for each drive and pass the drive that is the disk
        if ( argv[i] && (strcmp(argv[i], "-drive") == 0) && (strstr(argv[i+1], "/disk.config") == NULL) ) {
            has_drive = true;
            drive_data = argv[i+1];
            LOG_DEBUG("has drive : %d drive data : %s",has_drive, drive_data);
        }
        if ( argv[i] && strcmp(argv[i], "-smbios") == 0 ) {
            has_smbios = true;
            LOG_DEBUG("has smbios : %d", has_smbios);
        }
        if ( argv[i] && strcmp(argv[i], "-name") == 0 ) {
            vm_name = argv[i+1];
            LOG_DEBUG("vm_NAME : %s", vm_name);
        }
        if ( argv[i] && strcmp(argv[i], "-kernel") == 0 ) {
            kernel_path = argv[i+1];
            kernel_provided = true;
            LOG_DEBUG("kerne path : %s kernel provided status : %d", kernel_path, kernel_provided);
        }
        if ( argv[i] && strcmp(argv[i], "-initrd") == 0 ) {
            initrd_path = argv[i+1];
            LOG_DEBUG("initrd path : %s", initrd_path);
        }
        // Extra kernel arguments, if provided
        if ( argv[i] && strcmp(argv[i], "-append") == 0 ) {
            index = i;
        }
    }

    is_launch_request = has_drive && has_smbios;

    // If its not a VM launch request and execute the command without any processing
    if(!is_launch_request) {
        LOG_TRACE("Not vm launch request");
        execve(argv[0], argv, NULL);
        closeLog();
        return 0;
    }
    // Parse the command line request and extract the disk path
    
    disk_start_ptr = strstr(drive_data, "file=") + sizeof("file=") - 1;
    disk_end_ptr = strchr(drive_data, ',');
    int disk_path_len = disk_end_ptr-disk_start_ptr;
	LOG_DEBUG("Disk path length: %d", disk_path_len);
    memset_s(disk_path, sizeof(disk_path), '\0');
    strncpy_s(disk_path, PATH_MAX, disk_start_ptr, disk_path_len);
	LOG_DEBUG("Disk Path: %s", disk_path);
    //Extract UUID of VM
    strncpy_s(trust_report_dir, sizeof(trust_report_dir), disk_path, disk_path_len - (sizeof("/disk") - 1));
    char *uuid_ptr = strrchr(trust_report_dir, '/');
    strcpy_s(uuid, sizeof(uuid), uuid_ptr + 1);
    LOG_TRACE("Extracted UUID : %s", uuid);

    // Parse the config file and extract the manifest path
    if(load_config(g_config_file, config_map) < 0) {
    	LOG_ERROR("Can't load config file %s", g_config_file);
		return EXIT_FAILURE;
	}

    std::string reqValue = config_map["trust_report_dir"];
    clear_config(config_map);
    strcpy_s(trust_report_dir, sizeof(trust_report_dir), reqValue.c_str());
    memset(manifest_path, '\0', sizeof(manifest_path));
	snprintf(manifest_path, sizeof(manifest_path), "%s%s%s", trust_report_dir, uuid, "/trustpolicy.xml");
	LOG_DEBUG("Path of trust policy: %s", manifest_path);

// If not measured launch then execute command without calling vRTM
    if(access(manifest_path, F_OK)!=0){
        LOG_INFO("trustpolicy.xml doesn't exist at  %s", manifest_path);
        LOG_INFO( "Forwarding request for normal non-measured VM launch");
        execve(argv[0], argv, NULL);
        closeLog();
        return 0;
    }
	if ( conn_open() ) { 
        LOG_ERROR("Not able to launch measured VM");
        closeLog();
        return 0;
    }
    // If kernel and initrd are not passed then pass them as empty strings
    kernel_path = (kernel_path == NULL) ? "" : kernel_path;
    initrd_path = (initrd_path == NULL) ? "" : initrd_path;

    //LOG_DEBUG( "VM name: %s\n", vm_name);
    LOG_DEBUG("kernel_path=%s, ramdisk_path=%s, disk_path=%s, vm_uuid=%s\n",
                kernel_path, initrd_path, disk_path, uuid);
    vrtmcore_ip = getenv("VRTMCORE_IPADDR");
    if (!vrtmcore_ip)
        vrtmcore_ip = VRTMCORE_DEFAULT_IP_ADDR;

    s_vrtmcore_port = getenv("VRTMCORE_PORT");
    if (!s_vrtmcore_port)
        vrtmcore_port = VRTMCORE_DEFAULT_PORT;
    else
        vrtmcore_port = atoi(s_vrtmcore_port);

    /* 
    get the decision from VRTMCore. VRTMCore will verify the integrity of the disk using
    IMVM and return the result (pass/fail) and a unique id (integer) for the VM
    */

    LOG_TRACE("Calling vRTM to measure VM image");
    rp_domid = get_rpcore_response(kernel_path, initrd_path, disk_path, uuid);


    LOG_DEBUG( "Response from vrtmcore is : %d\n", rp_domid);
    if (rp_domid <= 0) {
        LOG_ERROR("Launch denied by VRTMCore\n");
        closeLog();
        return EXIT_FAILURE;
    }

    // add VRTMCore ip and port in kernel arguments for the VM. The VM can use it to contact VRTMCore
    if(kernel_provided && index != -1) {
        index++;
		strcat_s(kernel_args, sizeof(kernel_args), argv[index]);
        //snprintf(kernel_args, sizeof(kernel_args), "%s", argv[index]);
		strcat_s(kernel_args, sizeof(kernel_args), " vrtmcore_ip=");
		strcat_s(kernel_args, sizeof(kernel_args), vrtmcore_ip);
        //snprintf(kernel_args, sizeof(kernel_args), "%s rpcore_ip=%s", kernel_args, rpcore_ip);
		strcat_s(kernel_args, sizeof(kernel_args), " vrtmcore_port=");
		char vrtm_port_buff[32] = {'\0'};
		snprintf(vrtm_port_buff, sizeof(vrtm_port_buff), "%d", vrtmcore_port);
		strcat_s(kernel_args, sizeof(kernel_args), vrtm_port_buff);
        //snprintf(kernel_args, sizeof(kernel_args), "%s rp_port=%d", kernel_args, rpcore_port);
		kernel_args[sizeof(kernel_args) - 1] = '\0';
        argv[index] = kernel_args;
        
        LOG_DEBUG( "Modified kernel args: %s", kernel_args);
    }

    // dump the updated qemu command
    LOG_DEBUG("Modified qemu command:");
    for (i=0; i<argc; i++) {
        LOG_DEBUG( "%s%s", " ", argv[i]);
    }

    /* If VRTMCore allows the launch then the id returned by VRTMCore needs to be replaced with
       the actual UUID of the VM after successful launch. The control will be lost when we call execve command, so notify rp_listener now.
       If VM launch is successfull then rp_listener will request VRTMCore to update the entry for this VM.
    */
    /*if(notify_rp_listener(rp_domid, vm_name) <= 0) {
        closeLog();
        return EXIT_FAILURE;
    }*/
    LOG_DEBUG("Starting QEMU process");
    closeLog();

    //launch the VM process, VRTM-Proxy stack will be overwritten by qemu process.
    execve(argv[0], argv, NULL);

    return EXIT_SUCCESS;
}


