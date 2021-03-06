/* arch/arm/plat-s5pc11x/include/plat/pm.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com
 *
 * Based on plat-s3c24xx/include/plat/pm.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifdef CONFIG_PM

extern __init int s5pc11x_pm_init(void);

#else

static inline int s5pc11x_pm_init(void)
{
	return 0;
}
#endif

/* configuration for the IRQ mask over sleep */
extern unsigned long s5pc11x_irqwake_intmask;
extern unsigned long s5pc11x_irqwake_eintmask;

/* IRQ masks for IRQs allowed to go to sleep (see irq.c) */
extern unsigned long s5pc11x_irqwake_intallow;
extern unsigned long s5pc11x_irqwake_eintallow;

/* per-cpu sleep functions */

extern void (*pm_cpu_prep)(void);
extern void (*pm_cpu_sleep)(void);

/* Flags for PM Control */

extern unsigned long s5pc100_pm_flags;

/* from sleep.S */

extern int  s5pc110_cpu_save(unsigned long *saveblk);
extern void s5pc110_cpu_suspend(void);
extern void s5pc110_cpu_resume(void);

extern unsigned long s5pc110_sleep_save_phys;

#define SLEEP_MODE	0
#define DEEPIDLE_MODE	1

/* sleep save info */

struct sleep_save {
	void __iomem	*reg;
	unsigned long	val;
};

struct sleep_save_phy {
	unsigned long	reg;
	unsigned long	val;
};

#define SAVE_ITEM(x) \
	{ .reg = (x) }

extern void s5pc11x_pm_do_save_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count);
extern void s5pc11x_pm_do_restore_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count);
extern void s5pc11x_pm_do_save(struct sleep_save *ptr, int count);
extern void s5pc11x_pm_do_restore(struct sleep_save *ptr, int count);

extern void printascii(const char *);
