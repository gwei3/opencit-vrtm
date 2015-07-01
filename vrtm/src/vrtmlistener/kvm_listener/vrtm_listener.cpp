/*
rp_listener runs as a demon on the host OS, to support graceful VM termination and notifying VM UUID to
RPCore after VM starts up. 
Its main function is to receive the VM’s ID from RP-proxy after VM is launched and in case of successful 
VM launch it sends request to RPCore to replace the VM’s ID in RPCore with the actual UUID of the launched VM. 
It subscribes to libvirt events; if VM termination event is received from libvirt then it notifies RPCore with 
VM’s UUID to clean up the VM’s record in RPCore.
*/

#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/limits.h>
#include <signal.h>
#include <inttypes.h>
#include <pthread.h>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <map>
#include <iostream>
#include "tcpchan.h"
#include "channelcoding.h"
#include "parser.h"
#include "vrtm_api_code.h"

#include "logging.h"
#include "log_vrtmchannel.h"

#define UUID_LENGTH         36
#define TCP_REQUEST_SIZE    512
#define TCP_LISTEN_PORT     16004
#define PARAMSIZE           8192
#define RPCORESERVICE_PORT  6005
#define MAX_MSG_SIZE        4096

#define VM_STATUS_CANCELLED						0
#define VM_STATUS_STARTED						1
#define VM_STATUS_STOPPED						2
#define VM_STATUS_DELETED						3

#define log_properties_file "../configuration/vrtm_log.properties"

#define VIR_DEBUG(fmt) printf("%s:%d: " fmt "\n", __func__, __LINE__)
#define STREQ(a,b) (strcmp(a,b) == 0)

#ifndef ATTRIBUTE_UNUSED
# define ATTRIBUTE_UNUSED __attribute__((__unused__))
#endif

#ifndef byte
typedef unsigned char byte;
#endif

virConnectPtr dconn = NULL;
int g_fd = 0;
int run = 1;
int update_vm_status(char*, int);
//int map_rpid_uuid(int, char*);
//std::map<std::string, int> rp_id_map;
static int exit_status = 1;
static int sleep_duration_sec = 5;

int channel_open() {
    int fd = -1;

#ifndef USE_DRV_CHANNEL //not defined
    fd =  ch_open(NULL, 0);
#else
    fd = open("/dev/chandu", O_RDWR);

#endif
    if(fd < 0) {
        LOG_ERROR( "Can't connect with vRTM,  %s", strerror(errno));
        return -1;
    }
    ch_register(fd);
    return fd;
}

static int domainEventCallback(virConnectPtr conn ATTRIBUTE_UNUSED, virDomainPtr dom, int event,
                                  int detail, void *opaque ATTRIBUTE_UNUSED) {

    int eventType = (virDomainEventType) event;
    bool notifyRPCore = false;
    int vm_status;
    LOG_TRACE("Received event call back from Libvirt");
    /*
    check the event type for the event received from libvirt, if it indicated shutdown/failure/crash of the VM 
    then send request to RPCore to delete the entry for this VM from RPCore
    */
    if (eventType == VIR_DOMAIN_EVENT_UNDEFINED) {
    	LOG_DEBUG("VIR_DOMAIN_EVENT_UNDEFINED");
    	if ((virDomainEventStoppedDetailType) detail == VIR_DOMAIN_EVENT_UNDEFINED_REMOVED) {
    		LOG_DEBUG("VIR_DOMAIN_EVENT_UNDEFINED_REMOVED");
    		notifyRPCore = true;
    		vm_status = VM_STATUS_DELETED;
    	}
    	else {
    		 LOG_DEBUG("event detail is not VIR_DOMAIN_EVENT_UNDEFINED_REMOVED");
    	}
    }
    else if (eventType == VIR_DOMAIN_EVENT_STOPPED) {
        switch ((virDomainEventStoppedDetailType) detail) {
            case VIR_DOMAIN_EVENT_STOPPED_DESTROYED:
                notifyRPCore = true;
                vm_status = VM_STATUS_STOPPED;
                break;
            case VIR_DOMAIN_EVENT_STOPPED_SHUTDOWN:
                notifyRPCore = true;
                vm_status = VM_STATUS_STOPPED;
                break;
            case VIR_DOMAIN_EVENT_STOPPED_FAILED:
                notifyRPCore = true;
                vm_status = VM_STATUS_STOPPED;
                break;
            case VIR_DOMAIN_EVENT_STOPPED_CRASHED:
                notifyRPCore = true;
                vm_status = VM_STATUS_STOPPED;
                break;
        }
    } else if (eventType == VIR_DOMAIN_CRASHED) {
        notifyRPCore = true;
        vm_status = VM_STATUS_STOPPED;
    } else if (eventType == VIR_DOMAIN_EVENT_STARTED) {
		LOG_INFO( "Received VIR_DOMAIN_EVENT_STARTED event");
		notifyRPCore = true;
		vm_status = VM_STATUS_STARTED;
	}

    if (notifyRPCore) {
        char vm_uuid[UUID_LENGTH+1];
        LOG_TRACE( "Notifying vRTM of status update of domain %s", virDomainGetName(dom));
        virDomainGetUUIDString(dom, vm_uuid);
        //send the request to RPCore
        update_vm_status(vm_uuid, vm_status);
    }
    return 0;
}

