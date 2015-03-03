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
#include "tcpchan.h"
#include "channelcoding.h"
#include "pyifc.h"
#include "rp_api_code.h"

#define UUID_LENGTH         36
#define TCP_REQUEST_SIZE    512
#define TCP_LISTEN_PORT     16004
#define PARAMSIZE           8192
#define RPCORESERVICE_PORT  6005
#define MAX_MSG_SIZE        4096

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

int channel_open() {
    int fd = -1;

#ifndef USE_DRV_CHANNEL //not defined
    fd =  ch_open(NULL, 0);
#else
    fd = open("/dev/chandu", O_RDWR);

#endif
    if(fd < 0) {
        fprintf(stdout, "Open error chandu: %s\n", strerror(errno));
        return -1;
    }
    ch_register(fd);
    return fd;
}

static int domainEventCallback(virConnectPtr conn ATTRIBUTE_UNUSED, virDomainPtr dom, int event,
                                  int detail, void *opaque ATTRIBUTE_UNUSED) {

    int eventType = (virDomainEventType) event;
    bool notifyRPCore = false;

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
    } else if (eventType == VIR_DOMAIN_EVENT_CRASHED) {
        notifyRPCore = true;
    }

    if (notifyRPCore) {
        char vm_uuid[UUID_LENGTH+1];
        fprintf(stdout, "Notifying rpcore for shutdown of domain %s\n", __func__, virDomainGetName(dom));
        virDomainGetUUIDString(dom, vm_uuid);
        //send the request to RPCore
        delete_rp_uuid(vm_uuid);
    }

    return 0;
}

static void connectClose(virConnectPtr conn ATTRIBUTE_UNUSED, int reason, 
                            void *opaque ATTRIBUTE_UNUSED) {
    run = 0;
}

static void freeFunc(void *opaque) {
    char *str = opaque;
    free(str);
}


static void stopOnSignal(int sig) {
    fprintf(stdout, "Exiting on signal %d\n", sig);
    run = 0;
}

