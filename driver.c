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
#include <linux/gpio.h>

#include <linux/interrupt.h>
#include <linux/spi/spi.h>

#include "ads1220.h"
#include "devfile.h"


#define GPIO12 12
#define GPIO11 11

int param1;
int cb_param = 0;
int gpio11_irqn;

static irqreturn_t gpio_irq_handler(int irq, void *dev_id) 
{
	pr_info("IRQH: interrupted, irq: %d\n", irq);
	return IRQ_HANDLED;
}

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

	if(gpio_is_valid(GPIO12)==false) {
		pr_err("GPIO %d is not valid.\n", GPIO12);
		goto r_devfile;
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

	ads1220_test();


	pr_info("Device driver inserted successfully\n");

	return 0;
r_gpio11:
	gpio_free(GPIO11);
r_gpio11_irq:
	free_irq(gpio11_irqn, NULL);
r_gpio:
	gpio_free(GPIO12);
r_devfile:
	devfile_exit();
r_ads1220:
	ads1220_exit();

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
	devfile_exit();
	printk(KERN_INFO "ads1220: Module removed successfully\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kienlab.com");
MODULE_DESCRIPTION("abcefgh");
MODULE_VERSION("1:0.1");
