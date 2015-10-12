//
//  File: vrtmcoremain.cpp
//  Description: tcService implementation
//
//	Copyright (c) 2012, Intel Corporation. 
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
#include <stdlib.h>
#include <map>
#include <string>

#include "logging.h"
#include "log_vrtmchannel.h"
#include "tcconfig.h"
#include "vrtminterface.h"

#define    g_config_file "../configuration/vRTM.cfg"
#define	   log_properties_file "../configuration/vrtm_log.properties"

char    g_vrtmcore_ip [64]        = "127.0.0.1";
int     g_vrtmcore_port 		= 16005;
int     g_max_thread_limit 	= 64;
char    g_vrtm_root[64] = "../";
char 	g_trust_report_dir[512]  = "/var/lib/nova/trustreports/";
char* 	g_mount_path = "/mnt/vrtm/";
long 	g_entry_cleanup_interval = 30;
//long 	g_delete_vm_max_age = 3600;
long 	g_cancelled_vm_max_age = 86400;
//long	g_stopped_vm_max_age = 864000;

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

std:: map<std::string, std::string> config_map;

int  g_quit = 0;

int g_fdLock;

inline std::string trim_right_copy(const std::string &s, const std::string &delimiters)
{
	return s.substr(0,s.find_last_not_of(delimiters) +1 );
}

inline std::string trim_left_copy(const std::string &s, const std::string &delimiters)
{
	return s.substr(s.find_first_not_of(delimiters));
}

inline std::string trim_copy(const std::string &s, const std::string delimiters)
{
	return trim_left_copy(trim_right_copy(s,delimiters),delimiters);
}

int LoadConfig(const char * configFile)
{
	FILE *fp = fopen(configFile,"r");
	char *line;
	int line_size = 512;
	char *key;
	char *value;
	LOG_TRACE("Loading vRTM config file %s", configFile);
	if(fp == NULL)
	{
		LOG_ERROR("Failed to load vRTM config file");
		return -1;
	}
	while(true)
	{
		line = (char *) calloc(1,sizeof(char) * 512);
		fgets(line,line_size,fp);
		if(feof(fp)) {
			free(line);
			break;
		}
        LOG_TRACE("Line read from config file: %s", line);
        if( line[0] == '#' ) {
        	LOG_DEBUG("Comment in configuration file : %s", &line[1]);
        	free(line);
        	continue;
        }
		key=strtok(line,"=");
		value=strtok(NULL,"=");
		std::string map_key (key);
		std::string map_value (value);
        LOG_TRACE("Parsed Key=%s and Value=%s", map_key.c_str(), map_value.c_str());

		std::pair<std::string, std::string> config_pair (trim_copy(map_key," \t\n"),trim_copy(map_value," \t\n"));
		config_map.insert(config_pair);
		free(line);
	}
	return config_map.size();
}

