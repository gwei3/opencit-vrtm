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
#include <config.h>
#include <map>
#include <iostream>
#include "tcpchan.h"
#include "channelcoding.h"
#include "pyifc.h"
#include "rp_api_code.h"

#include "logging.h"
#include "log_rpchannel.h"

#define UUID_LENGTH         36
#define TCP_REQUEST_SIZE    512
#define TCP_LISTEN_PORT     16004
#define PARAMSIZE           8192
#define RPCORESERVICE_PORT  6005
#define MAX_MSG_SIZE        4096

#define log_properties_file "../configuration/rp_listenerlog.properties"

#define VIR_DEBUG(fmt) printf("%s:%d: " fmt "\n", __func__, __LINE__)
#define STREQ(a,b) (strcmp(a,b) == 0)

#ifndef ATTRIBUTE_UNUSED
# define ATTRIBUTE_UNUSED __attribute__((__unused__))
#endif

#ifndef byte
typedef unsigned char byte;
#endif

static int fd1;
virConnectPtr dconn = NULL;
int g_fd = 0;
int run = 1;
int delete_rp_uuid(char*);
int map_rpid_uuid(int, char*);
std::map<std::string, int> rp_id_map;
static int exit_status = 0;


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
    LOG_TRACE("");
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
		LOG_INFO( "In VIR_DOMAIN_EVENT_STARTED event");
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
	LOG_TRACE("");
    run = 0;
}

static void freeFunc(void *opaque) {
    char *str = opaque;
    LOG_TRACE("");
    free(str);
}


static void stopOnSignal(int sig) {

	LOG_TRACE( "Exiting on signal %d", sig);
    run = 0;
}

// register with libvirt and listen to libvirt events
void* listen_libvirt_events( void* input) {

    struct sigaction action_stop;
    memset(&action_stop, 0, sizeof(action_stop));
    action_stop.sa_handler = stopOnSignal;
    LOG_TRACE("");
    if (virInitialize() < 0) {
        LOG_ERROR("listen_libvirt_events(): Failed to initialize libvirt");
		exit_status = 1;
        return 1;
    }

    if (virEventRegisterDefaultImpl() < 0) {
        virErrorPtr err = virGetLastError();
        LOG_ERROR("listen_libvirt_events(): Failed to register event implementation: %s",
                err && err->message ? err->message: "Unknown error");
		exit_status = 1;
        return 1;
    }

    dconn = virConnectOpenAuth(NULL, virConnectAuthPtrDefault, VIR_CONNECT_RO);
    if (!dconn) {
    	LOG_ERROR( "listen_libvirt_events(): error opening vir");
		exit_status = 1;
        return 1;
    }

    virConnectRegisterCloseCallback(dconn, connectClose, NULL, NULL);
    sigaction(SIGTERM, &action_stop, NULL);
    sigaction(SIGINT, &action_stop, NULL);

    if ( !virConnectDomainEventRegister(dconn, domainEventCallback, strdup("callback"), freeFunc)) {
        if (virConnectSetKeepAlive(dconn, 5, 3) < 0) {
            virErrorPtr err = virGetLastError();
            LOG_ERROR( "listen_libvirt_events(): Failed to start keepalive protocol: %s",
                    err && err->message ? err->message : "Unknown error");
            run = 0;
        }

        while (run) {
            if (virEventRunDefaultImpl() < 0) {
                virErrorPtr err = virGetLastError();
                LOG_ERROR("listen_libvirt_events(): Failed to run event loop: %s",
                        err && err->message ? err->message : "Unknown error");
            }
        }
        virConnectDomainEventDeregister(dconn, domainEventCallback);
    }

    virConnectUnregisterCloseCallback(dconn, connectClose);
    VIR_DEBUG("listen_libvirt_events(): Closing connection");
    LOG_DEBUG("listen_libvirt_events(): Closing connection");
    if (dconn && virConnectClose(dconn) < 0) {
        LOG_ERROR( "listen_libvirt_events(): error closing vir connection");
    }
	exit_status = 1;
    return 0;
}

