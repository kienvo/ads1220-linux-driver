#ifndef __CDEV_H__
#define __CDEV_H__

#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

int devfile_init(void);
void devfile_exit(void);



#endif /* __CDEV_H__ */
