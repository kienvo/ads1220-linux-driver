/**
 * @file		ads1220.c
 * @author		github.com/kienvo (kienvo@kienlab.com)
 * @brief 		Linux_spi with ADS1220
 * @version		0.1
 * @date		Oct-30-2021
 * 
 * @copyright	Copyright (c) 2021 by kienvo@kienlab.com
 * 
 */


#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/module.h>
#include <linux/moduleparam.h>


#include <linux/device.h>
#include <linux/delay.h>

#include <linux/spi/spi.h>

#include "ads1220.h"
#include "devfile.h"



int param1;
int cb_param = 0;

module_param(param1, int, S_IRUSR| S_IWUSR);

int notify_param(const char *val, const struct kernel_param *kp)
{
	int r = param_set_int(val, kp);
	if(r == 0) {
		printk(KERN_INFO "Call back funtion called..\n");
		printk(KERN_INFO "New value of param1 = %d", cb_param);
		return 0;
	}
	return -1;
}



const struct kernel_param_ops cb_param_ops = 
{
	.set = &notify_param,
	.get = &param_get_int,
};

module_param_cb(cb_param, &cb_param_ops, &cb_param, S_IRUGO|S_IWUSR);

static int __init hello_init(void) 
{
	int ret;
	printk(KERN_INFO "ads1220: Module loaded\n");

	ret = ads1220_init();
	if(ret) return ret;

	if(devfile_init()) goto r_ads1220; 	// Error


	ads1220_test();


	pr_info("Device driver inserted successfully\n");

	return 0;

r_devfile:
	devfile_exit();
r_ads1220:
	ads1220_exit();

	return 1;
}

static void __exit hello_exit(void) 
{
	ads1220_exit();
	devfile_exit();
	printk(KERN_INFO "ads1220: Module removed successfully\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kienlab.com");
MODULE_DESCRIPTION("abcefgh");
MODULE_VERSION("1:0.1");