static void connectClose(virConnectPtr conn ATTRIBUTE_UNUSED, int reason, 
                            void *opaque ATTRIBUTE_UNUSED) {
	LOG_TRACE("Connect Close called with reason: %d", reason);
    run = 0;
}

static void freeFunc(void *opaque) {
    char *str = opaque;
    LOG_TRACE("Free func get called with value %s", str);
    free(str);
}


static void stopOnSignal(int sig) {

	LOG_INFO( "Exiting on signal %d", sig);
    run = 0;
    exit_status=0;
}

// register with libvirt and listen to libvirt events
void* listen_libvirt_events( void* input) {

    struct sigaction action_stop;
    memset(&action_stop, 0, sizeof(action_stop));
    action_stop.sa_handler = stopOnSignal;
    LOG_TRACE("Registering for listening to Libvirt events");

    LOG_TRACE("Call virInitialize");
    while(exit_status) {
        if (virInitialize() < 0) {
            LOG_ERROR("Failed to initialize libvirt");
            goto retry;
        }

        LOG_TRACE("Call virEventRegisterDefaultImpl");
        if (virEventRegisterDefaultImpl() < 0) {
            virErrorPtr err = virGetLastError();
            LOG_ERROR("Failed to register event implementation: %s",
                err && err->message ? err->message: "Unknown error");
            goto retry;
        }
        LOG_TRACE("Call virConnectOpenAuth");

        dconn = virConnectOpenAuth(NULL, virConnectAuthPtrDefault, VIR_CONNECT_RO);
        if (!dconn) {
    	    LOG_ERROR( "Error opening vir using virConnectOpenAuth");
            goto retry;
        }

        LOG_TRACE("Register close callback");
        virConnectRegisterCloseCallback(dconn, connectClose, NULL, NULL);
        sigaction(SIGTERM, &action_stop, NULL);
        sigaction(SIGINT, &action_stop, NULL);

        LOG_TRACE("Register domain event callback");
        if ( !virConnectDomainEventRegister(dconn, domainEventCallback, strdup("callback"), freeFunc)) {
            if (virConnectSetKeepAlive(dconn, 5, 3) < 0) {
                virErrorPtr err = virGetLastError();
                LOG_ERROR( "Failed to start keepalive protocol: %s",
                    err && err->message ? err->message : "Unknown error");
                run = 0;
            }
            LOG_TRACE("Polling to libvirt events");
            while (run) {
                if (virEventRunDefaultImpl() < 0) {
                    virErrorPtr err = virGetLastError();
                    LOG_ERROR("Failed to run event loop: %s",
                            err && err->message ? err->message : "Unknown error");
                }
            }
            LOG_TRACE("Deregistering domain event listener");
            virConnectDomainEventDeregister(dconn, domainEventCallback);
        }

        virConnectUnregisterCloseCallback(dconn, connectClose);
        VIR_DEBUG("listen_libvirt_events(): Closing connection");
        LOG_DEBUG("Closing connection");
        if (dconn && virConnectClose(dconn) < 0) {
            LOG_ERROR( "Error closing vir connection");
        }
        retry: 
            LOG_INFO ("Will try to connect to libvirt again in %d seconds", sleep_duration_sec);
            sleep(sleep_duration_sec);
            run=1;
            LOG_INFO("Retrying now...");
    }
    return 0;
}

