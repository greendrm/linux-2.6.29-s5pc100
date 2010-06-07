/* linux/arch/arm/mach-s5pc110/include/mach/system.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com
 *
 * S5PC110 - system implementation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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
