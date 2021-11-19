/**
 * @file		main.c
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


static int __init hello_init(void) 
{
	printk(KERN_INFO "Printk from init module\n");
	printk(KERN_INFO "2nd line printk from init module\n");
	return 0;
}

static void __exit hello_exit(void) 
{
	printk(KERN_INFO "Kernel module removed successfully\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kienlab.com");
MODULE_DESCRIPTION("abcefgh");
MODULE_VERSION("1:0.1");
