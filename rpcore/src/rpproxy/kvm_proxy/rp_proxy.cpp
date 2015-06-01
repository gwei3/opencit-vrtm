/*
RP Proxy acts as a proxy for QEMU. It intercepts the call from libvirt to QEMU to contact RPCore for VM image measurement. 
RP-Proxy binary is installed with the name qemu, so that when libvirt calls qemu, actually the RP-proxy binary is called.
RP-proxy will call qemu with VM launch options after the VM image measurement is successfull.
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
#include "pyifc.h"
#include "rp_api_code.h"
#include "logging.h"
#include "log_rpchannel.h"

#define QEMU_SYSTEM_PATH            "/usr/bin/qemu-system-x86_64_orig"
#define RP_PROXY_LOGFILE            "/var/log/rp_proxy.log"
#define RPCORE_DEFAULT_IP_ADDR      "127.0.0.1"
#define PARAMSIZE                   8192
#define TCSERVICESTARTAPPFROMAPP    15
#define RPCORE_DEFAULT_PORT         16005
#define RP_LISTENER_IP_ADDR         "127.0.0.1"
#define RP_LISTENER_SERVICE_PORT    16004
#define VM_NAME_MAXLEN              2048
#define log_properties_file "/opt/dcg_security-vrtm-cleanup-1.5/rpcore/configuration/rp_proxylog.properties"
#ifndef byte
typedef unsigned char byte;
#endif

int rp_fd = -1;
FILE* f = fopen(RP_PROXY_LOGFILE, "a");

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


/*
 Send kernel, ramdisk, disk and manifest path to RPCore and get the response
 Extract dom id from the response
*/
int get_rpcore_response(char* kernel_path, char* ramdisk_path, char* disk_path, 
                        char* manifest_path) {
    
    int         err = -1;
    int         retval;
    int         size = PARAMSIZE;
    byte        rgBuf[PARAMSIZE];
    tcBuffer*   pReq= (tcBuffer*) rgBuf;
    int         an = 0;
    char*       av[20] = {0};

    /*if (init_pyifc("rppy_ifc") < 0 )
        return -1;*/
    LOG_TRACE("");
    av[an++] = "./vmtest";
    av[an++] = "-kernel";
    av[an++] = kernel_path;
    av[an++] = "-ramdisk";
    av[an++] = ramdisk_path;
    av[an++] = "-disk";
    av[an++] = disk_path;
    av[an++] = "-manifest";
    av[an++] = manifest_path;
    av[an++] = "-config";
    av[an++] = "config";
 
#ifdef DEBUG
    LOG_TRACE( "get_rpcore_response(): Opening device driver .. ");
#endif

    rp_fd = channel_open();

    if(rp_fd < 0) {
        LOG_ERROR( "get_rpcore_response(): error opening socket: %s", strerror(errno));
        return -1;
    }

    size = sizeof(tcBuffer);
    size = encodeVM2RP_STARTAPP("foo", an, av, PARAMSIZE -size, &rgBuf[size]);

    //pReq->m_procid = 0;
    pReq->m_reqID = VM2RP_STARTAPP;
    pReq->m_ustatus = 0;
    //pReq->m_origprocid = 0;
    pReq->m_reqSize = size;

#ifdef DEBUG
    LOG_TRACE("get_rpcore_response(): sending request size %d", size + sizeof(tcBuffer));
#endif
    
    err = ch_write(rp_fd, rgBuf, size + sizeof(tcBuffer) );

    if (err < 0){
        LOG_ERROR( "get_rpcore_response(): write error %s", strerror(errno));
        goto fail;
    }

    memset(rgBuf, 0, sizeof(rgBuf));
    LOG_TRACE("get_rpcore_response(): sent request ..........");
    
again:  
    err = ch_read(rp_fd, rgBuf, sizeof(rgBuf));
    if (err < 0) {
        if (errno == 11) {
            sleep(1);
            goto again;
        }

        LOG_ERROR("get_rpcore_response(): read error:%d  %s", errno, strerror(errno));
        goto fail;
    }

    fprintf(f, "get_rpcore_response(): response  from server status %d return %d",
            pReq->m_ustatus, *(int*) &rgBuf[sizeof(tcBuffer)]);

    if(pReq->m_ustatus == 0)
        LOG_INFO( "get_rpcore_response(): VM entry creation in rpcore was successful");
    else {
        LOG_ERROR( "get_rpcore_response(): VM entry creation in rpcore failed");
        return -1;
    }
    retval = *(int*) &rgBuf[sizeof(tcBuffer)];

    return retval;

fail:
    if ( rp_fd >= 0)
        ch_close (rp_fd);

    //deinit_pyifc();
    return -1;
}