// Send request to vRTM to remove the entry for a particular VM
int update_vm_status(char* uuid, int vm_status) {
    
    int fd1 = 0;
    int err = -1;
    int retval;
    
    int         size= PARAMSIZE;
    byte        rgBuf[PARAMSIZE];
    tcBuffer*   pReq= (tcBuffer*) rgBuf;
    int response_size;
    byte response[1024] = {'\0'};
    LOG_TRACE( "Opening channel with vRTM to Update VM status for UUID = %s ", uuid);
    fd1 = channel_open();

    if(fd1 < 0)
    {
            LOG_ERROR( "Error while opening channel: %s", strerror(errno));
            return -1;
    }

    //create and send the request to RPCore over the channel
    LOG_TRACE("Prepare request data");
    size = sizeof(tcBuffer);
    //size= encodeVM2RP_TERMINATEAPP (strlen(uuid), uuid, PARAMSIZE -size, (byte*)&rgBuf[size]);
    size= encodeVM2RP_SETVM_STATUS (uuid, vm_status , PARAMSIZE -size, (byte*)&rgBuf[size]);
//    pReq->m_procid= 0;
    pReq->m_reqID=  VM2RP_SETVM_STATUS;
    pReq->m_ustatus= 0;
    //pReq->m_origprocid= 0;
    pReq->m_reqSize= size;

    LOG_DEBUG( "Requesting rpcore to update vm status for vm with uuid %s", uuid);
    LOG_TRACE( "Sending request size: %d \n Payload: %s",
                size + sizeof(tcBuffer), &rgBuf[20]);
    
    err = ch_write(fd1, rgBuf, size + sizeof(tcBuffer) );
    if (err < 0){
        LOG_ERROR( "Socket write error: %s", strerror(errno));
        retval = -1;
        goto fail;
    }

    memset(rgBuf, 0, sizeof(rgBuf));
    LOG_TRACE( "Request sent successfully");
    LOG_TRACE( "Reading from socket for response");
    
again:  
    err = ch_read(fd1, rgBuf, sizeof(rgBuf));
    if (err < 0){
        LOG_TRACE("Read error: %d", errno);
        if (errno == 11) {
            LOG_TRACE("Retrying to read from socket");
            sleep(1);
            goto again;
        }

        LOG_ERROR("Read error: %d  %s\n", errno, strerror(errno));
        retval = -1;
        goto fail;
    }

    LOG_DEBUG( "Response from vRTM. status: %d return: %s\n", pReq->m_ustatus, &rgBuf[sizeof(tcBuffer)]);

    decodeRP2VM_SETVM_STATUS(&response_size, response, (byte *)&rgBuf[sizeof(tcBuffer)]);
    LOG_DEBUG("Response : %d response size : %d", atoi((char *)response), response_size);
    if(pReq->m_ustatus == 0)
    	if ( atoi( (char *)response) == 0 )
    		LOG_INFO( "VM status is successfully  update for VM uuid %s\n", uuid);
    	else {
    		LOG_ERROR("Failed to update the status of VM with UUID : %s", uuid);
    		goto fail;
    	}
    else {
        LOG_ERROR( "Error occurred in vRTM in processing updating VM status for VM uuid %s\n", uuid);
        retval = -1;
        goto fail;
    }
    retval = atoi( (char *)response);
    //return retval;

fail:
    LOG_TRACE("Closing channel");
    if (fd1 >= 0)
        ch_close (fd1);
    fd1 = 0;
    return retval;
}

int main() {

    struct sigaction sigAct;
    memset(&sigAct, 0, sizeof(sigAct));
    sigAct.sa_handler = SIG_DFL;
    sigAct.sa_flags = SA_NOCLDWAIT;
//    const char * log_properties_file = "../../config/rp_listenerlog.properties";
    if( initLog(log_properties_file, "listener") ){
		return 1;
	}

    //set same logger instance in rp_channel
    set_logger_vrtmchannel(rootLogger);

    LOG_TRACE("Logger initialized");
    int sigRv = sigaction(SIGCHLD, &sigAct, NULL);

    if (sigRv < 0) {
        LOG_INFO( "Failed to set signal disposition for SIGCHLD");
    } else {
        LOG_INFO( "Set SIGCHLD to avoid zombies");
    }

    pthread_t th2;

    // create a thread to listen libvirt events
    LOG_DEBUG("Creating a thread to listen to Libvirt events");
    if (pthread_create(&th2, NULL, &listen_libvirt_events, NULL) < 0) {
        LOG_ERROR( "Failed to create thread to listen to Libvirt events");
	closeLog();
        return 1;
    } else
        LOG_INFO("Created a thread for listening libvirt events");

    while(exit_status) {
        sleep(1);
    }
    // Give Libvirt event listener to exit gracefully
    sleep(sleep_duration_sec);
    return 0;
}

