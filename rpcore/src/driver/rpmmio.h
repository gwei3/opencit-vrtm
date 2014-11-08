

/*
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


// --------------------------------------------------------------------


#ifndef _MMIO_H_
#define _MMIO_H_

#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#define MMIO_MAJOR 100
#ifndef MMIO_MAJOR
#define MMIO_MAJOR 0           // dynamic major by default
#endif

#ifndef MMIO_NR_DEVS
#define MMIO_NR_DEVS 1         // rpmmio0 only
#endif

#ifndef byte
typedef unsigned char byte;
#endif


#define TYPE(minor)     (((minor) >> 4) & 0xf)
#define NUM(minor)      ((minor) & 0xf)


#define JIFTIMEOUT 100


extern int      rpmmio_major;     
extern int      rpmmio_nr_devs;

ssize_t         rpmmio_read(struct file *filp, char __user *buf, size_t count,
                    loff_t *f_pos);
ssize_t         rpmmio_write(struct file *filp, const char __user *buf, size_t count,
                     loff_t *f_pos);
extern int      rpmmio_ioctl(struct inode *inode, struct file *filp,
                     unsigned cmd, unsigned long arg);
#endif


// --------------------------------------------------------------------


