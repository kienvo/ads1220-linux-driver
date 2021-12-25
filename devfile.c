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

static dev_t dev=0;
static struct  class *dev_class;
static struct cdev etx_dev;
static int is_read_comlete=0;
static DECLARE_WAIT_QUEUE_HEAD(wq);
static int gpio11_irqn, gpio12_irqn;
static int32_t data;
static bool is_opening, gpio12_irq_enabled, gpio11_irq_enabled;

static void gpio12_irq_disable(void) {
	if (gpio12_irq_enabled) {				
		disable_hardirq(gpio12_irqn);	
		gpio12_irq_enabled = false;			
	}									
}

static void gpio12_irq_enable(void) {
	if (!gpio12_irq_enabled) {				
		enable_irq(gpio12_irqn);	
		gpio12_irq_enabled = true;			
	}									
}

static void gpio11_irq_disable(void) {
	if (gpio11_irq_enabled) {				
		disable_hardirq(gpio11_irqn);	
		gpio11_irq_enabled = false;			
	}									
}

static void gpio11_irq_enable(void) {
	if (!gpio11_irq_enabled) {				
		enable_irq(gpio11_irqn);	
		gpio11_irq_enabled = true;			
	}									
}

static void gpio12_work(struct work_struct *work);

DECLARE_WORK(workq, gpio12_work);


static irqreturn_t gpio11_irq_handler(int irq, void *dev_id) 
{
	// static unsigned long flags = 0;
	static unsigned long old_jifies=0;
	unsigned long diff =  jiffies - old_jifies;
	int val=0;

	if (is_opening) return IRQ_HANDLED;

	if(diff < HZ/10) { // 1/5 second
		// pr_notice("ads1220: Interrupt ignored!\n");
		return IRQ_HANDLED;
	}
	
	old_jifies = jiffies;

	val = gpio_get_value(GPIO11);
	// local_irq_save(flags);
	if(val==0) { // Button pressed
		gpio12_irq_enable();
	} else {
		is_read_comlete=2;
		wake_up_interruptible(&wq); // Occur an end of file
	}
	pr_notice("ads1220: Pressed! %d\n", val);

	// local_irq_restore(flags);
	return IRQ_HANDLED;
}
static irqreturn_t gpio12_irq_handler(int irq, void *dev_id)
{
	// pr_info("gpio12: interrupt\n");
	schedule_work(&workq);
	return IRQ_HANDLED;
}	

static void gpio12_work(struct work_struct *work)	
{
	data = ads1220_cc_getsample();
	is_read_comlete=1;
	wake_up_interruptible(&wq);
}

// TODO: check if device is connected
static int etx_open(struct inode *inode, struct file * file)
{
	is_opening = true;
	ads1220_deconfig();

	gpio11_irq_enable();
	
	
	is_opening = false;
	is_read_comlete=0;
	pr_info("Device file opened.\n");
	return 0;
}

static int etx_release(struct inode *inode, struct file * file)
{
	gpio11_irq_disable();
	gpio12_irq_disable();
	pr_info("Device file closed.\n");
	return 0;
}


// TODO: Assume read 4 byte, will be fixed after testing is done
static ssize_t 
etx_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{
	int ret;
	while(1){
		ret = wait_event_interruptible_timeout(wq, is_read_comlete, HZ);
		/* If the ads1220 is not respond return -1 */
		if(ret == 0 && gpio12_irq_enabled) return -1;
		if(ret == 0) continue;
		break;
	}
	
	if(is_read_comlete == 1) {
		is_read_comlete = 0;
		if (copy_to_user(buf, &data, len)>0) {
			pr_err("ERROR: Copy to user failed.\n");
		}
		return len;	
	}
	if(is_read_comlete == 2) {

		is_read_comlete = 0;
		return 0;	// end of file
	}
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
		pr_info("device set to 1\n");
	else if (kbuf[0]=='0')
		pr_info("device set to 0\n");
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
	
	/* GPIO11 */
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

	if(request_irq(
		gpio11_irqn, (void *)gpio11_irq_handler, 
		IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "ads1220_gpio11", NULL)) {
		pr_err("cannot register IRQ\n");
		goto r_gpio11_irq;
	}
	disable_hardirq(gpio11_irqn);

	/* GPIO12 */
	if(gpio_is_valid(GPIO12)==false) {
		pr_err("ERROR: GPIO%d is not valid.\n", GPIO12);
		goto r_gpio12;
	}
	if(gpio_request(GPIO12, "GPIO11_IN") < 0) {
		pr_err("ERROR: GPIO%d request failed.\n", GPIO12);
		goto r_gpio12;
	}
	gpio_direction_input(GPIO12);
	gpio12_irqn = gpio_to_irq(GPIO12);

	if (request_irq(
		gpio12_irqn, (void *)gpio12_irq_handler,
		IRQF_TRIGGER_FALLING , "ads1220_gpio12", NULL)
		) {
		pr_err("cannot register IRQ\n");
		goto r_gpio12_irq;
	}
	disable_hardirq(gpio12_irqn);


	return 0;


r_gpio12_irq:
	free_irq(gpio12_irqn, NULL);
r_gpio12:
	gpio_free(GPIO12);
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

	free_irq(gpio12_irqn, NULL);
	gpio_unexport(GPIO12);
	gpio_free(GPIO12);
}