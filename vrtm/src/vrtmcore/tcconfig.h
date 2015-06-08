#ifndef _TCCONFIGH_
#define _TCCONFIGH_

int LoadConfig(const char* szInFile );


extern char g_config_dir[];
extern char g_staging_dir[];
extern char g_kns_ip[];
extern int  g_kns_port ;

//extern char g_mtwproxy_ip[];
//extern int  g_mtwproxy_port ;

//extern int  g_mtwproxy_on;

extern char g_domain[];
extern int g_mode;
extern char g_rpcore_ip []; 
extern int  g_rpcore_port ;
extern int  g_max_thread_limit ;
extern char g_python_scripts[];

extern int  g_quit;

#define RP_MODE_OPENSTACK 	0
#define RP_MODE_STANDALONE 	1

#endif
