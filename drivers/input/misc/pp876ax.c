/* drivers/input/misc/pp876ax.c
 * 
 * Copyright (C) 2011 PointChips, inc.
 * 
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 * 
 * This program is distributed in the hope that is will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABLILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Genernal Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>

#include <mach/map.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-b.h>

#include <linux/i2c-pb206x-platform.h>

#define IRQ	S3C_IRQ_GPIO(S5PC1XX_GPB(1)) 

struct pp876ax_device {
	struct i2c_client *client;
	int irq;
	/* TODO */
};


/* TODO */
static irqreturn_t pp876ax_i2c_isr(int this_irq, void *dev_id)
{
	struct pp876ax_device *dev;

	return IRQ_HANDLED;
}

static void gpio_configure(void)
{
	int ret;

	ret = gpio_request(S5PC1XX_GPB(1), "GPB1");
	if (ret) {
		printk("%s: gpio(GPB(1) request error: %d\n", __func__, ret);
	}
	s3c_gpio_cfgpin(S5PC1XX_GPB(1), S5PC1XX_GPB1_GPIO_INT2_1);
	s3c_gpio_setpull(S5PC1XX_GPB(1), S3C_GPIO_PULL_NONE);
}

static int __devinit pp876ax_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct pp876ax_device *dev;
	int ret;

	if (!i2c_check_functionality(client->adapter,
		I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA |
		I2C_FUNC_SMBUS_I2C_BLOCK)) {
		printk(KERN_ERR "%s: needed i2c functionality is not supported\n", __func__);
		return -ENODEV;
	}
	
	dev = kzalloc(sizeof(struct pp876ax_device), GFP_KERNEL);
	if (dev == NULL) {
		printk(KERN_ERR "%s: no memory\n", __func__);
		return -ENOMEM;
	}

	dev->client = client;
	i2c_set_clientdata(client, dev);

	// pdata = client->dev.platform_data;
	
	/* TODO: do something */
	gpio_configure();
	dev->irq = IRQ;

	ret = request_irq(dev->irq, pp876ax_i2c_isr, IRQF_TRIGGER_FALLING,
			                        "pp876ax_i2c_client", dev);
	if (ret) {
		dev_err(&client->dev, "failure requesting irq %d (error %d)\n",
		                                dev->irq, ret);
		goto err_request_irq;
	}

	dev_info(&client->dev, "pp87xax probed\n");
	return 0;

err_request_irq:
	kfree(dev);

	return ret;
}

static int __devexit pp876ax_remove(struct i2c_client *client)
{
	struct pp876ax_client *dev = i2c_get_clientdata(client);

	/* TODO: do something */

	kfree(dev);
	return 0;
}

#ifdef CONFIG_PM
static int pp876ax_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct pp876ax_device *dev = i2c_get_clientdata(client);
	
	return 0;
}

static int pp876ax_resume(struct i2c_client *client)
{
	struct pp876ax_device *dev = i2c_get_clientdata(client);

	return 0;
}
#else
#define pp876ax_suspend NULL
#define pp876ax_resume  NULL
#endif

static const struct i2c_device_id pp876ax_id[] = {
	{ "pp876ax_i2c_client", 0 },
	{ }
};

static struct i2c_driver pp876ax_driver = {
	.probe    = pp876ax_probe,
	.remove   = __devexit_p(pp876ax_remove),
	.id_table = pp876ax_id,
	.suspend  = pp876ax_suspend,
	.resume   = pp876ax_resume,
	.driver   = {
		.name = "pp876ax_i2c_client",
	},
};

static int __init pp876ax_init_driver(void)
{
	return i2c_add_driver(&pp876ax_driver);
}

static void __exit pp876ax_exit_driver(void)
{
	i2c_del_driver(&pp876ax_driver);
}

module_init(pp876ax_init_driver);
module_exit(pp876ax_exit_driver);

MODULE_DESCRIPTION("PP876AX I2C client driver");
MODULE_LICENSE("GPL");