int notify_rp_listener(int rp_domid, char* vm_name) {

    int     sock;
    struct  sockaddr_in server;
    char    msg[4096];
    char    resp[4];
    int     name_len;
     LOG_TRACE("");
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1) {
        LOG_ERROR("notify_rp_listener(): Could not create socket");
    }
     
    server.sin_addr.s_addr = inet_addr(RP_LISTENER_IP_ADDR);
    server.sin_family = AF_INET;
    server.sin_port = htons(RP_LISTENER_SERVICE_PORT);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        close(sock);
        LOG_ERROR( "notify_rp_listener():connection to rp listener failed");
        return 1;
    }

    /*
    Send the domid (received from RPCore) and vm_name (unique; generated by nova) to rp_listener.
    rp_listener will send uuid mapping request to RPCore.
    */
    name_len = strlen(vm_name);
    memcpy(msg, &rp_domid, sizeof(int));
    memcpy(&msg[sizeof(int)], &name_len, sizeof(int));
    memcpy(&msg[2*sizeof(int)], vm_name, strlen(vm_name));
    
    if(send(sock, msg, 2*sizeof(int) + name_len, 0) < 0) {
        LOG_ERROR( "notify_rp_listener():sending data to rp listener failed");
        close(sock);
        return 1;
    }
     
    if(recv(sock, resp, 4, 0) < 0) {
        close(sock);
        LOG_ERROR( "notify_rp_listener():failed to get response from rp listener");
        return 1;
    }
     
    LOG_TRACE( "notify_rp_listener():response from rp listener is %d", *(int*)resp);

    close(sock);

    return *(int*)resp;

}


