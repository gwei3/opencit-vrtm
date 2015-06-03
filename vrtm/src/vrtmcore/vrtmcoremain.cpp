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

#include "tcIO.h"
#include "logging.h"
#include "log_vrtmchannel.h"
#include "tcconfig.h"
//#include "jlmcrypto.h"

#define    g_config_file "../configuration/vRTM.cfg"
#define	   log_properties_file "../configuration/vRTMlog.properties"

char    g_python_scripts[256] = "../scripts";
//char    g_config_file[256]      = "vRTM.cfg";
char    g_rpcore_ip [64]        = "127.0.0.1";
int     g_rpcore_port 		= 16005;
int     g_max_thread_limit 	= 64;

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
	LOG_TRACE("");
	if(fp == NULL)
	{
		LOG_ERROR("Failed to load vRTM config file");
		return -1;
	}
	while(true)
	{
		line = (char *) calloc(1,sizeof(char) * 512);
		fgets(line,line_size,fp);
		if(feof(fp))
			break;
		key=strtok(line,"=");
		value=strtok(NULL,"=");
		std::string map_key (key);
		std::string map_value (value);
		std::pair<std::string, std::string> config_pair (trim_copy(map_key," \t\n"),trim_copy(map_value," \t\n"));
		config_map.insert(config_pair);
		free(line);
	}
	return config_map.size();
}

int read_config()
{
	int count=0;
	std::string rpcore_ip, rpcore_port, max_thread_limit;
	LOG_TRACE("");
	rpcore_ip = config_map["rpcore_ip"];
	if(rpcore_ip == ""){
		rpcore_ip = "127.0.0.1";
		LOG_WARN("vRTM IP is not found in vRTM.cfg. Using default IP %s", rpcore_ip.c_str());
	}
	count++;
	rpcore_port = config_map["rpcore_port"];
	if(rpcore_port == ""){
		rpcore_port = "16005";
		LOG_WARN("vRTM Port No. is not found in vRTM.cfg. Using default Port %s", rpcore_port.c_str());
	}
	count++;
	max_thread_limit = config_map["max_thread_limit"];
	if(max_thread_limit == ""){
		max_thread_limit = "63";
		LOG_WARN("Thread Limit for vRTM is not found in vRTM.cfg. Using default limit %s", max_thread_limit.c_str());
	}
	count++;
	strcpy(g_rpcore_ip,rpcore_ip.c_str());
	//sprintf(g_rpcore_port,"%d", rpcore_port);
	//sprintf(g_max_thread_limit,"%d",max_thread_limit);
	g_rpcore_port = atoi(rpcore_port.c_str());
	g_max_thread_limit = atoi(max_thread_limit.c_str());
	return count;
}

int singleInstanceRpcoreservice()
{
    //const char *pidPath="/var/run/rpcoreservice.pid";
    const char *lockFile="/tmp/rpcoreservice__DT_XX99";
    
    struct flock rpcsFlock;
    
    rpcsFlock.l_type = F_WRLCK;
    rpcsFlock.l_whence = SEEK_SET;
    rpcsFlock.l_start = 0;
    rpcsFlock.l_len = 1;

    if( ( g_fdLock = open(lockFile, O_WRONLY | O_CREAT, 0666)) == -1) 
    {
#ifdef TEST
    	LOG_ERROR("Can't open rpcoreservice lock file \n");
#endif
        return -1;
     }
     
     chmod(lockFile, 0666); // Just in case umask is not set properly.
     
     if ( fcntl(g_fdLock, F_SETLK, &rpcsFlock) == -1) {

#ifdef TEST
    	LOG_ERROR( "Already locked - rpcoreservice lock file \n");
#endif
		return -2;
     }

    return 1;

}
//test function declarations
void test_host_init();
int modmain(int an, char** av);

int main(int an, char** av)
{
    int            	iRet= 0;
    int 			instanceValid = 0;
    //const char*		configfile = "../../config/vRTM.cfg";
    //const char * 	log_properties_file = "../../config/vRTMlog.properties";
//------------------------------------------------------------------
    // Start the instance of logger, currently using log4cpp
    if( initLog(log_properties_file) ){
    	return 1;
    }
    //set same logger instance in rp_channel
    set_logger_vrtmchannel(rootLogger);

    if ((instanceValid = singleInstanceRpcoreservice()) == -2) {
    	LOG_ERROR("Process(rpcoreservice) already running\n");
		return 1;
    }
    
    if(instanceValid == -1) {
    	LOG_ERROR( "Process(rpcoreservice) could not open lock file\n");
		return 1;
    }
//------------------------------------------------------------------
	/*
	 * Start rpinterface
	 */

	//init linux service 

	if(!start_rp_interface(NULL)) {
		LOG_ERROR("cant init start_rp_interface\n");
		goto cleanup;
	}

	if ( LoadConfig(g_config_file) < 0 ) {
		LOG_ERROR("tcService main: can't load config file %s\n", g_config_file);
		goto cleanup;
	}
	if ( read_config() < 0)
	{
		LOG_ERROR("tcService main : cant't find required values in config file");
		goto cleanup;
	}
	
	return iRet;
//------------------------------------------------------------------

cleanup:
	  closeLog();
	  return iRet;
}


