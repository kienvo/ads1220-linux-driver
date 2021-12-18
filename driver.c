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

#include <linux/interrupt.h>
#include <linux/spi/spi.h>


#define GPIO12 12
#define GPIO11 11

int param1;
int cb_param = 0;
int gpio11_irqn;

static struct spi_device *ads1220;

struct spi_board_info ads1220_info =
{
	.modalias	= "ads1220spi",
	.max_speed_hz = 20000,
	.bus_num	= 1,
	.chip_select = 1,
	.mode		= SPI_MODE_1
};

static int ads1220_spi_txByte(uint8_t data)
{
	int ret;
	uint8_t rx = 0x00;

	if(ads1220) {
		struct spi_transfer tr = {
			.tx_buf = &data,
			.rx_buf = &rx,
			.len	= 1,
		};
		ret = spi_sync_transfer(ads1220, &tr, 1);
	}
	pr_info("ads1220_spi_txByte: rx: 0x%02X\n", rx);
	return ret;
}


static void ads1220_spi_txBuf(uint8_t const *tx, uint8_t const *rx, size_t len) 
{

	if(ads1220) {
		struct spi_transfer tr = {
			.tx_buf = (void *)tx,
			.rx_buf = (void *)rx,
			.len	= len,
		};
		spi_sync_transfer(ads1220, &tr, 1);// TODO: return handle
	}
	
}

static void hex_dump(const void *src, size_t len, size_t line_size, 
	char *prefix) 
// TODO: implement line_size later
{
	const uint8_t *mem;
	if(!src) { // == NULL
		printk("__null__\n");
		return;
	}
	mem = src;

	printk("%s | ", prefix);
	while(len-- >0) {
		printk("%02X ", *mem++);
	}
	printk("\n");
}

static void test(void)
{
	uint8_t test_tx[] = {
		0b00010000, 0, 0, 0
	};
	uint8_t test_rx[5];
	ads1220_spi_txBuf(test_tx, test_rx, 4);
	hex_dump(test_rx, 5, 32, "RX");
}

static int ads1220_init(void)
{
	struct spi_master *master;
	master = spi_busnum_to_master(ads1220_info.bus_num);
	if(!master) {
		pr_err("SPI Master not found.\n");
		return -ENODEV;
	}
	ads1220 = spi_new_device(master, &ads1220_info);
	if(!ads1220) {
		pr_err("Failed to create slave.\n");
		return -ENODEV;
	}
	ads1220->bits_per_word = 8;
	if(spi_setup(ads1220)) {
		pr_err("Failed to setup slave.\n");
		spi_unregister_device(ads1220);
		return -ENODEV;
	}
	return 0;
}

static void ads1220_exit(void) 
{
	spi_unregister_device(ads1220);
}

static irqreturn_t gpio_irq_handler(int irq, void *dev_id) 
{
	pr_info("IRQH: interrupted, irq: %d\n", irq);
	return IRQ_HANDLED;
}

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
	int ret;
	printk(KERN_INFO "ads1220: Module loaded\n");

	ret = ads1220_init();
	if(ret) return ret;

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
		IRQF_TRIGGER_FALLING, "ads1220", NULL)) {
		pr_err("cannot register IRQ\n");
		goto r_gpio11_irq;
	}

	test();


	pr_info("Device driver inserted successfully\n");

	return 0;
r_gpio11:
	gpio_free(GPIO11);
r_gpio11_irq:
	free_irq(gpio11_irqn, NULL);
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
	ads1220_exit();
	free_irq(gpio11_irqn, NULL);
	gpio_unexport(GPIO12);
	gpio_free(GPIO12);
	gpio_unexport(GPIO11);
	gpio_free(GPIO11);
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
