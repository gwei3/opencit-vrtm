#ifndef _TCCONFIGH_
#define _TCCONFIGH_

#define RP_MODE_OPENSTACK 	0
#define RP_MODE_STANDALONE 	1
extern char g_vrtmcore_ip[];
extern int  g_vrtmcore_port ;
extern int  g_max_thread_limit ;
extern char g_vrtm_root[];
extern char 	g_trust_report_dir[];
extern long 	g_entry_cleanup_interval;
//extern long 	g_delete_vm_max_age;
extern long 	g_cancelled_vm_max_age;
//extern long	g_stopped_vm_max_age;
extern char     g_deployment_type[];

extern char*	g_mount_path;
extern int  g_quit;

#endif
