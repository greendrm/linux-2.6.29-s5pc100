/* linux/arch/arm/plat-s5pc11x/pm.c
 *
 * Copyright (c) 2004,2009 Simtec Electronics
 *	boyko.lee <boyko.lee@samsung.com>
 *
 * S5PC11X Power Manager (Suspend-To-RAM) support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Parts based on arch/arm/mach-pxa/pm.c
 *
 * Thanks to Dimitry Andric for debugging
*/

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

#include <asm/mach/time.h>

#include <plat/pm.h>

#define PFX "s5pc11x-pm: "
static struct sleep_save core_save[] = {
#if 0
	SAVE_ITEM(S5P_CLK_SRC0),
	SAVE_ITEM(S5P_CLK_SRC1),
	SAVE_ITEM(S5P_CLK_SRC2),
	SAVE_ITEM(S5P_CLK_SRC3),
	
	SAVE_ITEM(S5P_CLK_DIV0),
	SAVE_ITEM(S5P_CLK_DIV1),
	SAVE_ITEM(S5P_CLK_DIV2),
	SAVE_ITEM(S5P_CLK_DIV3),
	SAVE_ITEM(S5P_CLK_DIV4),

	SAVE_ITEM(S5P_CLK_OUT),

	SAVE_ITEM(S5P_CLKGATE_D00),
	SAVE_ITEM(S5P_CLKGATE_D01),
	SAVE_ITEM(S5P_CLKGATE_D02),

	SAVE_ITEM(S5P_CLKGATE_D10),
	SAVE_ITEM(S5P_CLKGATE_D11),
	SAVE_ITEM(S5P_CLKGATE_D12),
	SAVE_ITEM(S5P_CLKGATE_D13),
	SAVE_ITEM(S5P_CLKGATE_D14),
	SAVE_ITEM(S5P_CLKGATE_D15),

	SAVE_ITEM(S5P_CLKGATE_D20),

	SAVE_ITEM(S5P_SCLKGATE0),
	SAVE_ITEM(S5P_SCLKGATE1),

	SAVE_ITEM(S5P_MEM_SYS_CFG),
	SAVE_ITEM(S5P_CAM_MUX_SEL),
	SAVE_ITEM(S5P_MIXER_OUT_SEL),

	SAVE_ITEM(S5P_LPMP_MODE_SEL),
	SAVE_ITEM(S5P_MIPI_PHY_CON0),
	SAVE_ITEM(S5P_MIPI_PHY_CON1),
	SAVE_ITEM(S5P_HDMI_PHY_CON0),
#endif
};

