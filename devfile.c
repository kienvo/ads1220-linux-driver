
#include "devfile.h"

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
	uint8_t gpio_state = 1;//gpio_get_value(GPIO12);
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

	return 0;


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
}