// register with libvirt and listen to libvirt events
void* listen_libvirt_events( void* input) {

    struct sigaction action_stop;
    memset(&action_stop, 0, sizeof(action_stop));
    action_stop.sa_handler = stopOnSignal;

    if (virInitialize() < 0) {
        fprintf(stderr, "listen_libvirt_events(): Failed to initialize libvirt");
        return 1;
    }

    if (virEventRegisterDefaultImpl() < 0) {
        virErrorPtr err = virGetLastError();
        fprintf(stderr, "listen_libvirt_events(): Failed to register event implementation: %s\n",
                err && err->message ? err->message: "Unknown error");
        return 1;
    }

    dconn = virConnectOpenAuth(NULL, virConnectAuthPtrDefault, VIR_CONNECT_RO);
    if (!dconn) {
        fprintf(stdout, "listen_libvirt_events(): error opening vir\n");
        return 1;
    }

    virConnectRegisterCloseCallback(dconn, connectClose, NULL, NULL);
    sigaction(SIGTERM, &action_stop, NULL);
    sigaction(SIGINT, &action_stop, NULL);

    if ( !virConnectDomainEventRegister(dconn, domainEventCallback, strdup("callback"), freeFunc)) {
        if (virConnectSetKeepAlive(dconn, 5, 3) < 0) {
            virErrorPtr err = virGetLastError();
            fprintf(stderr, "listen_libvirt_events(): Failed to start keepalive protocol: %s\n",
                    err && err->message ? err->message : "Unknown error");
            run = 0;
        }

        while (run) {
            if (virEventRunDefaultImpl() < 0) {
                virErrorPtr err = virGetLastError();
                fprintf(stderr, "listen_libvirt_events(): Failed to run event loop: %s\n",
                        err && err->message ? err->message : "Unknown error");
            }
        }
        virConnectDomainEventDeregister(dconn, domainEventCallback);
    }

    virConnectUnregisterCloseCallback(dconn, connectClose);
    VIR_DEBUG("listen_libvirt_events(): Closing connection");
    if (dconn && virConnectClose(dconn) < 0) {
        fprintf(stdout, "listen_libvirt_events(): error closing vir connection\n");
    }

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

    sprintf(buf, "%d", rpid);
    fprintf(stdout, "map_rpid_uuid(): Opening channel .. \n");

    if (!fd1)
        fd1 = channel_open();

    if(fd1 < 0) {
        fprintf(stdout, "map_rpid_uuid(): Open error channel: %s\n", strerror(errno));
        return -1;
    }

    //create and send the request to RPCore over the channel
    size = sizeof(tcBuffer);
    size= encodeVM2RP_SETUUID (buf, uuid, vdi_uuid, PARAMSIZE -size, (byte*)&rgBuf[size]);

    pReq->m_procid= 0;
    pReq->m_reqID=  VM2RP_SETUUID ;
    pReq->m_ustatus= 0;
    pReq->m_origprocid= 0;
    pReq->m_reqSize= size;

    fprintf(stdout, "map_rpid_uuid():Sending request size %d \n %s\n", 
                size + sizeof(tcBuffer), &rgBuf[20]);
    
    err = ch_write(fd1, rgBuf, size + sizeof(tcBuffer) );
    if (err < 0){
        fprintf(stdout, "map_rpid_uuid():write error: %s\n", strerror(errno));
	retval = 0;
        goto fail;
    }

    memset(rgBuf, 0, sizeof(rgBuf));
    fprintf(stdout, "map_rpid_uuid():Sent request ..........\n");
    
again:  
    err = ch_read(fd1, rgBuf, sizeof(rgBuf));
    if (err < 0){
        if (errno == 11) {
            sleep(1);
            goto again;
        }

        fprintf(stdout, "map_rpid_uuid():read error:%d  %s\n", errno, strerror(errno));
	retval = 0;
        goto fail;
    }


    printf( "map_rpid_uuid():Response  from server status %d return %d\n", 
                pReq->m_ustatus, *(int*) &rgBuf[sizeof(tcBuffer)]);

    if(pReq->m_ustatus == 0)
        printf( "map_rpid_uuid():mapping uuid was successfull for VM uuid %s\n", uuid);
    else {
        printf( "map_rpid_uuid():mapping uuid failed for VM uuid %s\n", uuid);
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

    fprintf(stdout, "delete_rp_uuid(): Opening channel .. \n");

    if (!fd1)
        fd1 = channel_open();

    if(fd1 < 0)
    {
            fprintf(stdout, "delete_rp_uuid(): Open error channel: %s\n", strerror(errno));
            return -1;
    }

    //create and send the request to RPCore over the channel
    size = sizeof(tcBuffer);
    size= encodeVM2RP_TERMINATEAPP (strlen(uuid), uuid, PARAMSIZE -size, (byte*)&rgBuf[size]);

    pReq->m_procid= 0;
    pReq->m_reqID=  VM2RP_TERMINATEAPP ;
    pReq->m_ustatus= 0;
    pReq->m_origprocid= 0;
    pReq->m_reqSize= size;

    fprintf(stdout, "delete_rp_uuid(): Requesting rpcore to remove entry for vm with uuid %s\n", uuid);
    fprintf(stdout, "delete_rp_uuid(): Sending request size %d \n %s\n", 
                size + sizeof(tcBuffer), &rgBuf[20]);
    
    err = ch_write(fd1, rgBuf, size + sizeof(tcBuffer) );
    if (err < 0){
        fprintf(stdout, "delete_rp_uuid(): write error: %s\n", strerror(errno));
	retval = -1;
        goto fail;
    }

    memset(rgBuf, 0, sizeof(rgBuf));
    fprintf(stdout, "delete_rp_uuid(): Sent request ..........\n");
    
again:  
    err = ch_read(fd1, rgBuf, sizeof(rgBuf));
    if (err < 0){
        if (errno == 11) {
            sleep(1);
            goto again;
        }

        fprintf(stdout, "delete_rp_uuid(): read error:%d  %s\n", errno, strerror(errno));
	retval = -1;
        goto fail;
    }

    printf( "delete_rp_uuid(): Response  from server status %d return %d\n", pReq->m_ustatus, *(int*) &rgBuf[sizeof(tcBuffer)]);

    if(pReq->m_ustatus == 0)
        printf( "delete_rp_uuid():deleting rpcore entry was successfull for VM uuid %s\n", uuid);
    else {
        printf( "map_rpid_uuid():deleting rpcore entry failed for VM uuid %s\n", uuid);
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

    char vm_uuid[UUID_LENGTH+1];
    virDomainPtr dom = virDomainLookupByName(dconn, vm_name);
    virDomainGetUUIDString(dom, vm_uuid);
    fprintf(stdout, "update_rp_id(): replacing %d with %s\n", rp_domid, vm_uuid);
    map_rpid_uuid(rp_domid, vm_uuid);
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

    resp = (char*) malloc(sizeof(int));
    memcpy(resp, &response, sizeof(int));

    if ((read_size = recv(sock, msg, MAX_MSG_SIZE, 0)) > 0 ) {
        memcpy(&rp_domid, msg, sizeof(int));
        memcpy(&name_len, &msg[sizeof(int)], sizeof(int));
        memcpy(&vm_name, &msg[2*sizeof(int)], name_len);
        vm_name[name_len] = '\0';

        fprintf(stdout, "conn_handler(): rp_domid=%d, vm_name=%s\n", rp_domid, vm_name);
        update_rp_domid(rp_domid, vm_name);
        write(sock, resp, sizeof(int));
    }
     
    if (read_size == 0) {
        fprintf(stdout, "conn_handler(): client disconnected\n");
    }
    else if (read_size == -1) {
        fprintf(stdout, "conn_handler(): recv failed\n");
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
     
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);

    if (socket_desc == -1) {
        fprintf(stdout, "listen_rp_proxy_requests(): could not create socket to listen\n");
    }
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(TCP_LISTEN_PORT);
     
    if (bind(socket_desc, (struct sockaddr *)&server , sizeof(server)) < 0) {
        fprintf(stdout, "listen_rp_proxy_requests(): bind failed. Error\n");
        return 1;
    }
     
    listen(socket_desc, 10);
    c = sizeof(struct sockaddr_in);

    fprintf(stdout, "listen_rp_proxy_requests(): listening for new connections\n");

    while ((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
        
        fprintf(stdout, "listen_rp_proxy_requests(): connection accepted from client\n");
        pthread_t th;
        new_sock = (int*) malloc(sizeof(int));
        *new_sock = client_sock;
         
        if (pthread_create(&th, NULL,  conn_handler, (void*)new_sock) < 0) {
            fprintf(stdout, "listen_rp_proxy_requests(): failed to create thread\n");
            return 1;
        }
        fprintf(stdout, "listen_rp_proxy_requests(): Handler assigned for tcp connection\n");
        sleep(1);
    }
     
    if (client_sock < 0) {
        fprintf(stdout, "listen_rp_proxy_requests(): accept failed\n");
        return 1;
    }
     
    return 0;
}


int main() {

    if (init_pyifc("rppy_ifc") < 0 )
        return -1;

    struct sigaction sigAct;
    memset(&sigAct, 0, sizeof(sigAct));
    sigAct.sa_handler = SIG_DFL;
    sigAct.sa_flags = SA_NOCLDWAIT;

    int sigRv = sigaction(SIGCHLD, &sigAct, NULL);

    if (sigRv < 0) {
        fprintf(stdout, "Failed to set signal disposition for SIGCHLD\n");
    } else {
        fprintf(stdout, "Set SIGCHLD to avoid zombies\n");
    }

    pthread_t th1, th2;

    // create a thread to start TCP server to listen requests from RP Proxy
    if (pthread_create(&th1, NULL, &listen_rp_proxy_requests, NULL) < 0) {
        fprintf(stdout, "failed to create thread\n");
        return 1;
    } else
        fprintf(stdout, "created thread for listening rp_proxy requests\n");

    // create a thread to listen libvirt events
    if (pthread_create(&th2, NULL, &listen_libvirt_events, NULL) < 0) {
        fprintf(stdout, "failed to create thread\n");
        return 1;
    } else
        fprintf(stdout, "created thread for listening libvirt events\n");

    while(1) {
        sleep(1);
    }

    deinit_pyifc();

    return 0;
}



