

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#include <sys/un.h>
#endif
#include <pthread.h>
#include <signal.h>
#ifdef LINUX
#include <linux/un.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <map>
#include <string>

#include "logging.h"
#include "log_vrtmchannel.h"
#include "tcconfig.h"
#include "vrtminterface.h"
#include "vrtm_listener.h"
#include "loadconfig.h"
#include "win_headers.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __linux__
#include "safe_lib.h"
#endif
#ifdef __cplusplus
}
#endif

#define    g_config_file "../configuration/vRTM.cfg"
#define	   log_properties_file "../configuration/vrtm_log.properties"

char    g_vrtmcore_ip [64]        = "127.0.0.1";
int     g_vrtmcore_port 		= 16005;
int     g_max_thread_limit 	= 64;
char    g_vrtm_root[64] = "../";
#ifdef __linux__
char 	g_trust_report_dir[512] = "/var/log/trustreports/";
#elif _WIN32
char 	g_trust_report_dir[512] = "C:/OpenStack/Log/trustreports";
#endif
long 	g_entry_cleanup_interval = 30;
//long 	g_delete_vm_max_age = 3600;
long 	g_cancelled_vm_max_age = 86400;
//long	g_stopped_vm_max_age = 864000;
char    g_deployment_type[8] = "vm";

char* 	g_mount_path = "/mnt/vrtm/";
std:: map<std::string, std::string> config_map;

//default signal handlers function pointers
//__sighandler_t default_handle;
void ( * default_handler_abrt) (int );
void ( * default_handler_hup) (int );
void ( * default_handler_int) (int );
void ( * default_handler_quit) (int );
void ( * default_handler_segv) (int );
void ( * default_handler_term) (int );
void ( * default_handler_fpe) (int );
void ( * default_handler_bus) (int );
void ( * default_handler_tstp) (int );

int  g_quit = 0;
#ifdef _WIN32
HANDLE g_fdLock;
#elif __linux__
int g_fdLock;
#endif