int read_config()
{
	bool create_report_dir = false;
	struct stat info;
	int count=0;
	int ret_val;
	std::string vrtmcore_ip, vrtmcore_port, max_thread_limit, vrtm_root, trust_report_dir;
	std::string entry_cleanup_interval, delete_vm_max_age, cancelled_vm_max_age, stopped_vm_max_age;

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
		trust_report_dir = "/var/lib/nova/trustreports/";
		LOG_WARN("Trust Report directory is not found in vRTM.cfg. Using default location %s", trust_report_dir.c_str());
	}
	count++;
	entry_cleanup_interval = config_map["entry_cleanup_interval"];
	if (entry_cleanup_interval == "") {
		LOG_WARN("VM Entry cleanup interval not found in vRTM.cfg. Using default value : %d", g_entry_cleanup_interval);
	}
	count++;
	/*delete_vm_max_age = config_map["delete_vm_max_age"];
	if (delete_vm_max_age == "") {
		LOG_WARN("Deleted VM cleanup interval not found in vRTM.cfg. Using default value : %d", g_delete_vm_max_age);
	}
	count++;*/
	cancelled_vm_max_age = config_map["cancelled_vm_max_age"];
	if (cancelled_vm_max_age == "") {
		LOG_WARN("Cancelled VM cleanup interval not found in vRTM.cfg. Using default value : %d", g_cancelled_vm_max_age);
	}
	count++;
	/*stopped_vm_max_age = config_map["stopped_vm_max_age"];
	if (stopped_vm_max_age == "") {
		LOG_WARN("Stopped VM cleanup interval not found in vRTM.cfg. Using default value : %d", g_stopped_vm_max_age);
	}
	count++;*/
	strcpy(g_vrtmcore_ip,vrtmcore_ip.c_str());
	LOG_DEBUG("vRTM IP : %s", g_vrtmcore_ip);
	g_vrtmcore_port = atoi(vrtmcore_port.c_str());
	LOG_DEBUG("vRTM listening port : %d", g_vrtmcore_port);
	g_max_thread_limit = atoi(max_thread_limit.c_str());
	LOG_DEBUG("vRTM Max concurrent request processing limit : %d", g_max_thread_limit);
	strcpy(g_vrtm_root, vrtm_root.c_str());
	strcat(g_vrtm_root, "/");
	LOG_DEBUG("vRTM root : %s", g_vrtm_root);
	strcpy(g_trust_report_dir, trust_report_dir.c_str());
	strcat(g_trust_report_dir, "/");
	LOG_DEBUG("vRTM trust report directory : %s", g_trust_report_dir);
	g_entry_cleanup_interval = atoi(entry_cleanup_interval.c_str());
	LOG_DEBUG("VM Entry cleanup interval : %d", g_entry_cleanup_interval);
	//g_delete_vm_max_age = atoi(delete_vm_max_age.c_str());
	//LOG_DEBUG("Deleted VM cleanup interval : %d", g_delete_vm_max_age);
	g_cancelled_vm_max_age = atoi(cancelled_vm_max_age.c_str());
	LOG_DEBUG("Cancelled VM cleanup interval : %d", g_cancelled_vm_max_age);
	//g_stopped_vm_max_age = atoi(stopped_vm_max_age.c_str());
	//LOG_DEBUG("Stopped VM cleanup interval : %d", g_stopped_vm_max_age);

	// clear all data in trust report directory if directory already exists
	if( stat( g_trust_report_dir, &info ) != 0 ) {
	    LOG_DEBUG( "cannot access %s\n", g_trust_report_dir );
	    LOG_DEBUG( "New trust report directory %s will be created", g_trust_report_dir);
	    create_report_dir = true;
	}
	else if( info.st_mode & S_IFDIR ){ // S_ISDIR() doesn't exist on my windows
		LOG_DEBUG( "%s is a directory and already exists", g_trust_report_dir );
		LOG_INFO("%s will be cleaned", g_trust_report_dir);
		char command[512];
		sprintf(command,"rm -rf %s/*", g_trust_report_dir);
		if (system(command) == 0 ) {
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
	if ( create_report_dir) {
		ret_val = mkdir(g_trust_report_dir, 0766);
		if (ret_val == 0 ) {
			LOG_DEBUG("Trust report directory: %s created successfully", g_trust_report_dir);
		}
		else if (ret_val < 0 && errno == EEXIST ) {
			LOG_DEBUG("Trust report directory: %s already exist", g_trust_report_dir);
		}
		else {
			LOG_ERROR("Failed to create the Trust report directory", g_trust_report_dir);
			return -1;
		}
	}
	// create mount location for vRTM at /mnt/vrtm
	ret_val = mkdir(g_mount_path, 0766);
	if (ret_val == 0 ) {
		LOG_DEBUG("directory: %s to mount VM images for vRTM created successfully", g_mount_path);
	}
	else if (ret_val < 0 && errno == EEXIST ) {
		LOG_DEBUG("directory: %s to mount VM images for vRTM already exist", g_mount_path);
	}
	else {
		LOG_ERROR("Failed to create the directory: %s required for vRTM to mount VM images", g_mount_path);
		return -1;
	}
	return count;
}

int singleInstanceRpcoreservice()
{
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

    return 1;
}

void vrtm_signal_handler(int sig_caught) {
	std::string signal_name;
	switch(sig_caught) {
	case SIGABRT:
		signal_name = "SIGABRT";
		signal(sig_caught, *default_handler_abrt);
		break;
	case SIGHUP:
		signal_name = "SIGHUP";
		signal(sig_caught, *default_handler_hup);
		break;
	case SIGINT:
		signal_name = "SIGINT";
		signal(sig_caught, *default_handler_int);
		break;
	case SIGQUIT:
		signal_name = "SIGQUIT";
		signal(sig_caught, *default_handler_quit);
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
	case SIGBUS:
		signal_name = "SIGBUS";
		signal(sig_caught, *default_handler_bus);
		break;
	}
	LOG_INFO("caught signal %s", signal_name.c_str());
	// clean the trust report directory

	std::string command = "rm -rf " + std::string(g_trust_report_dir) + "*";
	int ret_val = system(command.c_str());
	if (ret_val != 0) {
		LOG_ERROR("Failed to remove the trust report directory : %s", command.c_str());
	}
	else {
		LOG_DEBUG("trust report directory removed successfully");
	}
	LOG_INFO("vRTM will Exit with recieved signal %d", sig_caught);
	raise(sig_caught);
}


int main(int an, char** av)
{
    int            	iRet= 0;
    int 			instanceValid = 0;
    
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
    default_handler_hup = signal(SIGHUP, vrtm_signal_handler);  //handle shell session termination
    default_handler_int = signal(SIGINT, vrtm_signal_handler);  //handle signal interrupt ctrl-c
    //signal(SIGKILL, vrtm_signal_handler); //handle kill -9 but can't be handled or ignored or blocked
    default_handler_quit = signal(SIGQUIT, vrtm_signal_handler); //handle quit command given by ctrl-\ ---
    default_handler_segv = signal(SIGSEGV, vrtm_signal_handler); //handle segmentation fault
    default_handler_term = signal(SIGTERM, vrtm_signal_handler); //handle the kill command
    //signal(SIGSTOP, vrtm_signal_handler); //handle suspend signals ctrl-s -- can't be handled or ignored or blocked
    default_handler_fpe = signal(SIGFPE, vrtm_signal_handler);  //to handle divide by zero errors
    default_handler_bus = signal(SIGBUS, vrtm_signal_handler);  //handle invalid pointer dereferencing different from sigsegv
    //default_handler_tstp = signal(SIGTSTP, vrtm_signal_handler); //to handle interactive stop signal ctrl-z and ctrl-y, don't need to clean, process can be resumed again

    LOG_TRACE("Load config file %s", g_config_file);
	if ( LoadConfig(g_config_file) < 0 ) {
		LOG_ERROR("Can't load config file %s", g_config_file);
		goto cleanup;
	}

    LOG_TRACE("Read configurations");
	if ( read_config() < 0)
	{
		LOG_ERROR("tcService main : cant't find required values in config file");
		goto cleanup;
	}

    LOG_TRACE("Starting vRTM interface");
	if(!start_vrtm_interface(NULL)) {
		LOG_ERROR("Can't initialize vRTM interface");
		goto cleanup;
	}

	return iRet;

cleanup:
	  closeLog();
	  return iRet;
}