static struct sleep_save gpio_save[] = {
#if 0
	SAVE_ITEM(S5PC11X_GPA0CON),
	SAVE_ITEM(S5PC11X_GPA0DAT),
	SAVE_ITEM(S5PC11X_GPA0PUD),
	SAVE_ITEM(S5PC11X_GPA1CON),
	SAVE_ITEM(S5PC11X_GPA1DAT),
	SAVE_ITEM(S5PC11X_GPA1PUD),
	SAVE_ITEM(S5PC11X_GPBCON),
	SAVE_ITEM(S5PC11X_GPBDAT),
	SAVE_ITEM(S5PC11X_GPBPUD),
	SAVE_ITEM(S5PC11X_GPCCON),
	SAVE_ITEM(S5PC11X_GPCDAT),
	SAVE_ITEM(S5PC11X_GPCPUD),
	SAVE_ITEM(S5PC11X_GPDCON),
	SAVE_ITEM(S5PC11X_GPDDAT),
	SAVE_ITEM(S5PC11X_GPDPUD),
	SAVE_ITEM(S5PC11X_GPE0CON),
	SAVE_ITEM(S5PC11X_GPE0DAT),
	SAVE_ITEM(S5PC11X_GPE0PUD),
	SAVE_ITEM(S5PC11X_GPE1CON),
	SAVE_ITEM(S5PC11X_GPE1DAT),
	SAVE_ITEM(S5PC11X_GPE1PUD),
	SAVE_ITEM(S5PC11X_GPF0CON),
	SAVE_ITEM(S5PC11X_GPF0DAT),
	SAVE_ITEM(S5PC11X_GPF0PUD),
	SAVE_ITEM(S5PC11X_GPF1CON),
	SAVE_ITEM(S5PC11X_GPF1DAT),
	SAVE_ITEM(S5PC11X_GPF1PUD),
	SAVE_ITEM(S5PC11X_GPF2CON),
	SAVE_ITEM(S5PC11X_GPF2DAT),
	SAVE_ITEM(S5PC11X_GPF2PUD),
	SAVE_ITEM(S5PC11X_GPF3CON),
	SAVE_ITEM(S5PC11X_GPF3DAT),
	SAVE_ITEM(S5PC11X_GPF3PUD),
	SAVE_ITEM(S5PC11X_GPG0CON),
	SAVE_ITEM(S5PC11X_GPG0DAT),
	SAVE_ITEM(S5PC11X_GPG0PUD),
	SAVE_ITEM(S5PC11X_GPG1CON),
	SAVE_ITEM(S5PC11X_GPG1DAT),
	SAVE_ITEM(S5PC11X_GPG1PUD),
	SAVE_ITEM(S5PC11X_GPG2CON),
	SAVE_ITEM(S5PC11X_GPG2DAT),
	SAVE_ITEM(S5PC11X_GPG2PUD),
	SAVE_ITEM(S5PC11X_GPG3CON),
	SAVE_ITEM(S5PC11X_GPG3DAT),
	SAVE_ITEM(S5PC11X_GPG3PUD),
	SAVE_ITEM(S5PC11X_GPH0CON),
	SAVE_ITEM(S5PC11X_GPH0DAT),
	SAVE_ITEM(S5PC11X_GPH0PUD),
	SAVE_ITEM(S5PC11X_GPH1CON),
	SAVE_ITEM(S5PC11X_GPH1DAT),
	SAVE_ITEM(S5PC11X_GPH1PUD),
	SAVE_ITEM(S5PC11X_GPH2CON),
	SAVE_ITEM(S5PC11X_GPH2DAT),
	SAVE_ITEM(S5PC11X_GPH2PUD),
	SAVE_ITEM(S5PC11X_GPH3CON),
	SAVE_ITEM(S5PC11X_GPH3DAT),
	SAVE_ITEM(S5PC11X_GPH3PUD),
	SAVE_ITEM(S5PC11X_GPICON),
	SAVE_ITEM(S5PC11X_GPIDAT),
	SAVE_ITEM(S5PC11X_GPIPUD),
	SAVE_ITEM(S5PC11X_GPJ0CON),
	SAVE_ITEM(S5PC11X_GPJ0DAT),
	SAVE_ITEM(S5PC11X_GPJ0PUD),
	SAVE_ITEM(S5PC11X_GPJ1CON),
	SAVE_ITEM(S5PC11X_GPJ1DAT),
	SAVE_ITEM(S5PC11X_GPJ1PUD),
	SAVE_ITEM(S5PC11X_GPJ2CON),
	SAVE_ITEM(S5PC11X_GPJ2DAT),
	SAVE_ITEM(S5PC11X_GPJ2PUD),
	SAVE_ITEM(S5PC11X_GPJ3CON),
	SAVE_ITEM(S5PC11X_GPJ3DAT),
	SAVE_ITEM(S5PC11X_GPJ3PUD),
	SAVE_ITEM(S5PC11X_GPK0CON),
	SAVE_ITEM(S5PC11X_GPK0DAT),
	SAVE_ITEM(S5PC11X_GPK0PUD),
	SAVE_ITEM(S5PC11X_GPK1CON),
	SAVE_ITEM(S5PC11X_GPK1DAT),
	SAVE_ITEM(S5PC11X_GPK1PUD),
	SAVE_ITEM(S5PC11X_GPK2CON),
	SAVE_ITEM(S5PC11X_GPK2DAT),
	SAVE_ITEM(S5PC11X_GPK2PUD),
	SAVE_ITEM(S5PC11X_GPK3CON),
	SAVE_ITEM(S5PC11X_GPK3DAT),
	SAVE_ITEM(S5PC11X_GPK3PUD),
#endif
};

