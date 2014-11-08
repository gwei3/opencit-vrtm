//
//  File: rpmmio.c
//      Trusted service device driver
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


#include <linux/module.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/device.h>
#include "rpmmio.h"





/*
 * Some code from this file was derived from material developed by
 *     Alessandro Rubini and Jonathan Corbet and subject to the
 *     following terms.
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 */

#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
/* #include <asm/system.h> */
#include <asm/uaccess.h>




extern ssize_t  rpmmio_read(struct file *filp, char __user *buf, size_t count,
                            loff_t *f_pos);
extern ssize_t  rpmmio_write(struct file *filp, const char __user *buf, size_t count,
                             loff_t *f_pos);
extern int      rpmmio_open(struct inode *inode, struct file *filp);
extern int      rpmmio_close(struct inode *inode, struct file *filp);


// ----------------------------------------------------------------------------


unsigned    rpmmio_serviceInitilized= 0;
int         rpmmio_servicepid= 0;


int         rpmmio_major=   MMIO_MAJOR;
int         rpmmio_minor=   0;
int         rpmmio_nr_devs= MMIO_NR_DEVS;


module_param(rpmmio_major, int, S_IRUGO);
module_param(rpmmio_minor, int, S_IRUGO);
module_param(rpmmio_nr_devs, int, S_IRUGO);


struct rpmmio_dev*  rpmmio_devices;

struct file_operations rpmmio_fops= {
    .owner=    THIS_MODULE,
    .read=     rpmmio_read,
    .write=    rpmmio_write,
    .open=     rpmmio_open,
    .release=  rpmmio_close,
};



//
//  For udev and dynamic allocation of device
static struct class*    pclass= NULL;
static struct device*   pdevice= NULL;



// ----------------------------------------------------------------------------




// ------------------------------------------------------------------------------


int rpmmio_open(struct inode *inode, struct file *filp)
{
 

    return 0;
}


int rpmmio_close(struct inode *inode, struct file *filp)
{
    int                 pid= current->pid;
    struct rpmmio_Qent* pent= NULL;

    return 1;
}


ssize_t rpmmio_read(struct file *filp, char __user *buf, size_t count,
                    loff_t *f_pos)
{
    struct rpmmio_dev*  dev= filp->private_data; 
    ssize_t             retval= 0;

    return retval;
}


ssize_t rpmmio_write(struct file *filp, const char __user *buf, size_t count,
                     loff_t *f_pos)
{
	ssize_t             retval= 0;
    return retval;
}


// --------------------------------------------------------------------


// The cleanup function must handle initialization failures.
void rpmmio_cleanup(void)
{
    int         i;
    dev_t       devno= MKDEV(rpmmio_major, rpmmio_minor);

}



int rpmmio_init(void)
{
    int     result, i;
    dev_t   dev= 0;

    if(rpmmio_major) {
        // static registration
        dev= MKDEV(rpmmio_major, rpmmio_minor);
        result= register_chrdev(dev, "rpmmio", &rpmmio_fops);
    } 
    else {
        // dynamic registration
        result= alloc_chrdev_region(&dev, rpmmio_minor, rpmmio_nr_devs, "rpmmio");
        rpmmio_major= MAJOR(dev);
    }
    if(result<0) {
        printk(KERN_WARNING "rpmmio: can't get major %d\n", rpmmio_major);
        return result;
    }

    pclass= class_create(THIS_MODULE, "rpmmio");
    if(pclass==NULL)
        goto fail;
    pdevice= device_create(pclass, NULL, MKDEV(rpmmio_major,0), NULL, "rpmmio0");
    if(pdevice==NULL)
        goto fail;

  
 
#ifdef TESTDEVICE
    printk(KERN_DEBUG "rpmmio: rpmmio_init complete\n");
#endif
    return 0;

fail:
    rpmmio_cleanup();
    return result;
}


MODULE_LICENSE("Dual BSD/GPL");
module_init(rpmmio_init);
module_exit(rpmmio_cleanup);


// ------------------------------------------------------------------------------


