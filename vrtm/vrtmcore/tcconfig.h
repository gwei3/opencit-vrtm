#ifndef _TCCONFIGH_
#define _TCCONFIGH_

#define RP_MODE_OPENSTACK 	0
#define RP_MODE_STANDALONE 	1

extern char g_rpcore_ip []; 
extern int  g_rpcore_port ;
extern int  g_max_thread_limit ;
extern char 	g_trust_report_dir[];
extern char*	g_mount_path;
extern long 	g_entry_cleanup_interval;
//extern long 	g_delete_vm_max_age;
extern long 	g_cancelled_vm_max_age;
//extern long		g_stopped_vm_max_age;

extern int  g_quit;

#endif
