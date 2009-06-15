/* linux/arch/arm/mach-s5pc100/cpu.c
 *
 * Copyright 2008 Samsung Electronics
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/sysdev.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/proc-fns.h>
#include <asm/irq.h>

#include <mach/hardware.h>
#include <mach/idle.h>
#include <mach/map.h>

#include <plat/cpu-freq.h>
#include <plat/regs-serial.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>
#include <plat/sdhci.h>
#include <plat/iic-core.h>
#include <plat/s5pc110.h>

#include <plat/regs-power.h>
#include <plat/regs-clock.h>

#undef T32_PROBE_DEBUGGING

#if defined(T32_PROBE_DEBUGGING)
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#endif

/* Initial IO mappings */

static struct map_desc s5pc110_iodesc[] __initdata = {
	IODESC_ENT(LCD),
	IODESC_ENT(SROMC),
	IODESC_ENT(SYSTIMER),
        IODESC_ENT(OTG),
        IODESC_ENT(OTGSFR),
        IODESC_ENT(SYSCON),
        IODESC_ENT(GPIO),
        IODESC_ENT(NAND),
	//IODESC_ENT(HOSTIFB),
};

/* s5pc110_map_io
 *
 * register the standard cpu IO areas
*/

static void s5pc110_idle(void)
{
	unsigned int tmp;
/*
 * 1. Set CFG_STANDBYWFI field of PWR_CFG to 2¡¯b01.
 * 2. Set PMU_INT_DISABLE bit of OTHERS register to 1¡¯b1 to prevent interrupts from
 *    occurring while entering IDLE mode.
 * 3. Execute Wait For Interrupt instruction (WFI).
*/
}

void __init s5pc110_map_io(void)
{
	iotable_init(s5pc110_iodesc, ARRAY_SIZE(s5pc110_iodesc));

	/* set s5pc110 idle function */
	s5pc11x_idle = s5pc110_idle;
}

void __init s5pc110_init_clocks(int xtal)
{
	printk(KERN_DEBUG "%s: initialising clocks\n", __func__);
	s3c24xx_register_baseclocks(xtal);
	s5pc11x_register_clocks();
	s5pc110_register_clocks();
	s5pc110_setup_clocks();
}

void __init s5pc110_init_irq(void)
{
	/* VIC0, VIC1, and VIC2 are fully populated. */
	s5pc11x_init_irq(~0, ~0, ~0, ~0);
}

struct sysdev_class s5pc110_sysclass = {
	.name	= "s5pc110-core",
};

static struct sys_device s5pc110_sysdev = {
	.cls	= &s5pc110_sysclass,
};

static int __init s5pc110_core_init(void)
{
	return sysdev_class_register(&s5pc110_sysclass);
}

core_initcall(s5pc110_core_init);

int __init s5pc110_init(void)
{
	printk("S5PC110: Initialising architecture\n");

	return sysdev_register(&s5pc110_sysdev);
}
