/* arch/arm/mach-s3c2410/include/mach/system-reset.h
 *
 * Copyright (c) 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C2410 - System define for arch_reset() function
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <mach/hardware.h>
#include <linux/io.h>
#include <asm/proc-fns.h>

#include <plat/regs-clock.h>
#include <plat/regs-watchdog.h>
#include <plat/map.h>

#include <linux/clk.h>
#include <linux/err.h>

extern void (*s5pc11x_reset_hook)(void);

static void
arch_reset(char mode)
{
	struct clk *wdtclk;

	if (mode == 's') {
		cpu_reset(0);
	}

	if (s5pc11x_reset_hook)
		s5pc11x_reset_hook();
#if !defined(CONFIG_S3C2410_WATCHDOG)
	printk("arch_reset: attempting watchdog reset\n");

	__raw_writel(0, S3C2410_WTCON);	  /* disable watchdog, to be safe  */

	wdtclk = clk_get(NULL, "watchdog");
	if (!IS_ERR(wdtclk)) {
		clk_enable(wdtclk);
	} else
		printk(KERN_WARNING "%s: warning: cannot get watchdog clock\n", __func__);

	/* put initial values into count and data */
	__raw_writel(0x80, S3C2410_WTCNT);
	__raw_writel(0x80, S3C2410_WTDAT);

	/* set the watchdog to go and reset... */
	__raw_writel(S3C2410_WTCON_ENABLE|S3C2410_WTCON_DIV16|S3C2410_WTCON_RSTEN |
		     S3C2410_WTCON_PRESCALE(0x20), S3C2410_WTCON);

	/* wait for reset to assert... */
	mdelay(500);

	printk(KERN_ERR "Watchdog reset failed to assert reset\n");

	/* delay to allow the serial port to show the message */
	mdelay(50);
#else

#warning "Watchdog is not used as a software reset source"

#endif

}
