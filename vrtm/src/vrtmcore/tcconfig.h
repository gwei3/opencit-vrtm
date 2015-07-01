#ifndef _TCCONFIGH_
#define _TCCONFIGH_

int LoadConfig(const char* szInFile );


extern char g_rpcore_ip []; 
extern int  g_rpcore_port ;
extern int  g_max_thread_limit ;
extern char 	g_trust_report_dir[];

extern int  g_quit;

#define RP_MODE_OPENSTACK 	0
#define RP_MODE_STANDALONE 	1

#endif
