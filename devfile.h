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
#include <linux/workqueue.h>

#include <linux/jiffies.h>
#include <linux/irqflags.h>

#include "ads1220.h"


#define GPIO11 11
#define GPIO12 12

int devfile_init(void);
void devfile_exit(void);


#endif /* __CDEV_H__ */
