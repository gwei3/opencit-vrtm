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
#include "pyifc.h"
#include "vrtm_api_code.h"

#include "logging.h"
#include "log_vrtmchannel.h"

#define UUID_LENGTH         36
#define TCP_REQUEST_SIZE    512
#define TCP_LISTEN_PORT     16004
#define PARAMSIZE           8192
#define RPCORESERVICE_PORT  6005
#define MAX_MSG_SIZE        4096

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
int delete_rp_uuid(char*);
int map_rpid_uuid(int, char*);
std::map<std::string, int> rp_id_map;
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
    LOG_TRACE("Received event call back from Libvirt");
    /*
    check the event type for the event received from libvirt, if it indicated shutdown/failure/crash of the VM 
    then send request to RPCore to delete the entry for this VM from RPCore
    */
    if (eventType == VIR_DOMAIN_EVENT_STOPPED) {
        switch ((virDomainEventStoppedDetailType) detail) {
            case VIR_DOMAIN_EVENT_STOPPED_DESTROYED:
                notifyRPCore = true;
                break;
            case VIR_DOMAIN_EVENT_STOPPED_SHUTDOWN:
                notifyRPCore = true;
                break;
            case VIR_DOMAIN_EVENT_STOPPED_FAILED:
                notifyRPCore = true;
                break;
            case VIR_DOMAIN_EVENT_STOPPED_CRASHED:
                notifyRPCore = true;
                break;
        }
    } else if (eventType == VIR_DOMAIN_CRASHED) {
        notifyRPCore = true;
    } else if (eventType == VIR_DOMAIN_EVENT_STARTED) {
		LOG_INFO( "Received VIR_DOMAIN_EVENT_STARTED event");
		char vm_uuid[UUID_LENGTH+1];
		std::string vm_name_str(virDomainGetName(dom));
		LOG_TRACE( "VM Name: %s", vm_name_str.c_str());
		int rp_domid=rp_id_map[vm_name_str];
		LOG_TRACE("RP Dom ID: %d", rp_domid);
		if(rp_domid!=0) {
			rp_id_map.erase(vm_name_str);
			virDomainGetUUIDString(dom, vm_uuid);
			LOG_DEBUG( "Updating rp_dom_id:UUID mapping: replacing %d with %s", rp_domid, vm_uuid);
			map_rpid_uuid(rp_domid, vm_uuid);
		}
	}

    if (notifyRPCore) {
        char vm_uuid[UUID_LENGTH+1];
        LOG_TRACE( "Notifying rpcore for shutdown of domain %s", virDomainGetName(dom));
        virDomainGetUUIDString(dom, vm_uuid);
        //send the request to RPCore
        delete_rp_uuid(vm_uuid);
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

// send request to vRTM to map rp_domid with actual VM UUID in vRTM data structures
int map_rpid_uuid(int rpid, char* uuid) {
   
    int fd1=0; 
    char buf[32] = {0};
    int err = -1;
    int retval;
    
    int         size= PARAMSIZE;
    byte        rgBuf[PARAMSIZE];
    tcBuffer*   pReq= (tcBuffer*) rgBuf;
    char*       vdi_uuid = ""; 

    LOG_TRACE("Start mapping rp_domid %d with vm UUID %s", rpid, uuid);
    sprintf(buf, "%d", rpid);
    LOG_TRACE( "Opening channel with vRTM");

    fd1 = channel_open();

    if(fd1 < 0) {
        LOG_ERROR( "Error while openning channel: %s", strerror(errno));
        return -1;
    }
    
    LOG_TRACE("Prepare request data");
    //create and send the request to RPCore over the channel
    size = sizeof(tcBuffer);
    size= encodeVM2RP_SETUUID (buf, uuid, vdi_uuid, PARAMSIZE -size, (byte*)&rgBuf[size]);

    ///pReq->m_procid= 0;
    pReq->m_reqID=  VM2RP_SETUUID ;
    pReq->m_ustatus= 0;
    //pReq->m_origprocid= 0;
    pReq->m_reqSize= size;

    LOG_DEBUG("Sending request size: %d \n Payload: %s",
                size + sizeof(tcBuffer), &rgBuf[20]);
    
    err = ch_write(fd1, rgBuf, size + sizeof(tcBuffer) );
    if (err < 0){
        LOG_ERROR("Write error: %s", strerror(errno));
        retval = 0;
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

        LOG_ERROR( "Read error:%d  %s", errno, strerror(errno));
        retval = 0;
        goto fail;
    }

    LOG_DEBUG( "Response from vRTM. status: %d return: %d",
                pReq->m_ustatus, *(int*) &rgBuf[sizeof(tcBuffer)]);

    if(pReq->m_ustatus == 0)
        LOG_INFO( "Mapping uuid was successfull for VM uuid %s", uuid);
    else {
        LOG_ERROR( "Mapping uuid failed for VM uuid %s", uuid);
        return -1;
    }

    retval = *(int*) &rgBuf[sizeof(tcBuffer)];
    //return retval;

fail:
    LOG_TRACE("Closing channel");
    if ( fd1 >= 0)
        ch_close (fd1);
    fd1 = 0;
    return retval;
}

// Send request to vRTM to remove the entry for a particular VM
int delete_rp_uuid(char* uuid) {
    
    int fd1 = 0;
    int err = -1;
    int retval;
    
    int         size= PARAMSIZE;
    byte        rgBuf[PARAMSIZE];
    tcBuffer*   pReq= (tcBuffer*) rgBuf;
    
    LOG_TRACE( "Opening channel with vRTM to delete UUID mapping for UUID = %s ", uuid);
    fd1 = channel_open();

    if(fd1 < 0)
    {
            LOG_ERROR( "Error while opening channel: %s", strerror(errno));
            return -1;
    }

    //create and send the request to RPCore over the channel
    LOG_TRACE("Prepare request data");
    size = sizeof(tcBuffer);
    size= encodeVM2RP_TERMINATEAPP (strlen(uuid), uuid, PARAMSIZE -size, (byte*)&rgBuf[size]);

//    pReq->m_procid= 0;
    pReq->m_reqID=  VM2RP_TERMINATEAPP;
    pReq->m_ustatus= 0;
    //pReq->m_origprocid= 0;
    pReq->m_reqSize= size;

    LOG_DEBUG( "Requesting rpcore to remove entry for vm with uuid %s", uuid);
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

    LOG_DEBUG( "Response from vRTM. status: %d return: %d\n", pReq->m_ustatus, *(int*) &rgBuf[sizeof(tcBuffer)]);

    if(pReq->m_ustatus == 0)
        LOG_INFO( "Deleting VM entry from vRTM is successful for VM uuid %s\n", uuid);
    else {
        LOG_ERROR( "Deleting VM entry from vRTM is failed for VM uuid %s\n", uuid);
        return -1;
    }

    retval = *(int*) &rgBuf[sizeof(tcBuffer)];
    //return retval;

fail:
    LOG_TRACE("Closing channel");
    if (fd1 >= 0)
        ch_close (fd1);
    fd1 = 0;
    return retval;
}

// Add VM name and rp_domid mapping in map for later use.
// Once VM get launched this information will be used to update VM UUID - rp_domid mapping in vRTM
void update_rp_domid(int rp_domid, char* vm_name) {
	std::string vm_name_str(vm_name);
	LOG_DEBUG("Adding vm - rp_domid mapping in map for vm_name = %s and rp_domid = %d", vm_name, rp_domid);
	rp_id_map[vm_name_str]=rp_domid; 
}

// read the data over the socket from RP Proxy. Extract dom id and VM name from the request and 
// call rpcore wrapper function to updated this dom id with actual VM UUID.
void *conn_handler(void *socket_desc) {

    int     sock = *(int*) socket_desc;
    int     read_size;
    char    msg[MAX_MSG_SIZE];
    int     rp_domid, name_len;
    char    vm_name[2048];
    char    *resp;
    int     response = 1;

    LOG_TRACE("Start processing request in separate thread");
    resp = (char*) malloc(sizeof(int));
    memcpy(resp, &response, sizeof(int));

    LOG_TRACE("Reading date from socket");
    if ((read_size = recv(sock, msg, MAX_MSG_SIZE, 0)) > 0 ) {
        memcpy(&rp_domid, msg, sizeof(int));
        memcpy(&name_len, &msg[sizeof(int)], sizeof(int));
        memcpy(&vm_name, &msg[2*sizeof(int)], name_len);
        vm_name[name_len] = '\0';

        LOG_DEBUG("Received data from socket: rp_domid=%d, vm_name=%s", rp_domid, vm_name);
        update_rp_domid(rp_domid, vm_name);
        LOG_DEBUG("Writing response to socket: Response=%d", resp);
        write(sock, resp, sizeof(int));
    }
     
    if (read_size == 0) {
        LOG_INFO("Client disconnected");
    }
    else if (read_size == -1) {
        LOG_ERROR( "Recv failed");
    }
    free(socket_desc);
    free(resp);
    return 0;
}

// create a TCP server to listen to vRTM Proxy requests, this is a multithreaded server
// On receiving a request, create a thread and process it
void* listen_rp_proxy_requests(void* data) {

    int socket_desc , client_sock , c , *new_sock;
    struct sockaddr_in server , client;
     
    LOG_TRACE("Thread started to start a socket");
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);

    if (socket_desc == -1) {
        LOG_ERROR( "Could not create socket to listen");
	exit_status = 0;
    }
    LOG_TRACE("Socket has been created");
 
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(TCP_LISTEN_PORT);
    
    LOG_TRACE("Binding socket"); 
    if (bind(socket_desc, (struct sockaddr *)&server , sizeof(server)) < 0) {
        LOG_ERROR("Bind failed");
        LOG_ERROR("Exiting thread");
	exit_status = 0;
        return 1;
    }
    LOG_TRACE("Binded successfully");

    listen(socket_desc, 10);
    c = sizeof(struct sockaddr_in);

    LOG_INFO("Listening for new requests from vRTM proxy");

    while ((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
        
        LOG_TRACE("Connection accepted from client");
        pthread_t th;
        new_sock = (int*) malloc(sizeof(int));
        *new_sock = client_sock;
         
        if (pthread_create(&th, NULL,  conn_handler, (void*)new_sock) < 0) {
            LOG_ERROR("Failed to create thread. Request will not get processed");
            LOG_INFO("Continue listening for next request");
        } 
        else
            LOG_TRACE( "Handler assigned for processing request");
    }
     
    if (client_sock < 0) {
        LOG_ERROR( "Accept failed");
	exit_status = 0;
        return 1;
    }
     
    return 0;
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

    pthread_t th1, th2;

    // create a thread to start TCP server to listen requests from RP Proxy
    LOG_DEBUG("Creating a thread to start listening on socket");
    if (pthread_create(&th1, NULL, &listen_rp_proxy_requests, NULL) < 0) {
        LOG_ERROR( "Failed to create thread for listening on socket");
        closeLog();
        return 1;
    } else
        LOG_TRACE( "Created thread for to start listening rp_proxy requests on socket");

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