// send request to RPCore to replace the dom id with actual VM UUID in RPCore data structures
int map_rpid_uuid(int rpid, char* uuid) {
    
    char buf[32] = {0};
    int err = -1;
    int retval;
    
    int         size= PARAMSIZE;
    byte        rgBuf[PARAMSIZE];
    tcBuffer*   pReq= (tcBuffer*) rgBuf;
    char*       vdi_uuid = ""; 

    LOG_TRACE("");
    sprintf(buf, "%d", rpid);
    LOG_TRACE( "map_rpid_uuid(): Opening channel ..");

    if (!fd1)
        fd1 = channel_open();

    if(fd1 < 0) {
        LOG_ERROR( "map_rpid_uuid(): Open error channel: %s", strerror(errno));
        return -1;
    }

    //create and send the request to RPCore over the channel
    size = sizeof(tcBuffer);
    size= encodeVM2RP_SETUUID (buf, uuid, vdi_uuid, PARAMSIZE -size, (byte*)&rgBuf[size]);

    ///pReq->m_procid= 0;
    pReq->m_reqID=  VM2RP_SETUUID ;
    pReq->m_ustatus= 0;
    //pReq->m_origprocid= 0;
    pReq->m_reqSize= size;

    LOG_DEBUG("map_rpid_uuid():Sending request size %d \n %s",
                size + sizeof(tcBuffer), &rgBuf[20]);
    
    err = ch_write(fd1, rgBuf, size + sizeof(tcBuffer) );
    if (err < 0){
        LOG_ERROR("map_rpid_uuid():write error: %s", strerror(errno));
	retval = 0;
        goto fail;
    }

    memset(rgBuf, 0, sizeof(rgBuf));
    LOG_INFO( "map_rpid_uuid():Sent request ..........");
    
again:  
    err = ch_read(fd1, rgBuf, sizeof(rgBuf));
    if (err < 0){
        if (errno == 11) {
            sleep(1);
            goto again;
        }

        LOG_ERROR( "map_rpid_uuid():read error:%d  %s", errno, strerror(errno));
	retval = 0;
        goto fail;
    }


    LOG_DEBUG( "map_rpid_uuid():Response  from server status %d return %d",
                pReq->m_ustatus, *(int*) &rgBuf[sizeof(tcBuffer)]);

    if(pReq->m_ustatus == 0)
        LOG_INFO( "map_rpid_uuid():mapping uuid was successfull for VM uuid %s", uuid);
    else {
        LOG_ERROR( "map_rpid_uuid():mapping uuid failed for VM uuid %s", uuid);
        return -1;
    }

    retval = *(int*) &rgBuf[sizeof(tcBuffer)];
    //return retval;

fail:
    if ( fd1 >= 0)
        ch_close (fd1);
    fd1 = 0;
    return retval;
}

// Send request to RPCore to remove the entry for a particular VM
int delete_rp_uuid(char* uuid) {
    
    int err = -1;
    int retval;
    
    int         size= PARAMSIZE;
    byte        rgBuf[PARAMSIZE];
    tcBuffer*   pReq= (tcBuffer*) rgBuf;
    LOG_TRACE( "delete_rp_uuid(): Opening channel .. ");

    if (!fd1)
        fd1 = channel_open();

    if(fd1 < 0)
    {
            LOG_ERROR( "delete_rp_uuid(): Open error channel: %s", strerror(errno));
            return -1;
    }

    //create and send the request to RPCore over the channel
    size = sizeof(tcBuffer);
    size= encodeVM2RP_TERMINATEAPP (strlen(uuid), uuid, PARAMSIZE -size, (byte*)&rgBuf[size]);

//    pReq->m_procid= 0;
    pReq->m_reqID=  VM2RP_TERMINATEAPP ;
    pReq->m_ustatus= 0;
    //pReq->m_origprocid= 0;
    pReq->m_reqSize= size;

    LOG_DEBUG( "delete_rp_uuid(): Requesting rpcore to remove entry for vm with uuid %s", uuid);
    LOG_TRACE( "delete_rp_uuid(): Sending request size %d \n %s",
                size + sizeof(tcBuffer), &rgBuf[20]);
    
    err = ch_write(fd1, rgBuf, size + sizeof(tcBuffer) );
    if (err < 0){
        LOG_ERROR( "delete_rp_uuid(): write error: %s", strerror(errno));
	retval = -1;
        goto fail;
    }

    memset(rgBuf, 0, sizeof(rgBuf));
    LOG_TRACE( "delete_rp_uuid(): Sent request ..........\n");
    
again:  
    err = ch_read(fd1, rgBuf, sizeof(rgBuf));
    if (err < 0){
        if (errno == 11) {
            sleep(1);
            goto again;
        }

        LOG_ERROR("delete_rp_uuid(): read error:%d  %s\n", errno, strerror(errno));
	retval = -1;
        goto fail;
    }

    LOG_DEBUG( "delete_rp_uuid(): Response  from server status %d return %d\n", pReq->m_ustatus, *(int*) &rgBuf[sizeof(tcBuffer)]);

    if(pReq->m_ustatus == 0)
        LOG_INFO( "delete_rp_uuid():deleting rpcore entry was successfull for VM uuid %s\n", uuid);
    else {
        LOG_ERROR( "map_rpid_uuid():deleting rpcore entry failed for VM uuid %s\n", uuid);
        return -1;
    }

    retval = *(int*) &rgBuf[sizeof(tcBuffer)];
    //return retval;

fail:
    if ( fd1 >= 0)
        ch_close (fd1);
    fd1 = 0;
    return retval;
}

