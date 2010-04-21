/* linux/arch/arm/mach-s5pc110/pm.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com/
 *
 * S5PC110 (and compatible) Power Manager (Suspend-To-RAM) support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/sysdev.h>
#include <linux/io.h>

#include <mach/hardware.h>

#include <asm/mach-types.h>

#include <plat/regs-gpio.h>
#include <plat/regs-clock.h>
#include <plat/cpu.h>
#include <plat/pm.h>


#define DBG(fmt...) printk(KERN_DEBUG fmt)

void s5pc110_cpu_suspend(void)
{
	unsigned long tmp;

	/* issue the standby signal into the pm unit. Note, we
	 * issue a write-buffer drain just in case */

	tmp = 0;
/*
 * MCR p15,0,<Rd>,c7,c10,5 ; Data Memory Barrier Operation.
 * MCR p15,0,<Rd>,c7,c10,4 ; Data Synchronization Barrier operation.
 * MCR p15,0,<Rd>,c7,c0,4 ; Wait For Interrupt.
 */
#if defined(CONFIG_CPU_S5PC110_EVT0_ERRATA)

	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;
	__raw_writel(tmp, S5P_PWR_CFG);
	
	/*
	 * Use POWER MODE register to enter sleep mode.
	 */
	tmp = (1<<2);
	__raw_writel(tmp, S5P_PWR_MODE);
	
	while(1);
#else
	cpu_do_idle();
	/* we should never get past here */

	panic("sleep resumed to originator?");

#endif
}

static void s5pc110_pm_prepare(void)
{

}

static int s5pc110_pm_add(struct sys_device *sysdev)
{
	pm_cpu_prep = s5pc110_pm_prepare;
	pm_cpu_sleep = s5pc110_cpu_suspend;

	return 0;
}

static struct sleep_save s5pc110_sleep[] = {

};

static int s5pc110_pm_resume(struct sys_device *dev)
{
	s5pc11x_pm_do_restore(s5pc110_sleep, ARRAY_SIZE(s5pc110_sleep));
	return 0;
}

static struct sysdev_driver s5pc110_pm_driver = {
	.add		= s5pc110_pm_add,
	.resume		= s5pc110_pm_resume,
};

static __init int s5pc110_pm_drvinit(void)
{
	printk("S5PC110 Power driver init\n");
	return sysdev_driver_register(&s5pc110_sysclass, &s5pc110_pm_driver);
}

arch_initcall(s5pc110_pm_drvinit);

