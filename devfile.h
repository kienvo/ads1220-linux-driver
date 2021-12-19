#ifndef __CDEV_H__
#define __CDEV_H__

#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/wait.h>


#define GPIO11 11

int devfile_init(void);
void devfile_exit(void);



#endif /* __CDEV_H__ */