int main(int argc, char** argv) {
    int     i, index;
    char    *kernel_path = NULL, *initrd_path = NULL, *vm_name;
    char    *disk_start_ptr, *disk_end_ptr;
    char    *drive_data, disk_path[PATH_MAX], manifest_path[PATH_MAX];

    char    kernel_args[4096];
    int     rp_domid = -1;
    char    *rpcore_ip, *s_rpcore_port;
    int     rpcore_port;
    bool    has_drive = false, has_smbios = false;
    bool    is_launch_request = false;
    bool    kernel_provided = false;
    int     vm_pid;
    //const char* log_properties_file = "/opt/dcg_security-vrtm-cleanup-2.0/rpcore/config/rp_proxylog.properties";
    // instantiate the logger
    if( initLog(log_properties_file) ){
       	return 1;
    }
    // set same logger instance in rp_channel
    set_logger_rpchannel(rootLogger);

    LOG_TRACE("");
    vm_pid = getpid();
    LOG_DEBUG("rp_proxy process id is %d\n", vm_pid);

#ifdef DEBUG
    /* 
    log the original qemu command. Two possibilities here:
    I. Start a qemu process and talk QMP over stdio to discover what capabilities the binary 
       supports. In this case just start the qemu process without any modification.
    II. Start a qemu process for new VM. In this case first send request to RPCore and proceed only
        if RPCore allows the launch.
    */
    LOG_DEBUG( "original qemu command:");
    for (i=0; i<argc; i++) {
        LOG_DEBUG("\n%s%s\n", " ", argv[i]);
    }
#endif

    argv[0] = QEMU_SYSTEM_PATH;

    for (i = 0; i < argc; i++) {
        // based on the drive flag, determine if its a VM launch request
        // Possible issue: if there are multiple drives (cd, disk, floppy) then need to check 
        // the type for each drive and pass the drive that is the disk
        if ( argv[i] && (strcmp(argv[i], "-drive") == 0) && (strstr(argv[i+1], "/disk.config") == NULL) ) {
            has_drive = true;

            drive_data = argv[i+1];
            LOG_DEBUG("has drive : %d drive data : ",has_drive, drive_data);
        }
        if ( argv[i] && strcmp(argv[i], "-smbios") == 0 ) {
            has_smbios = true;
            LOG_DEBUG("has smbios : %d", has_smbios);
        }
        if ( argv[i] && strcmp(argv[i], "-name") == 0 ) {
            vm_name = argv[i+1];
            LOG_DEBUG("vm_NAME : ", vm_name);
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
        fclose(f);
        LOG_INFO("Not vm launch request");
        execve(argv[0], argv, NULL);
        return 0;
    }

    // Parse the command line request and extract the disk path and manifest path
    disk_start_ptr = strstr(drive_data, "file=") + strlen("file=");
    disk_end_ptr = strstr(drive_data, ",if=none");
    memset(disk_path, '\0', sizeof(disk_path));
    strncpy(disk_path, disk_start_ptr, disk_end_ptr-disk_start_ptr);
    memset(manifest_path, '\0', sizeof(manifest_path));
    strncpy(manifest_path, disk_path, strlen(disk_path)-strlen("/disk"));
    sprintf(manifest_path, "%s%s", manifest_path, "/trustpolicy.xml");

// If not measured launch then execute command without calling vRTM
    if(access(manifest_path, F_OK)!=0){
	LOG_INFO("Trustpolicy.xml doesn't exist at  %s", manifest_path);
	LOG_INFO( "Forwarding request for normal non-measured VM launch");
        fclose(f);
        execve(argv[0], argv, NULL);
        return 0;
    }

    // If kernel and initrd are not passed then pass them as empty strings
    kernel_path = (kernel_path == NULL) ? "" : kernel_path;
    initrd_path = (initrd_path == NULL) ? "" : initrd_path;

    LOG_DEBUG( "vm name: %s\n", vm_name);
    LOG_DEBUG("kernel_path=%s, ramdisk_path=%s, disk_path=%s, manifest_path=%s\n",
                kernel_path, initrd_path, disk_path, manifest_path);
    rpcore_ip = getenv("RPCORE_IPADDR");
    if (!rpcore_ip)
        rpcore_ip = RPCORE_DEFAULT_IP_ADDR;

    s_rpcore_port = getenv("RPCORE_PORT");
    if (!s_rpcore_port)
        rpcore_port = RPCORE_DEFAULT_PORT;
    else
        rpcore_port = atoi(s_rpcore_port);

    /* 
    get the decision from RPCore. RPCore will verify the integrity of the disk using 
    IMVM and return the result (pass/fail) and a unique id (integer) for the VM
    */
    rp_domid = get_rpcore_response(kernel_path, initrd_path, disk_path, manifest_path);

    if (rp_fd >= 0)
        close(rp_fd);
    
    //deinit_pyifc();

    if (rp_domid <= 0) {
        LOG_ERROR("Launch denied by RPCore\n");
        fclose(f);
        return EXIT_FAILURE;
    }

    LOG_DEBUG( "response from rpcore is : %d\n", rp_domid);
    // add RPCore ip and port in kernel arguments for the VM. The VM can use it to contact RPCore
    if(kernel_provided) {
        index++;
        sprintf(kernel_args, "%s", argv[index]);
        sprintf(kernel_args, "%s rpcore_ip=%s", kernel_args, rpcore_ip);
        sprintf(kernel_args, "%s rp_port=%d", kernel_args, rpcore_port);
        argv[index] = kernel_args;
        
        LOG_DEBUG( "modified kernel args: %s\n", kernel_args);
}

    // dump the updated qemu command
    LOG_DEBUG("modified qemu command:\n");
    for (i=0; i<argc; i++) {
        LOG_DEBUG( "%s%s", " ", argv[i]);
    }

    /* If RPCore allows the launch then the id returned by RPCore needs to be replaced with
       the actual UUID of the VM after successful launch. The control will be lost when we call execve command, so notify rp_listener now.
       If VM launch is successfull then rp_listener will request RPCore to update the entry for this VM.
    */
    if(notify_rp_listener(rp_domid, vm_name) <= 0) {
        fclose(f);
        return EXIT_FAILURE;
    }
    fclose(f);

    //launch the VM process, RP-Proxy stack will be overwritten by qemu process.
    execve(argv[0], argv, NULL);

    return EXIT_SUCCESS;
}


