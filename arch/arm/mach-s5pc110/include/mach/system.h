/* linux/arch/arm/mach-s5pc110/include/mach/system.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5PC110 - system implementation
 */

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H __FILE__

void (*s5pc11x_idle)(void);
void (*s5pc11x_reset_hook)(void);

void s5pc11x_default_idle(void)
{
	printk("default idle function\n");
}

static void arch_idle(void)
{
	if(s5pc11x_idle != NULL)
		(s5pc11x_idle)();
	else
		s5pc11x_default_idle();
}

#include <mach/system-reset.h>

#endif /* __ASM_ARCH_IRQ_H */