// wrapper on map_rpid_uuid. 
// get VM UUID using VM name and call the function to replace the domid with actual VM UUID.
void update_rp_domid(int rp_domid, char* vm_name) {
	std::string vm_name_str(vm_name);
	LOG_TRACE("");
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

    LOG_TRACE("");
    resp = (char*) malloc(sizeof(int));
    memcpy(resp, &response, sizeof(int));

    if ((read_size = recv(sock, msg, MAX_MSG_SIZE, 0)) > 0 ) {
        memcpy(&rp_domid, msg, sizeof(int));
        memcpy(&name_len, &msg[sizeof(int)], sizeof(int));
        memcpy(&vm_name, &msg[2*sizeof(int)], name_len);
        vm_name[name_len] = '\0';

        LOG_DEBUG("conn_handler(): rp_domid=%d, vm_name=%s", rp_domid, vm_name);
        update_rp_domid(rp_domid, vm_name);
        write(sock, resp, sizeof(int));
    }
     
    if (read_size == 0) {
        LOG_INFO("conn_handler(): client disconnected");
    }
    else if (read_size == -1) {
        LOG_ERROR( "conn_handler(): recv failed");
    }
    free(socket_desc);
    free(resp);
    return 0;
}

// create a TCP server to listen to RP Proxy requests, this is a multithreaded server
// On receiving a request, create a thread and process it
void* listen_rp_proxy_requests(void* data) {

    int socket_desc , client_sock , c , *new_sock;
    struct sockaddr_in server , client;
     
    LOG_TRACE("");
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);

    if (socket_desc == -1) {
        LOG_ERROR( "listen_rp_proxy_requests(): could not create socket to listen");
		exit_status = 1;
    }
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(TCP_LISTEN_PORT);
     
    if (bind(socket_desc, (struct sockaddr *)&server , sizeof(server)) < 0) {
        LOG_ERROR("listen_rp_proxy_requests(): bind failed. Error");
		exit_status = 1;
        return 1;
    }
     
    listen(socket_desc, 10);
    c = sizeof(struct sockaddr_in);

    LOG_TRACE("listen_rp_proxy_requests(): listening for new connections");

    while ((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
        
        LOG_TRACE("listen_rp_proxy_requests(): connection accepted from client");
        pthread_t th;
        new_sock = (int*) malloc(sizeof(int));
        *new_sock = client_sock;
         
        if (pthread_create(&th, NULL,  conn_handler, (void*)new_sock) < 0) {
            LOG_ERROR( "listen_rp_proxy_requests(): failed to create thread");
            return 1;
        }
        LOG_TRACE( "listen_rp_proxy_requests(): Handler assigned for tcp connection");
        sleep(1);
    }
     
    if (client_sock < 0) {
        LOG_ERROR( "listen_rp_proxy_requests(): accept failed");
		exit_status = 1;
        return 1;
    }
     
    return 0;
}


int main() {

    /*if (init_pyifc("rppy_ifc") < 0 )
        return -1;*/

    struct sigaction sigAct;
    memset(&sigAct, 0, sizeof(sigAct));
    sigAct.sa_handler = SIG_DFL;
    sigAct.sa_flags = SA_NOCLDWAIT;
//    const char * log_properties_file = "../../config/rp_listenerlog.properties";
    if( initLog(log_properties_file) ){
		return 1;
	}

    //set same logger instance in rp_channel
    set_logger_rpchannel(rootLogger);

    LOG_TRACE("");
    int sigRv = sigaction(SIGCHLD, &sigAct, NULL);

    if (sigRv < 0) {
        LOG_INFO( "Failed to set signal disposition for SIGCHLD");
    } else {
        LOG_INFO( "Set SIGCHLD to avoid zombies");
    }

    pthread_t th1, th2;

    // create a thread to start TCP server to listen requests from RP Proxy
    if (pthread_create(&th1, NULL, &listen_rp_proxy_requests, NULL) < 0) {
        LOG_ERROR( "failed to create thread");
        return 1;
    } else
        LOG_TRACE( "created thread for listening rp_proxy requests\n");

    // create a thread to listen libvirt events
    if (pthread_create(&th2, NULL, &listen_libvirt_events, NULL) < 0) {
        LOG_ERROR( "failed to create thread\n");
        return 1;
    } else
        LOG_TRACE( "created thread for listening libvirt events\n");

    while(!exit_status) {
        sleep(1);
    }

    //deinit_pyifc();

    return 0;
}