/* this lot should be really saved by the IRQ code */
/* VICXADDRESSXX initilaization to be needed */
static struct sleep_save irq_save[] = {
#if 0
	SAVE_ITEM(S5PC110_VIC0REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC110_VIC1REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC110_VIC2REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC110_VIC0REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC110_VIC1REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC110_VIC2REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC110_VIC0REG(VIC_INT_SOFT)),
	SAVE_ITEM(S5PC110_VIC1REG(VIC_INT_SOFT)),
	SAVE_ITEM(S5PC110_VIC2REG(VIC_INT_SOFT)),
#endif
};

static struct sleep_save sromc_save[] = {
#if 0
	SAVE_ITEM(S5PC11X_SROM_BW),
	SAVE_ITEM(S5PC11X_SROM_BC0),
	SAVE_ITEM(S5PC11X_SROM_BC1),
	SAVE_ITEM(S5PC11X_SROM_BC2),
	SAVE_ITEM(S5PC11X_SROM_BC3),
	SAVE_ITEM(S5PC11X_SROM_BC4),
	SAVE_ITEM(S5PC11X_SROM_BC5),
#endif
};

#define SAVE_UART(va) \
	SAVE_ITEM((va) + S3C2410_ULCON), \
	SAVE_ITEM((va) + S3C2410_UCON), \
	SAVE_ITEM((va) + S3C2410_UFCON), \
	SAVE_ITEM((va) + S3C2410_UMCON), \
	SAVE_ITEM((va) + S3C2410_UBRDIV), \
	SAVE_ITEM((va) + S3C2410_UDIVSLOT), \
	SAVE_ITEM((va) + S3C2410_UINTMSK)


static struct sleep_save uart_save[] = {
#if 0
	SAVE_UART(S3C24XX_VA_UART0),
#endif
};

#define DBG(fmt...)

#define s5pc11x_pm_debug_init() do { } while(0)
#define s5pc11x_pm_check_prepare() do { } while(0)
#define s5pc11x_pm_check_restore() do { } while(0)
#define s5pc11x_pm_check_store()   do { } while(0)

/* helper functions to save and restore register state */

void s5pc11x_pm_do_save(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		ptr->val = __raw_readl(ptr->reg);
		//DBG("saved %p value %08lx\n", ptr->reg, ptr->val);
	}
}

/* s5pc11x_pm_do_restore
 *
 * restore the system from the given list of saved registers
 *
 * Note, we do not use DBG() in here, as the system may not have
 * restore the UARTs state yet
*/

void s5pc11x_pm_do_restore(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		//printk(KERN_DEBUG "restore %p (restore %08lx, was %08x)\n",
		       //ptr->reg, ptr->val, __raw_readl(ptr->reg));

		__raw_writel(ptr->val, ptr->reg);
	}
}

/* s5pc11x_pm_do_restore_core
 *
 * similar to s36410_pm_do_restore_core
 *
 * WARNING: Do not put any debug in here that may effect memory or use
 * peripherals, as things may be changing!
*/

/* s5pc11x_pm_do_save_phy
 *
 * save register of system
 *
 * Note, I made this function to support driver with ioremap.
 * If you want to use this function, you should to input as first parameter
 * struct sleep_save_phy type
*/

void s5pc11x_pm_do_save_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count)
{
	void __iomem *target_reg;
	struct resource *res;
	u32 reg_size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_size = res->end - res->start + 1;
	target_reg = ioremap(res->start,reg_size);

	for (; count > 0; count--, ptr++) {
		ptr->val = readl(target_reg + (ptr->reg));
	}
}

/* s5pc11x_pm_do_restore_phy
 *
 * restore register of system
 *
 * Note, I made this function to support driver with ioremap.
 * If you want to use this function, you should to input as first parameter
 * struct sleep_save_phy type
*/

void s5pc11x_pm_do_restore_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count)
{
	void __iomem *target_reg;
	struct resource *res;
	u32 reg_size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_size = res->end - res->start + 1;
	target_reg = ioremap(res->start,reg_size);

	for (; count > 0; count--, ptr++) {
		writel(ptr->val, (target_reg + ptr->reg));
	}
}

