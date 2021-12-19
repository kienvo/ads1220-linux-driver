/**
 * @file		devfile.c
 * @author		github.com/kienvo (kienvo@kienlab.com)
 * @brief 		Device file interface
 * @version		0.1
 * @date		Dec-19-2021
 * 
 * @copyright	Copyright (c) 2021 by kienvo@kienlab.com
 * 
 */


#include "devfile.h"

dev_t dev=0;
static struct  class *dev_class;
static struct cdev etx_dev;
static int test=0;
static DECLARE_WAIT_QUEUE_HEAD(wq);
int gpio11_irqn;


static irqreturn_t gpio_irq_handler(int irq, void *dev_id) 
{
	int val = gpio_get_value(GPIO11);
	pr_info("IRQH: interrupted, GPIO11: %d\n", val);
	if(val==1) 
		test++;
	wake_up(&wq);
	return IRQ_HANDLED;
}

static int etx_open(struct inode *inode, struct file * file)
{
	enable_irq(gpio11_irqn);
	pr_info("Device file opened.\n");
	return 0;
}

static int etx_release(struct inode *inode, struct file * file)
{
	disable_irq(gpio11_irqn);
	pr_info("Device file closed.\n");
	return 0;
}


// TODO: Attemted read 4 byte, will be fixed after testing is done
static ssize_t 
etx_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{
	int32_t c = 55555;
	if (copy_to_user(buf, &c, len)>0) {
		pr_err("ERROR: Copy to user failed.\n");
	}
	wait_event_timeout(wq, test>=3, 10*HZ); // 10s wait
	if(test<3)
		return len;	// end of file
	else {
		test = 0;
		return 0;	
	}

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
		//gpio_set_value(GPIO12, 1);
		pr_info("device set to 1");
	else if (kbuf[0]=='0')
		pr_info("device set to 0");
		// gpio_set_value(GPIO12, 0);
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

int devfile_init(void) 
{
	if (alloc_chrdev_region(&dev, 0, 1, "ads1220cdev") < 0) {
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
	

	if(gpio_is_valid(GPIO11)==false) {
		pr_err("ERROR: GPIO%d is not valid.\n", GPIO11);
		goto r_gpio11;
	}
	if(gpio_request(GPIO11, "GPIO11_IN") < 0) {
		pr_err("ERROR: GPIO%d request failed.\n", GPIO11);
		goto r_gpio11;
	}

	gpio_direction_input(GPIO11);

	gpio11_irqn = gpio_to_irq(GPIO11);
	pr_info("gpio11 irq number: %d\n", gpio11_irqn);

	if(request_irq(
		gpio11_irqn, (void *)gpio_irq_handler, 
		IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "ads1220", NULL)) {
		pr_err("cannot register IRQ\n");
		goto r_gpio11_irq;
	}
	disable_irq(gpio11_irqn);

	return 0;


r_gpio11_irq:
	free_irq(gpio11_irqn, NULL);
r_gpio11:
	gpio_free(GPIO11);
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

void devfile_exit(void)
{
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&etx_dev);
	unregister_chrdev_region(dev, 1);

	free_irq(gpio11_irqn, NULL);
	gpio_unexport(GPIO11);
	gpio_free(GPIO11);
}