#include "kvm_standalone.h"
#include "sys/mount.h"
#include <linux/fs.h>
#define TCSERVICESTARTAPPFROMAPP    15
int g_rpchan_fd = -1;

int channel_open() {
    int fd = -1;

	fd =  ch_open(NULL, 0);


	if(fd < 0)
	{
        	fprintf(stdout, "Cannot connect: %s\n", strerror(errno));
        	return -1;
    }
    
  	ch_register(fd);  
    return fd;

}


int get_rpcore_decision(char* kernel_file, char* ramdisk_file) {
	
	char buf[1024*4] = {0};
	char kernel[1024] = {0};
	char ramdisk[1024] = {0};
	char config[1024] = {0};

	int err = -1;
	int i = 0;
	int retval;
	
	int         size= PARAMSIZE;
	byte        rgBuf[PARAMSIZE];
    tcBuffer*   pReq= (tcBuffer*) rgBuf;
	int         an = 0;
	char*      av[20] = {0};
	
	char  *ip = NULL, *port =NULL;


	setenv("PYTHONPATH", "../../scripts", 1);
	if (init_pyifc("rppy_ifc") < 0 )
		return -1;
	

	av[an++] = "./vmtest";
	av[an++] = "-kernel";
	av[an++] = kernel_file;
	av[an++] = "-ramdisk";
	av[an++] = ramdisk_file;
	av[an++] = "-config";
	av[an++] = "config";

 
	fprintf(stdout, "Opening channel .. \n");

    if (g_rpchan_fd <= 0)
        g_rpchan_fd = channel_open();

	if(g_rpchan_fd < 0)
	{
        	fprintf(stdout, "Open error chandu: %s\n", strerror(errno));
        	return -1;
    }

	
	size = sizeof(tcBuffer);

	size= encodeVM2RP_STARTAPP("foo", an, av, PARAMSIZE - size, &rgBuf[size]);

	pReq->m_procid= 0;
	pReq->m_reqID= TCSERVICESTARTAPPFROMAPP;
	pReq->m_ustatus= 0;
	pReq->m_origprocid= 0;
	pReq->m_reqSize= size;

	fprintf(stdout, "Sending request size %d \n", size + sizeof(tcBuffer));
	
	err = ch_write(g_rpchan_fd, rgBuf, size + sizeof(tcBuffer) );
	if (err < 0){
		fprintf(stdout, "write error: %s\n", strerror(errno));
		goto fail;
	}

	memset(rgBuf, 0, sizeof(rgBuf));
	fprintf(stdout, "Sent request ..........\n");
	
again:
	err = ch_read(g_rpchan_fd, rgBuf, sizeof(rgBuf));
	if (err < 0){
		if (errno == 11) {
			sleep(1);
			goto again;
		}
		fprintf(stdout, "read error:%d  %s\n", errno, strerror(errno));
		goto fail;
	}
	fprintf(stdout, "Response  from server status %d return %d\n", pReq->m_ustatus, *(int*) &rgBuf[sizeof(tcBuffer)]);
	retval = *(int*) &rgBuf[sizeof(tcBuffer)];
	return retval;

fail:
	if ( g_rpchan_fd >= 0) {
		ch_close (g_rpchan_fd);
		g_rpchan_fd = 0;
	}
	
	deinit_pyifc();	
	return 0;
}

int qemu_nbd_attach(char* device, char* image) {
    char sz_qemu_nbd_attach[1024];
    sprintf(sz_qemu_nbd_attach, "qemu-nbd -c %s %s", device, image);
    return system(sz_qemu_nbd_attach);
}

int mount_image(char* device, char* target_directory) {
    char sz_mount_command[1024];
    sprintf(sz_mount_command, "mount %s %s", device, target_directory);
    return system(sz_mount_command);
}

int qcow_umount(char* directory) {
    return umount(directory);
} 

int qemu_nbd_detach(char* device) {
    char sz_qemu_nbd_detach[1024];
    sprintf(sz_qemu_nbd_detach, "qemu-nbd -d %s", device);
    printf("Detaching device %s\n", device);
    return system(sz_qemu_nbd_detach);
}

int main ( int argc, char** argv) 
{
	char sz_rpid[64]  = {'\0'},
         sz_kernel[1024] = {'\0'},
         sz_initrd[1024] = {'\0'},
         sz_template[32] = {'\0'},
         qemu_cmdline[1024] = {'\0'},
         sz_bashrc_filename[128] = {'\0'};
    static const int DATA_LENGTH = 8*1024;
    int secret_in_length = 0;
    
    int rpid;
    
	char sz_buf[DATA_LENGTH] = {0};
	
    sz_kernel[1023]='\0';

    sprintf (sz_template, "/tmp/qcowXXXXXX");
    if (!mkdtemp(sz_template)) return -1;
    
    qemu_nbd_attach("/dev/nbd0", "/root/ubuntu-12.04.img");
    printf("%d\n", mount_image("/dev/nbd0p1", sz_template));

    if (argc < 3) {
        sprintf (sz_kernel, "%s%s",sz_template,"/boot/vmlinuz-3.5.0-23-generic");
        sprintf (sz_initrd, "%s%s",sz_template,"/boot/initrd.img-3.5.0-23-generic");
    }
    else {
        sprintf (sz_kernel, "%s%s",sz_template,argv[1]);
        sprintf (sz_initrd, "%s%s",sz_template,argv[2]);
    }
    rpid = get_rpcore_decision(sz_kernel,sz_initrd);

    // pass rpid to vm (by adding line to bashrc)
    sprintf (sz_rpid, "%d",rpid);
    printf("rpid is %s\n", sz_rpid );
    sprintf (sz_bashrc_filename, "%s/etc/bash.bashrc",sz_template);

    FILE *fd;
    if ((fd = fopen(sz_bashrc_filename,"a"))==NULL) {
        printf("failed to open VM internal file\n");
        return -1;
    }

    fprintf(fd, "\nexport THIS_RPID=%s\n", sz_rpid);
    fclose(fd);

    if (rpid <= 999) {
        printf ("Something went wrong.\n");
        return -1;
    }
    printf("rpid is: %s\n", sz_rpid);
    qcow_umount(sz_template);
    qemu_nbd_detach("/dev/nbd0");
   
    if ( g_rpchan_fd >= 0) {
		ch_close (g_rpchan_fd);
		g_rpchan_fd = 0;
	}
    sprintf (qemu_cmdline, "qemu-system-x86_64 -m 2048 -net nic,model=e1000 -net user -enable-kvm ~/ubuntu-12.04.img");
    printf(qemu_cmdline);
    return system (qemu_cmdline);
    return 0;
}	