static void s5pc11x_pm_do_restore_core(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		__raw_writel(ptr->val, ptr->reg);
	}
}

/* s5pc11x_pm_show_resume_irqs
 *
 * print any IRQs asserted at resume time (ie, we woke from)
*/

static void s5pc11x_pm_show_resume_irqs(int start, unsigned long which,
					unsigned long mask)
{
	int i;

	which &= ~mask;

	for (i = 0; i <= 31; i++) {
		if ((which) & (1L<<i)) {
			DBG("IRQ %d asserted at resume\n", start+i);
		}
	}
}

void (*pm_cpu_prep)(void);
void (*pm_cpu_sleep)(void);

#define any_allowed(mask, allow) (((mask) & (allow)) != (allow))


/* s5pc11x_pm_enter
 *
 * central control for sleep/resume process
*/
static int s5pc11x_pm_enter(suspend_state_t state)
{
	unsigned long regs_save[16];
	unsigned int tmp;

	/* ensure the debug is initialised (if enabled) */

	DBG("s5pc11x_pm_enter(%d)\n", state);

	if (pm_cpu_prep == NULL || pm_cpu_sleep == NULL) {
		printk(KERN_ERR PFX "error: no cpu sleep functions set\n");
		return -EINVAL;
	}

	/* store the physical address of the register recovery block */
	s5pc110_sleep_save_phys = virt_to_phys(regs_save);

	DBG("s5pc11x_sleep_save_phys=0x%08lx\n", s5pc110_sleep_save_phys);

	s5pc11x_pm_do_save(gpio_save, ARRAY_SIZE(gpio_save));
	s5pc11x_pm_do_save(irq_save, ARRAY_SIZE(irq_save));
	s5pc11x_pm_do_save(core_save, ARRAY_SIZE(core_save));
	s5pc11x_pm_do_save(sromc_save, ARRAY_SIZE(sromc_save));
	s5pc11x_pm_do_save(uart_save, ARRAY_SIZE(uart_save));

	/* ensure INF_REG0  has the resume address */
	__raw_writel(virt_to_phys(s5pc110_cpu_resume), S5P_INFORM0);

	/* call cpu specific preperation */

	pm_cpu_prep();

	/* flush cache back to ram */
	flush_cache_all();

	/* send the cpu to sleep... */
	__raw_writel(0xffffffff, S5PC110_VIC0REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC1REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC2REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC0REG(VIC_INT_SOFT_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC1REG(VIC_INT_SOFT_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC2REG(VIC_INT_SOFT_CLEAR));

	/* s5pc11x_cpu_save will also act as our return point from when
	 * we resume as it saves its own register state, so use the return
	 * code to differentiate return from save and return from sleep */

	if (s5pc110_cpu_save(regs_save) == 0) {
		flush_cache_all();
		/* This function for Chip bug on EVT0 */
		pm_cpu_sleep();
	}

	/* restore the cpu state */
	cpu_init();

	s5pc11x_pm_do_restore(gpio_save, ARRAY_SIZE(gpio_save));
	s5pc11x_pm_do_restore(irq_save, ARRAY_SIZE(irq_save));
	s5pc11x_pm_do_restore(core_save, ARRAY_SIZE(core_save));
	s5pc11x_pm_do_restore(sromc_save, ARRAY_SIZE(sromc_save));
	s5pc11x_pm_do_restore(uart_save, ARRAY_SIZE(uart_save));

	DBG("post sleep, preparing to return\n");

	s5pc11x_pm_check_restore();

	/* ok, let's return from sleep */
	DBG("S3C6410 PM Resume (post-restore)\n");
	return 0;
}


static struct platform_suspend_ops s5pc11x_pm_ops = {
	.enter		= s5pc11x_pm_enter,
	.valid		= suspend_valid_only_mem,
};

/* s5pc11x_pm_init
 *
 * Attach the power management functions. This should be called
 * from the board specific initialisation if the board supports
 * it.
*/

int __init s5pc11x_pm_init(void)
{
	printk("s5pc11x Power Management, (c) 2008 Samsung Electronics\n");
	/* set the irq configuration for wake */
	suspend_set_ops(&s5pc11x_pm_ops);
	return 0;
}
