#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/crc32.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>

#include <plat/map-base.h>
#include <plat/regs-serial.h>
#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>

#include <mach/regs-mem.h>
#include <mach/regs-irq.h>
#include <asm/gpio.h>

static irqreturn_t
s3c_button_interrupt(int irq, void *dev_id)
{

	switch (irq) {

		case IRQ_EINT10:
			printk("IRQ_EINT10 Interrupt occured \n");
			break;

		case IRQ_EINT11:
			printk("IRQ_EINT11 Interrupt occured \n");
			break;

		default:
			printk("Button Interrupt occured \n");
			break;
	}

	return IRQ_HANDLED;
}

static struct irqaction s3c_button_irq = {
	.name		= "s3c button Tick",
	.flags		= IRQF_SHARED ,
	.handler	= s3c_button_interrupt,
};

static unsigned int s3c_button_gpio_init(void)
{
	u32 err;

	err = gpio_request(S5P64XX_GPH1(2),"GPH1");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		s3c_gpio_cfgpin(S5P64XX_GPH1(2),S5P64XX_GPH1_2_EXT_INT1_2);
		s3c_gpio_setpull(S5P64XX_GPH1(2), S3C_GPIO_PULL_UP);
	}

	err = gpio_request(S5P64XX_GPH1(3),"GPH1");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		s3c_gpio_cfgpin(S5P64XX_GPH1(3),S5P64XX_GPH1_3_EXT_INT1_3);
		s3c_gpio_setpull(S5P64XX_GPH1(3), S3C_GPIO_PULL_UP);
	}


	err = gpio_request(S5P64XX_GPH1(7),"GPH1");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		s3c_gpio_cfgpin(S5P64XX_GPH1(7),S5P64XX_GPH1_7_EXT_INT1_7);
		s3c_gpio_setpull(S5P64XX_GPH1(7), S3C_GPIO_PULL_UP);
	}

	return err;
}

static void __init s3c_button_init(void)
{

	printk("########## SMDK6442 Button init function \n");

	if (s3c_button_gpio_init()) {
		printk(KERN_ERR "%s failed\n", __FUNCTION__);
		return;
	}

	set_irq_type(IRQ_EINT10, IRQF_TRIGGER_FALLING);
	setup_irq(IRQ_EINT10, &s3c_button_irq);

	set_irq_type(IRQ_EINT11, IRQF_TRIGGER_FALLING);
	setup_irq(IRQ_EINT11, &s3c_button_irq);

	set_irq_type(IRQ_EINT15, IRQF_TRIGGER_FALLING);
	setup_irq(IRQ_EINT15, &s3c_button_irq);

}

device_initcall(s3c_button_init);
