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

#include <linux/kdev_t.h>
#include <linux/cdev.h>

#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>


#define GPIO12 12

int param1;
int cb_param = 0;

dev_t dev=0;
static struct  class *dev_class;
static struct cdev etx_dev;

static int etx_open(struct inode *inode, struct file * file)
{
	pr_info("Device file opened.\n");
	return 0;
}

static int etx_release(struct inode *inode, struct file * file)
{
	pr_info("Device file closed.\n");
	return 0;
}

static ssize_t 
etx_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{
	uint8_t gpio_state = gpio_get_value(GPIO12);
	len =1;
	if (copy_to_user(buf, &gpio_state, len)>0) {
		pr_err("ERROR: Copy to user failed.\n");
	}
	pr_info("Read GPIO12 = %d\n", gpio_state);
	return 0;

}

static ssize_t 
etx_write(struct file *fp, const char __user *buf, size_t len, loff_t *off)
{
	uint8_t kbuf[10];
	if (copy_from_user(kbuf, buf, len) >0) {
		pr_err("ERROR: copy_from_user failed.\n");
	}
	pr_info("Write GPIO12 = %c\n", kbuf[0]);
	if(kbuf[0]=='1') 
		gpio_set_value(GPIO12, 1);
	else if (kbuf[0]=='0')
		gpio_set_value(GPIO12, 0);
	else 
		pr_err("Unknown command.\n");
	return len;
}



static struct file_operations fops =
{
	.owner	= THIS_MODULE,
	.read 	= etx_read,
	.write 	= etx_write,
	.open 	= etx_open,
	.release = etx_release,
};


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
	printk(KERN_INFO "ads1220: Module loaded\n");

	if (alloc_chrdev_region(&dev, 0, 1, "ads1220") < 0) {
		pr_err("Cannot allocate major number\n");
		goto r_unreg;
	}
	pr_info("Major = %d minor = %d\n", MAJOR(dev), MINOR(dev));
	cdev_init(&etx_dev, &fops);

	if (cdev_add(&etx_dev, dev, 1) < 0) {
		pr_err("cdev_add failed.\n");
		goto r_del;
	}

	if((dev_class = class_create(THIS_MODULE, "ads_class")) == NULL) {
		pr_err("class_create faled.\n");
		goto r_class;
	}

	if(device_create(dev_class, NULL, dev, NULL, "ads1220")==NULL) {
		pr_err("Cannot create the Device\n");
		goto r_device;
	}

	if(gpio_is_valid(GPIO12)==false) {
		pr_err("GPIO %d is not valid.\n", GPIO12);
		goto r_device;
	}

	if(gpio_request(GPIO12, "GPIO_12") <0 ) {
		pr_err("ERROR: GPIO %d request.\n", GPIO12);
		goto r_gpio;
	}

	gpio_direction_output(GPIO12, 0);

	gpio_export(GPIO12, false);
	pr_info("Device driver inserted successfully\n");

	return 0;

r_gpio:
	gpio_free(GPIO12);
r_device:
	device_destroy(dev_class, dev);
r_class:
	class_destroy(dev_class);
r_del:
	cdev_del(&etx_dev);
r_unreg:
	unregister_chrdev_region(dev, 1);

	return 1;
}

static void __exit hello_exit(void) 
{
	gpio_unexport(GPIO12);
	gpio_free(GPIO12);
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&etx_dev);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "ads1220: Module removed successfully\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kienlab.com");
MODULE_DESCRIPTION("abcefgh");
MODULE_VERSION("1:0.1");