int read_config()
{
	bool create_report_dir = false;
	struct stat info;
	int count=0;
	int ret_val;
	std::string vrtmcore_ip, vrtmcore_port, max_thread_limit, vrtm_root, trust_report_dir;
	std::string entry_cleanup_interval, delete_vm_max_age, cancelled_vm_max_age, stopped_vm_max_age, deployment_type;

	LOG_TRACE("Setting vRTM configuration");
	vrtmcore_ip = config_map["vrtmcore_ip"];
	if(vrtmcore_ip == ""){
		vrtmcore_ip = "127.0.0.1";
		LOG_WARN("vRTM IP is not found in vRTM.cfg. Using default IP %s", vrtmcore_ip.c_str());
	}
	count++;
	vrtmcore_port = config_map["vrtmcore_port"];
	if(vrtmcore_port == ""){
		vrtmcore_port = "16005";
		LOG_WARN("vRTM Port No. is not found in vRTM.cfg. Using default Port %s", vrtmcore_port.c_str());
	}
	count++;
	max_thread_limit = config_map["max_thread_limit"];
	if(max_thread_limit == ""){
		max_thread_limit = "63";
		LOG_WARN("Thread Limit for vRTM is not found in vRTM.cfg. Using default limit %s", max_thread_limit.c_str());
	}
	count++;
	vrtm_root = config_map["vrtm_root"];
	if (vrtm_root == "") {
		vrtm_root = "../";
		LOG_WARN("Vrtm Root is not found in vRTM.cfg. Using default location %s", vrtm_root.c_str());
	}
	count++;
	trust_report_dir = config_map["trust_report_dir"];
	if (trust_report_dir == "") {
#ifdef _WIN32
		trust_report_dir = "C:/OpenStack/Log/trustreports";
#elif __linux__
		trust_report_dir = "/var/log/trustreports/";
#endif
		LOG_WARN("Trust Report directory is not found in vRTM.cfg. Using default location %s", trust_report_dir.c_str());
	}
	count++;
	entry_cleanup_interval = config_map["entry_cleanup_interval"];
	if (entry_cleanup_interval == "") {
		entry_cleanup_interval = "30";
		LOG_WARN("VM Entry cleanup interval not found in vRTM.cfg. Using default value : %d", entry_cleanup_interval.c_str());
	}
	count++;
	/*delete_vm_max_age = config_map["delete_vm_max_age"];
	if (delete_vm_max_age == "") {
		LOG_WARN("Deleted VM cleanup interval not found in vRTM.cfg. Using default value : %d", g_delete_vm_max_age);
	}
	count++;*/
	cancelled_vm_max_age = config_map["cancelled_vm_max_age"];
	if (cancelled_vm_max_age == "") {
		cancelled_vm_max_age = "86400";
		LOG_WARN("Cancelled VM cleanup interval not found in vRTM.cfg. Using default value : %d", cancelled_vm_max_age.c_str());
	}
	count++;
	/*stopped_vm_max_age = config_map["stopped_vm_max_age"];
	if (stopped_vm_max_age == "") {
		LOG_WARN("Stopped VM cleanup interval not found in vRTM.cfg. Using default value : %d", g_stopped_vm_max_age);
	}
	count++;*/
	deployment_type = config_map["deployment_type"];
	if (deployment_type == "") {
		LOG_WARN("Deployment type not found in vRTM.cfg. Using default value : %d", g_deployment_type);
	}
	count++;
	strcpy_s(g_vrtmcore_ip,sizeof(g_vrtmcore_ip), vrtmcore_ip.c_str());
	LOG_DEBUG("vRTM IP : %s", g_vrtmcore_ip);
	g_vrtmcore_port = atoi(vrtmcore_port.c_str());
	LOG_DEBUG("vRTM listening port : %d", g_vrtmcore_port);
	g_max_thread_limit = atoi(max_thread_limit.c_str());
	LOG_DEBUG("vRTM Max concurrent request processing limit : %d", g_max_thread_limit);
	strcpy_s(g_vrtm_root, sizeof(g_vrtm_root), vrtm_root.c_str());
	strcat_s(g_vrtm_root, sizeof(g_vrtm_root), "/");
	LOG_DEBUG("vRTM root : %s", g_vrtm_root);
	strcpy_s(g_trust_report_dir,sizeof(g_trust_report_dir),trust_report_dir.c_str());
	strcat_s(g_trust_report_dir,sizeof(g_trust_report_dir),"/");
	LOG_DEBUG("vRTM trust report directory : %s", g_trust_report_dir);
	g_entry_cleanup_interval = atoi(entry_cleanup_interval.c_str());
	LOG_DEBUG("VM Entry cleanup interval : %d", g_entry_cleanup_interval);
	//g_delete_vm_max_age = atoi(delete_vm_max_age.c_str());
	//LOG_DEBUG("Deleted VM cleanup interval : %d", g_delete_vm_max_age);
	g_cancelled_vm_max_age = atoi(cancelled_vm_max_age.c_str());
	LOG_DEBUG("Cancelled VM cleanup interval : %d", g_cancelled_vm_max_age);
	//g_stopped_vm_max_age = atoi(stopped_vm_max_age.c_str());
	//LOG_DEBUG("Stopped VM cleanup interval : %d", g_stopped_vm_max_age);
	strcpy_s(g_deployment_type,sizeof(g_deployment_type), deployment_type.c_str());
	LOG_DEBUG("Deployment type : %s", g_deployment_type);

	// clear all data in trust report directory if directory already exists
	if( stat( g_trust_report_dir, &info ) != 0 ) {
	    LOG_DEBUG( "cannot access %s\n", g_trust_report_dir );
	    LOG_DEBUG( "New trust report directory %s will be created", g_trust_report_dir);
	    create_report_dir = true;
	}
	else if( info.st_mode & S_IFDIR ) { // S_ISDIR() doesn't exist on my windows
		LOG_DEBUG( "%s is a directory and already exists", g_trust_report_dir );
		LOG_INFO("%s will be cleaned", g_trust_report_dir);
		if (remove_dir(g_trust_report_dir) == 0) {
			LOG_INFO("Trust report directory %s, cleaned successfully", g_trust_report_dir);
		}
		else {
			LOG_ERROR("Trust report directory %s, couldn't be cleaned", g_trust_report_dir);
		}
	}
	else {
	    LOG_DEBUG( "%s is present but not a directory\n", g_trust_report_dir );
	    create_report_dir = true;
	}
	// create trust report directory if not exist
	if (create_report_dir) {
		if (make_dir(g_trust_report_dir) != 0) {
			return -1;
		}
	}
#ifdef __linux__
	chmod(g_trust_report_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	// create mount location for vRTM at /mnt/vrtm
	if (make_dir(g_mount_path) != 0) {
		return -1;
	}
#endif
	return count;
}

int singleInstanceRpcoreservice()
{
#ifdef __linux__
	const char *lockFile="/tmp/rpcoreservice__DT_XX99";
    struct flock rpcsFlock;
    
    rpcsFlock.l_type = F_WRLCK;
    rpcsFlock.l_whence = SEEK_SET;
    rpcsFlock.l_start = 0;
    rpcsFlock.l_len = 1;

    if( ( g_fdLock = open(lockFile, O_WRONLY | O_CREAT, 0666)) == -1) 
    {
    	LOG_ERROR("Can't open rpcoreservice lock file \n");
        return -1;
     }
     
     chmod(lockFile, 0666); // Just in case umask is not set properly.
     
     if ( fcntl(g_fdLock, F_SETLK, &rpcsFlock) == -1) {
    	LOG_ERROR( "Already locked - rpcoreservice lock file \n");
		return -2;
     }
#elif _WIN32
    const char *lockFile="../rpcoreservice__DT_XX99";
	if ( ( g_fdLock = CreateFile((LPCSTR)lockFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, 2, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		//LOG_ERROR("Unable get Handle of lock file. \n Another instance of vRTM might be running");
		return -2;
	}
#endif

    return 1;
}

void vrtm_signal_handler(int sig_caught) {
	std::string signal_name;
	switch(sig_caught) {
	case SIGABRT:
		signal_name = "SIGABRT";
		signal(sig_caught, *default_handler_abrt);
		break;
	case SIGINT:
		signal_name = "SIGINT";
		signal(sig_caught, *default_handler_int);
		break;
	case SIGSEGV:
		signal_name = "SIGSEGV";
		signal(sig_caught, *default_handler_segv);
		break;
	case SIGTERM:
		signal_name = "SIGTERM";
		signal(sig_caught, *default_handler_term);
		break;
	case SIGFPE:
		signal_name = "SIGFPE";
		signal(sig_caught, *default_handler_fpe);
		break;
#ifdef __linux__
	case SIGHUP:
		signal_name = "SIGHUP";
		signal(sig_caught, *default_handler_hup);
		break;
	case SIGQUIT:
		signal_name = "SIGQUIT";
		signal(sig_caught, *default_handler_quit);
		break;
	case SIGBUS:
		signal_name = "SIGBUS";
		signal(sig_caught, *default_handler_bus);
		break;
#endif
	}
	LOG_INFO("caught signal %s", signal_name.c_str());
	// clean the trust report directory
	if (remove_dir(g_trust_report_dir) == 0) {
		LOG_INFO("Trust report directory %s, cleaned successfully", g_trust_report_dir);
	}
	else {
		LOG_ERROR("Trust report directory %s, couldn't be cleaned", g_trust_report_dir);
	}
	LOG_INFO("vRTM will Exit with recieved signal %d", sig_caught);

	raise(sig_caught);
}


int main(int an, char** av)
{
    int            	iRet= 0;
    int 			instanceValid = 0;
    pthread_t vrtm_listener_thread, vrtm_interface_thread;
    
    // Start the instance of logger, currently using log4cpp
    if( initLog(log_properties_file, "vrtm") ){
    	return 1;
    }
    //set same logger instance in rp_channel
    set_logger_vrtmchannel(rootLogger);


    LOG_INFO("Starting vRTM core");
    if ((instanceValid = singleInstanceRpcoreservice()) == -2) {
    	LOG_ERROR("Process(vRTM core service) already running\n");
		return 1;
    }
    
    if(instanceValid == -1) {
    	LOG_ERROR( "Process (vRTM core service) could not open lock file\n");
		return 1;
    }

    //Signal handling and clean our trust reports
    default_handler_abrt = signal(SIGABRT, vrtm_signal_handler); //handle abort signal , arise when program itself detect error
    default_handler_int = signal(SIGINT, vrtm_signal_handler);  //handle signal interrupt ctrl-c
    //signal(SIGKILL, vrtm_signal_handler); //handle kill -9 but can't be handled or ignored or blocked
	default_handler_segv = signal(SIGSEGV, vrtm_signal_handler); //handle segmentation fault
    default_handler_term = signal(SIGTERM, vrtm_signal_handler); //handle the kill command
    //signal(SIGSTOP, vrtm_signal_handler); //handle suspend signals ctrl-s -- can't be handled or ignored or blocked
    default_handler_fpe = signal(SIGFPE, vrtm_signal_handler);  //to handle divide by zero errors
#ifdef __linux__
	default_handler_hup = signal(SIGHUP, vrtm_signal_handler);  //handle shell session termination
	default_handler_quit = signal(SIGQUIT, vrtm_signal_handler); //handle quit command given by ctrl-\ ---
	default_handler_bus = signal(SIGBUS, vrtm_signal_handler);  //handle invalid pointer dereferencing different from sigsegv
#endif
    //default_handler_tstp = signal(SIGTSTP, vrtm_signal_handler); //to handle interactive stop signal ctrl-z and ctrl-y, don't need to clean, process can be resumed again

    LOG_TRACE("Load config file %s", g_config_file);
	if ( load_config(g_config_file, config_map) < 0 ) {
		LOG_ERROR("Can't load config file %s", g_config_file);
		goto cleanup;
	}

    LOG_TRACE("Read configurations");
	if ( read_config() < 0)
	{
		LOG_ERROR("tcService main : cant't find required values in config file");
		goto cleanup;
	}

	clear_config(config_map);

#ifdef _WIN32
	// Uses WMI calls to HyperV interface. So no need of vRTM listener.
	LOG_TRACE("Starting vRTM interface");
	if (!start_vrtm_interface(NULL)) {
		LOG_ERROR("Can't initialize vRTM interface");
		goto cleanup;
	}
#elif __linux__
    if (strcmp(g_deployment_type, "docker") != 0) {
        LOG_TRACE("Starting vRTM listener");
        if (pthread_create(&vrtm_listener_thread, NULL, &start_vrtm_listener, (void*)NULL) < 0) {
            LOG_ERROR( "Failed to start vrtm listener");
        } else
            LOG_INFO("vrtm listener started");
    }

    LOG_TRACE("Starting vRTM interface");
    if (pthread_create(&vrtm_interface_thread, NULL, &start_vrtm_interface, (void*)NULL) < 0) {
        LOG_ERROR( "Failed to start vrtm interface");
    } else
        LOG_INFO("vrtm interface started");

        pthread_join(vrtm_interface_thread,NULL);

        LOG_DEBUG("main exited");
#endif
        return iRet;

cleanup:
#ifdef _WIN32
	  CloseHandle(g_fdLock);
#elif __linux__
	  close(g_fdLock);
#endif
	  closeLog();
	  return iRet;
}


