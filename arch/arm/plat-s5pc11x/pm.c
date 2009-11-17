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
#include <linux/regulator/machine.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>

#include <plat/map-base.h>
#include <plat/regs-serial.h>
#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-mem.h>
#include <mach/regs-irq.h>
#include <linux/gpio.h>

#include <asm/mach/time.h>

#include <plat/pm.h>

#define PFX "s5pc11x-pm: "

#define DEBUG_S5P_PM

#if defined(DEBUG_S5P_PM)
static void s5p_low_lvl_debug(char *buf, char *name, void  __iomem *regs)
{
	sprintf(buf, "%s:%08x\n", name,
			__raw_readl(regs));
	printascii(buf);
}
#else
#define s5p_low_lvl_debug()	NULL
#endif

static struct sleep_save core_save[] = {
	/* Clock source */
	SAVE_ITEM(S5P_CLK_SRC0),
	SAVE_ITEM(S5P_CLK_SRC1),
	SAVE_ITEM(S5P_CLK_SRC2),
	SAVE_ITEM(S5P_CLK_SRC3),
	SAVE_ITEM(S5P_CLK_SRC4),
	SAVE_ITEM(S5P_CLK_SRC5),
	SAVE_ITEM(S5P_CLK_SRC6),
	/* Clock source Mask */
	SAVE_ITEM(S5P_CLK_SRC_MASK0),
	SAVE_ITEM(S5P_CLK_SRC_MASK1),
	/* Clock Divider */
	/*SAVE_ITEM(S5P_CLK_DIV0),*/
	SAVE_ITEM(S5P_CLK_DIV1),
	SAVE_ITEM(S5P_CLK_DIV2),
	SAVE_ITEM(S5P_CLK_DIV3),
	SAVE_ITEM(S5P_CLK_DIV4),
	SAVE_ITEM(S5P_CLK_DIV5),
	SAVE_ITEM(S5P_CLK_DIV6),
	SAVE_ITEM(S5P_CLK_DIV7),
	/* Clock IP Clock gate */
	SAVE_ITEM(S5P_CLKGATE_IP0),
	SAVE_ITEM(S5P_CLKGATE_IP1),
	SAVE_ITEM(S5P_CLKGATE_IP2),
	SAVE_ITEM(S5P_CLKGATE_IP3),
	SAVE_ITEM(S5P_CLKGATE_IP4),
	/* Clock Blcok and Bus gate */
	SAVE_ITEM(S5P_CLKGATE_BLOCK),
	SAVE_ITEM(S5P_CLKGATE_BUS0),
	SAVE_ITEM(S5P_CLKGATE_BUS1),
	/* Clock ETC */
	SAVE_ITEM(S5P_CLK_OUT),
	SAVE_ITEM(S5P_MDNIE_SEL),
};

static struct sleep_save gpio_save[] = {
	SAVE_ITEM(S5PC11X_GPA0CON),
	SAVE_ITEM(S5PC11X_GPA0DAT),
	SAVE_ITEM(S5PC11X_GPA0PUD),
	SAVE_ITEM(S5PC11X_GPA0DRV),
	SAVE_ITEM(S5PC11X_GPA0CONPDN),
	SAVE_ITEM(S5PC11X_GPA0PUDPDN),
	SAVE_ITEM(S5PC11X_GPA1CON),
	SAVE_ITEM(S5PC11X_GPA1DAT),
	SAVE_ITEM(S5PC11X_GPA1PUD),
	SAVE_ITEM(S5PC11X_GPA1DRV),
	SAVE_ITEM(S5PC11X_GPA1CONPDN),
	SAVE_ITEM(S5PC11X_GPA1PUDPDN),
	SAVE_ITEM(S5PC11X_GPBCON),
	SAVE_ITEM(S5PC11X_GPBDAT),
	SAVE_ITEM(S5PC11X_GPBPUD),
	SAVE_ITEM(S5PC11X_GPBDRV),
	SAVE_ITEM(S5PC11X_GPBCONPDN),
	SAVE_ITEM(S5PC11X_GPBPUDPDN),
	SAVE_ITEM(S5PC11X_GPC0CON),
	SAVE_ITEM(S5PC11X_GPC0DAT),
	SAVE_ITEM(S5PC11X_GPC0PUD),
	SAVE_ITEM(S5PC11X_GPC0DRV),
	SAVE_ITEM(S5PC11X_GPC0CONPDN),
	SAVE_ITEM(S5PC11X_GPC0PUDPDN),
	SAVE_ITEM(S5PC11X_GPC1CON),
	SAVE_ITEM(S5PC11X_GPC1DAT),
	SAVE_ITEM(S5PC11X_GPC1PUD),
	SAVE_ITEM(S5PC11X_GPC1DRV),
	SAVE_ITEM(S5PC11X_GPC1CONPDN),
	SAVE_ITEM(S5PC11X_GPC1PUDPDN),
	SAVE_ITEM(S5PC11X_GPD0CON),
	SAVE_ITEM(S5PC11X_GPD0DAT),
	SAVE_ITEM(S5PC11X_GPD0PUD),
	SAVE_ITEM(S5PC11X_GPD0DRV),
	SAVE_ITEM(S5PC11X_GPD0CONPDN),
	SAVE_ITEM(S5PC11X_GPD0PUDPDN),
	SAVE_ITEM(S5PC11X_GPD1CON),
	SAVE_ITEM(S5PC11X_GPD1DAT),
	SAVE_ITEM(S5PC11X_GPD1PUD),
	SAVE_ITEM(S5PC11X_GPD1DRV),
	SAVE_ITEM(S5PC11X_GPD1CONPDN),
	SAVE_ITEM(S5PC11X_GPD1PUDPDN),
	SAVE_ITEM(S5PC11X_GPE0CON),
	SAVE_ITEM(S5PC11X_GPE0DAT),
	SAVE_ITEM(S5PC11X_GPE0PUD),
	SAVE_ITEM(S5PC11X_GPE0DRV),
	SAVE_ITEM(S5PC11X_GPE0CONPDN),
	SAVE_ITEM(S5PC11X_GPE0PUDPDN),
	SAVE_ITEM(S5PC11X_GPE1CON),
	SAVE_ITEM(S5PC11X_GPE1DAT),
	SAVE_ITEM(S5PC11X_GPE1PUD),
	SAVE_ITEM(S5PC11X_GPE1DRV),
	SAVE_ITEM(S5PC11X_GPE1CONPDN),
	SAVE_ITEM(S5PC11X_GPE1PUDPDN),
	SAVE_ITEM(S5PC11X_GPF0CON),
	SAVE_ITEM(S5PC11X_GPF0DAT),
	SAVE_ITEM(S5PC11X_GPF0PUD),
	SAVE_ITEM(S5PC11X_GPF0DRV),
	SAVE_ITEM(S5PC11X_GPF0CONPDN),
	SAVE_ITEM(S5PC11X_GPF0PUDPDN),
	SAVE_ITEM(S5PC11X_GPF1CON),
	SAVE_ITEM(S5PC11X_GPF1DAT),
	SAVE_ITEM(S5PC11X_GPF1PUD),
	SAVE_ITEM(S5PC11X_GPF1DRV),
	SAVE_ITEM(S5PC11X_GPF1CONPDN),
	SAVE_ITEM(S5PC11X_GPF1PUDPDN),
	SAVE_ITEM(S5PC11X_GPF2CON),
	SAVE_ITEM(S5PC11X_GPF2DAT),
	SAVE_ITEM(S5PC11X_GPF2PUD),
	SAVE_ITEM(S5PC11X_GPF2DRV),
	SAVE_ITEM(S5PC11X_GPF2CONPDN),
	SAVE_ITEM(S5PC11X_GPF2PUDPDN),
	SAVE_ITEM(S5PC11X_GPF3CON),
	SAVE_ITEM(S5PC11X_GPF3DAT),
	SAVE_ITEM(S5PC11X_GPF3PUD),
	SAVE_ITEM(S5PC11X_GPF3DRV),
	SAVE_ITEM(S5PC11X_GPF3CONPDN),
	SAVE_ITEM(S5PC11X_GPF3PUDPDN),
	SAVE_ITEM(S5PC11X_GPG0CON),
	SAVE_ITEM(S5PC11X_GPG0DAT),
	SAVE_ITEM(S5PC11X_GPG0PUD),
	SAVE_ITEM(S5PC11X_GPG0DRV),
	SAVE_ITEM(S5PC11X_GPG0CONPDN),
	SAVE_ITEM(S5PC11X_GPG0PUDPDN),
	SAVE_ITEM(S5PC11X_GPG1CON),
	SAVE_ITEM(S5PC11X_GPG1DAT),
	SAVE_ITEM(S5PC11X_GPG1PUD),
	SAVE_ITEM(S5PC11X_GPG1DRV),
	SAVE_ITEM(S5PC11X_GPG1CONPDN),
	SAVE_ITEM(S5PC11X_GPG1PUDPDN),
	SAVE_ITEM(S5PC11X_GPG2CON),
	SAVE_ITEM(S5PC11X_GPG2DAT),
	SAVE_ITEM(S5PC11X_GPG2PUD),
	SAVE_ITEM(S5PC11X_GPG2DRV),
	SAVE_ITEM(S5PC11X_GPG2CONPDN),
	SAVE_ITEM(S5PC11X_GPG2PUDPDN),
	SAVE_ITEM(S5PC11X_GPG3CON),
	SAVE_ITEM(S5PC11X_GPG3DAT),
	SAVE_ITEM(S5PC11X_GPG3PUD),
	SAVE_ITEM(S5PC11X_GPG3DRV),
	SAVE_ITEM(S5PC11X_GPG3CONPDN),
	SAVE_ITEM(S5PC11X_GPG3PUDPDN),
	SAVE_ITEM(S5PC11X_GPH0CON),
	SAVE_ITEM(S5PC11X_GPH0DAT),
	SAVE_ITEM(S5PC11X_GPH0PUD),
	SAVE_ITEM(S5PC11X_GPH0DRV),
	SAVE_ITEM(S5PC11X_GPH0CONPDN),
	SAVE_ITEM(S5PC11X_GPH0PUDPDN),
	SAVE_ITEM(S5PC11X_GPH1CON),
	SAVE_ITEM(S5PC11X_GPH1DAT),
	SAVE_ITEM(S5PC11X_GPH1PUD),
	SAVE_ITEM(S5PC11X_GPH1DRV),
	SAVE_ITEM(S5PC11X_GPH1CONPDN),
	SAVE_ITEM(S5PC11X_GPH1PUDPDN),
	SAVE_ITEM(S5PC11X_GPH2CON),
	SAVE_ITEM(S5PC11X_GPH2DAT),
	SAVE_ITEM(S5PC11X_GPH2PUD),
	SAVE_ITEM(S5PC11X_GPH2DRV),
	SAVE_ITEM(S5PC11X_GPH2CONPDN),
	SAVE_ITEM(S5PC11X_GPH2PUDPDN),
	SAVE_ITEM(S5PC11X_GPH3CON),
	SAVE_ITEM(S5PC11X_GPH3DAT),
	SAVE_ITEM(S5PC11X_GPH3PUD),
	SAVE_ITEM(S5PC11X_GPH3DRV),
	SAVE_ITEM(S5PC11X_GPH3CONPDN),
	SAVE_ITEM(S5PC11X_GPH3PUDPDN),
	SAVE_ITEM(S5PC11X_GPICON),
	SAVE_ITEM(S5PC11X_GPIDAT),
	SAVE_ITEM(S5PC11X_GPIPUD),
	SAVE_ITEM(S5PC11X_GPIDRV),
	SAVE_ITEM(S5PC11X_GPICONPDN),
	SAVE_ITEM(S5PC11X_GPIPUDPDN),
	SAVE_ITEM(S5PC11X_GPJ0CON),
	SAVE_ITEM(S5PC11X_GPJ0DAT),
	SAVE_ITEM(S5PC11X_GPJ0PUD),
	SAVE_ITEM(S5PC11X_GPJ0DRV),
	SAVE_ITEM(S5PC11X_GPJ0CONPDN),
	SAVE_ITEM(S5PC11X_GPJ0PUDPDN),
	SAVE_ITEM(S5PC11X_GPJ1CON),
	SAVE_ITEM(S5PC11X_GPJ1DAT),
	SAVE_ITEM(S5PC11X_GPJ1PUD),
	SAVE_ITEM(S5PC11X_GPJ1DRV),
	SAVE_ITEM(S5PC11X_GPJ1CONPDN),
	SAVE_ITEM(S5PC11X_GPJ1PUDPDN),
	SAVE_ITEM(S5PC11X_GPJ2CON),
	SAVE_ITEM(S5PC11X_GPJ2DAT),
	SAVE_ITEM(S5PC11X_GPJ2PUD),
	SAVE_ITEM(S5PC11X_GPJ2DRV),
	SAVE_ITEM(S5PC11X_GPJ2CONPDN),
	SAVE_ITEM(S5PC11X_GPJ2PUDPDN),
	SAVE_ITEM(S5PC11X_GPJ3CON),
	SAVE_ITEM(S5PC11X_GPJ3DAT),
	SAVE_ITEM(S5PC11X_GPJ3PUD),
	SAVE_ITEM(S5PC11X_GPJ3DRV),
	SAVE_ITEM(S5PC11X_GPJ3CONPDN),
	SAVE_ITEM(S5PC11X_GPJ3PUDPDN),
	SAVE_ITEM(S5PC11X_GPJ4CON),
	SAVE_ITEM(S5PC11X_GPJ4DAT),
	SAVE_ITEM(S5PC11X_GPJ4PUD),
	SAVE_ITEM(S5PC11X_GPJ4DRV),
	SAVE_ITEM(S5PC11X_GPJ4CONPDN),
	SAVE_ITEM(S5PC11X_GPJ4PUDPDN),
	SAVE_ITEM(S5PC11X_MP01CON),
	SAVE_ITEM(S5PC11X_MP01DAT),
	SAVE_ITEM(S5PC11X_MP01PUD),
	SAVE_ITEM(S5PC11X_MP01DRV),
	SAVE_ITEM(S5PC11X_MP01CONPDN),
	SAVE_ITEM(S5PC11X_MP01PUDPDN),
	SAVE_ITEM(S5PC11X_MP02CON),
	SAVE_ITEM(S5PC11X_MP02DAT),
	SAVE_ITEM(S5PC11X_MP02PUD),
	SAVE_ITEM(S5PC11X_MP02DRV),
	SAVE_ITEM(S5PC11X_MP02CONPDN),
	SAVE_ITEM(S5PC11X_MP02PUDPDN),
	SAVE_ITEM(S5PC11X_MP03CON),
	SAVE_ITEM(S5PC11X_MP03DAT),
	SAVE_ITEM(S5PC11X_MP03PUD),
	SAVE_ITEM(S5PC11X_MP03DRV),
	SAVE_ITEM(S5PC11X_MP03CONPDN),
	SAVE_ITEM(S5PC11X_MP03PUDPDN),
	SAVE_ITEM(S5PC11X_MP04CON),
	SAVE_ITEM(S5PC11X_MP04DAT),
	SAVE_ITEM(S5PC11X_MP04PUD),
	SAVE_ITEM(S5PC11X_MP04DRV),
	SAVE_ITEM(S5PC11X_MP04CONPDN),
	SAVE_ITEM(S5PC11X_MP04PUDPDN),
	SAVE_ITEM(S5PC11X_MP05CON),
	SAVE_ITEM(S5PC11X_MP05DAT),
	SAVE_ITEM(S5PC11X_MP05PUD),
	SAVE_ITEM(S5PC11X_MP05DRV),
	SAVE_ITEM(S5PC11X_MP05CONPDN),
	SAVE_ITEM(S5PC11X_MP05PUDPDN),
	SAVE_ITEM(S5PC11X_MP06CON),
	SAVE_ITEM(S5PC11X_MP06DAT),
	SAVE_ITEM(S5PC11X_MP06PUD),
	SAVE_ITEM(S5PC11X_MP06DRV),
	SAVE_ITEM(S5PC11X_MP06CONPDN),
	SAVE_ITEM(S5PC11X_MP06PUDPDN),
	SAVE_ITEM(S5PC11X_MP07CON),
	SAVE_ITEM(S5PC11X_MP07DAT),
	SAVE_ITEM(S5PC11X_MP07PUD),
	SAVE_ITEM(S5PC11X_MP07DRV),
	SAVE_ITEM(S5PC11X_MP07CONPDN),
	SAVE_ITEM(S5PC11X_MP07PUDPDN),
	SAVE_ITEM(S5PC11X_MP10CON),
	SAVE_ITEM(S5PC11X_MP10DAT),
	SAVE_ITEM(S5PC11X_MP10PUD),
	SAVE_ITEM(S5PC11X_MP10DRV),
	SAVE_ITEM(S5PC11X_MP10CONPDN),
	SAVE_ITEM(S5PC11X_MP10PUDPDN),
	SAVE_ITEM(S5PC11X_MP11CON),
	SAVE_ITEM(S5PC11X_MP11DAT),
	SAVE_ITEM(S5PC11X_MP11PUD),
	SAVE_ITEM(S5PC11X_MP11DRV),
	SAVE_ITEM(S5PC11X_MP11CONPDN),
	SAVE_ITEM(S5PC11X_MP11PUDPDN),
	SAVE_ITEM(S5PC11X_MP12CON),
	SAVE_ITEM(S5PC11X_MP12DAT),
	SAVE_ITEM(S5PC11X_MP12PUD),
	SAVE_ITEM(S5PC11X_MP12DRV),
	SAVE_ITEM(S5PC11X_MP12CONPDN),
	SAVE_ITEM(S5PC11X_MP12PUDPDN),
	SAVE_ITEM(S5PC11X_MP13CON),
	SAVE_ITEM(S5PC11X_MP13DAT),
	SAVE_ITEM(S5PC11X_MP13PUD),
	SAVE_ITEM(S5PC11X_MP13DRV),
	SAVE_ITEM(S5PC11X_MP13CONPDN),
	SAVE_ITEM(S5PC11X_MP13PUDPDN),
	SAVE_ITEM(S5PC11X_MP14CON),
	SAVE_ITEM(S5PC11X_MP14DAT),
	SAVE_ITEM(S5PC11X_MP14PUD),
	SAVE_ITEM(S5PC11X_MP14DRV),
	SAVE_ITEM(S5PC11X_MP14CONPDN),
	SAVE_ITEM(S5PC11X_MP14PUDPDN),
	SAVE_ITEM(S5PC11X_MP15CON),
	SAVE_ITEM(S5PC11X_MP15DAT),
	SAVE_ITEM(S5PC11X_MP15PUD),
	SAVE_ITEM(S5PC11X_MP15DRV),
	SAVE_ITEM(S5PC11X_MP15CONPDN),
	SAVE_ITEM(S5PC11X_MP15PUDPDN),
	SAVE_ITEM(S5PC11X_MP16CON),
	SAVE_ITEM(S5PC11X_MP16DAT),
	SAVE_ITEM(S5PC11X_MP16PUD),
	SAVE_ITEM(S5PC11X_MP16DRV),
	SAVE_ITEM(S5PC11X_MP16CONPDN),
	SAVE_ITEM(S5PC11X_MP16PUDPDN),
	SAVE_ITEM(S5PC11X_MP17CON),
	SAVE_ITEM(S5PC11X_MP17DAT),
	SAVE_ITEM(S5PC11X_MP17PUD),
	SAVE_ITEM(S5PC11X_MP17DRV),
	SAVE_ITEM(S5PC11X_MP17CONPDN),
	SAVE_ITEM(S5PC11X_MP17PUDPDN),
	SAVE_ITEM(S5PC11X_MP18CON),
	SAVE_ITEM(S5PC11X_MP18DAT),
	SAVE_ITEM(S5PC11X_MP18PUD),
	SAVE_ITEM(S5PC11X_MP18DRV),
	SAVE_ITEM(S5PC11X_MP18CONPDN),
	SAVE_ITEM(S5PC11X_MP18PUDPDN),
	SAVE_ITEM(S5PC11X_MP20CON),
	SAVE_ITEM(S5PC11X_MP20DAT),
	SAVE_ITEM(S5PC11X_MP20PUD),
	SAVE_ITEM(S5PC11X_MP20DRV),
	SAVE_ITEM(S5PC11X_MP20CONPDN),
	SAVE_ITEM(S5PC11X_MP20PUDPDN),
	SAVE_ITEM(S5PC11X_MP21CON),
	SAVE_ITEM(S5PC11X_MP21DAT),
	SAVE_ITEM(S5PC11X_MP21PUD),
	SAVE_ITEM(S5PC11X_MP21DRV),
	SAVE_ITEM(S5PC11X_MP21CONPDN),
	SAVE_ITEM(S5PC11X_MP21PUDPDN),
	SAVE_ITEM(S5PC11X_MP22CON),
	SAVE_ITEM(S5PC11X_MP22DAT),
	SAVE_ITEM(S5PC11X_MP22PUD),
	SAVE_ITEM(S5PC11X_MP22DRV),
	SAVE_ITEM(S5PC11X_MP22CONPDN),
	SAVE_ITEM(S5PC11X_MP22PUDPDN),
	SAVE_ITEM(S5PC11X_MP23CON),
	SAVE_ITEM(S5PC11X_MP23DAT),
	SAVE_ITEM(S5PC11X_MP23PUD),
	SAVE_ITEM(S5PC11X_MP23DRV),
	SAVE_ITEM(S5PC11X_MP23CONPDN),
	SAVE_ITEM(S5PC11X_MP23PUDPDN),
	SAVE_ITEM(S5PC11X_MP24CON),
	SAVE_ITEM(S5PC11X_MP24DAT),
	SAVE_ITEM(S5PC11X_MP24PUD),
	SAVE_ITEM(S5PC11X_MP24DRV),
	SAVE_ITEM(S5PC11X_MP24CONPDN),
	SAVE_ITEM(S5PC11X_MP24PUDPDN),
	SAVE_ITEM(S5PC11X_MP25CON),
	SAVE_ITEM(S5PC11X_MP25DAT),
	SAVE_ITEM(S5PC11X_MP25PUD),
	SAVE_ITEM(S5PC11X_MP25DRV),
	SAVE_ITEM(S5PC11X_MP25CONPDN),
	SAVE_ITEM(S5PC11X_MP25PUDPDN),
	SAVE_ITEM(S5PC11X_MP26CON),
	SAVE_ITEM(S5PC11X_MP26DAT),
	SAVE_ITEM(S5PC11X_MP26PUD),
	SAVE_ITEM(S5PC11X_MP26DRV),
	SAVE_ITEM(S5PC11X_MP26CONPDN),
	SAVE_ITEM(S5PC11X_MP26PUDPDN),
	SAVE_ITEM(S5PC11X_MP27CON),
	SAVE_ITEM(S5PC11X_MP27DAT),
	SAVE_ITEM(S5PC11X_MP27PUD),
	SAVE_ITEM(S5PC11X_MP27DRV),
	SAVE_ITEM(S5PC11X_MP27CONPDN),
	SAVE_ITEM(S5PC11X_MP27PUDPDN),
	SAVE_ITEM(S5PC11X_MP28CON),
	SAVE_ITEM(S5PC11X_MP28DAT),
	SAVE_ITEM(S5PC11X_MP28PUD),
	SAVE_ITEM(S5PC11X_MP28DRV),
	SAVE_ITEM(S5PC11X_MP28CONPDN),
	SAVE_ITEM(S5PC11X_MP28PUDPDN),
};

/* this lot should be really saved by the IRQ code */
/* VICXADDRESSXX initilaization to be needed */
static struct sleep_save irq_save[] = {
	SAVE_ITEM(S5PC110_VIC0REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC110_VIC1REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC110_VIC2REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC110_VIC3REG(VIC_INT_SELECT)),
	SAVE_ITEM(S5PC110_VIC0REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC110_VIC1REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC110_VIC2REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC110_VIC3REG(VIC_INT_ENABLE)),
	SAVE_ITEM(S5PC110_VIC0REG(VIC_INT_SOFT)),
	SAVE_ITEM(S5PC110_VIC1REG(VIC_INT_SOFT)),
	SAVE_ITEM(S5PC110_VIC2REG(VIC_INT_SOFT)),
	SAVE_ITEM(S5PC110_VIC3REG(VIC_INT_SOFT)),
};

static struct sleep_save sromc_save[] = {
	SAVE_ITEM(S5PC11X_SROM_BW),
	SAVE_ITEM(S5PC11X_SROM_BC0),
	SAVE_ITEM(S5PC11X_SROM_BC1),
	SAVE_ITEM(S5PC11X_SROM_BC2),
	SAVE_ITEM(S5PC11X_SROM_BC3),
	SAVE_ITEM(S5PC11X_SROM_BC4),
	SAVE_ITEM(S5PC11X_SROM_BC5),
};

static struct sleep_save uart_save[] = {
	SAVE_ITEM((S3C24XX_VA_UART0) + S3C2410_ULCON),
	SAVE_ITEM((S3C24XX_VA_UART0) + S3C2410_UCON),
	SAVE_ITEM((S3C24XX_VA_UART0) + S3C2410_UFCON),
	SAVE_ITEM((S3C24XX_VA_UART0) + S3C2410_UMCON),
	SAVE_ITEM((S3C24XX_VA_UART0) + S3C2410_UBRDIV),
	SAVE_ITEM((S3C24XX_VA_UART0) + S3C2410_UDIVSLOT),
	SAVE_ITEM((S3C24XX_VA_UART0) + S3C2410_UINTMSK),
};

#define DBG(fmt...)

#define s5pc11x_pm_debug_init() do { } while (0)
#define s5pc11x_pm_check_prepare() do { } while (0)
#define s5pc11x_pm_check_restore() do { } while (0)
#define s5pc11x_pm_check_store()   do { } while (0)

/* helper functions to save and restore register state */

void s5pc11x_pm_do_save(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		ptr->val = __raw_readl(ptr->reg);
		DBG("saved %p value %08lx\n", ptr->reg, ptr->val);
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
		printk(KERN_DEBUG "restore %p (restore %08lx, was %08x)\n",
				ptr->reg, ptr->val, __raw_readl(ptr->reg));

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

void s5pc11x_pm_do_save_phy(struct sleep_save_phy *ptr,
			struct platform_device *pdev, int count)
{
	void __iomem *target_reg;
	struct resource *res;
	u32 reg_size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_size = res->end - res->start + 1;
	target_reg = ioremap(res->start, reg_size);

	for (; count > 0; count--, ptr++)
		ptr->val = readl(target_reg + (ptr->reg));

}

/* s5pc11x_pm_do_restore_phy
 *
 * restore register of system
 *
 * Note, I made this function to support driver with ioremap.
 * If you want to use this function, you should to input as first parameter
 * struct sleep_save_phy type
*/

void s5pc11x_pm_do_restore_phy(struct sleep_save_phy *ptr,
			struct platform_device *pdev, int count)
{
	void __iomem *target_reg;
	struct resource *res;
	u32 reg_size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_size = res->end - res->start + 1;
	target_reg = ioremap(res->start, reg_size);

	for (; count > 0; count--, ptr++)
		writel(ptr->val, (target_reg + ptr->reg));

}

static void s5pc11x_pm_do_restore_core(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++)
		__raw_writel(ptr->val, ptr->reg);

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
		if ((which) & (1L<<i))
			DBG("IRQ %d asserted at resume\n", start+i);

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

#ifdef DEBUG_S5P_PM
	char buf[128];
#endif
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
#ifndef CONFIG_S5P_DEEP_IDLE_TEST
/* USB & OSC Clock pad should be disabled
* in C110 EVT0. If not, wakeup failure occures sometimes
*/
#if defined(CONFIG_CPU_S5PC110_EVT0_ERRATA)
	tmp = __raw_readl(S5P_SLEEP_CFG);
	tmp &= ~(S5P_SLEEP_CFG_OSC_EN | S5P_SLEEP_CFG_USBOSC_EN);
	__raw_writel(tmp , S5P_SLEEP_CFG);
#endif

#endif
	/* Power mode Config setting */
	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;

#ifdef CONFIG_S5P_DEEP_IDLE_TEST
	DBG("deep idle mode\n");
	tmp |= S5P_CFG_WFI_IDLE;
#else
	DBG("sleep mode\n");
	tmp |= S5P_CFG_WFI_SLEEP;
#endif
	__raw_writel(tmp, S5P_PWR_CFG);

#ifdef CONFIG_S5P_DEEP_IDLE_TEST
	/* IDLE config register set */
	/* TOP Memory retention off */
	/* TOP Memory LP mode       */
	/* ARM_L2_Cacheret on       */
	tmp = __raw_readl(S5P_IDLE_CFG);
	tmp &= ~(0x3f << 26);
	tmp |= ((1<<30) | (1<<28) | (1<<26) | (1<<0));
	__raw_writel(tmp, S5P_IDLE_CFG);
#endif

#ifdef CONFIG_S5P_DEEP_IDLE_TEST
	tmp = __raw_readl(S5P_WAKEUP_MASK);
	tmp &= ~((1 << 13) | (1 << 1));
	__raw_writel(tmp , S5P_WAKEUP_MASK);
#endif

	__raw_writel(0xffffffff, S5PC110_VIC0REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC1REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC2REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC3REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC0REG(VIC_INT_SOFT_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC1REG(VIC_INT_SOFT_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC2REG(VIC_INT_SOFT_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC3REG(VIC_INT_SOFT_CLEAR));

	/* Clear all EINT PENDING bit */
	__raw_writel(0xff, S5PC11X_VA_GPIO + 0xF40);
	tmp = __raw_readl(S5PC11X_VA_GPIO + 0xF40);
	__raw_writel(0xff, S5PC11X_VA_GPIO + 0xF44);
	tmp = __raw_readl(S5PC11X_VA_GPIO + 0xF44);
	__raw_writel(0xff, S5PC11X_VA_GPIO + 0xF48);
	tmp = __raw_readl(S5PC11X_VA_GPIO + 0xF48);
	__raw_writel(0xff, S5PC11X_VA_GPIO + 0xF4C);
	tmp = __raw_readl(S5PC11X_VA_GPIO + 0xF4C);

	__raw_writel(0xFFFF , S5P_WAKEUP_STAT);

	/* SYSC INT Disable */
	tmp = __raw_readl(S5P_OTHERS);
	tmp |= S5P_OTHER_SYSC_INTOFF;
	__raw_writel(tmp, S5P_OTHERS);

	/* s5pc11x_cpu_save will also act as our return point from when
	 * we resume as it saves its own register state, so use the return
	 * code to differentiate return from save and return from sleep */

#ifdef DEBUG_S5P_PM
	
	s5p_low_lvl_debug(buf, "WAKEUP_STAT", S5P_WAKEUP_STAT);
	s5p_low_lvl_debug(buf, "EINT_WAKEUP_MASK", S5P_EINT_WAKEUP_MASK);
	s5p_low_lvl_debug(buf, "WAKEUP_MASK", S5P_WAKEUP_MASK);
	s5p_low_lvl_debug(buf, "EINTPENDING0", (S5PC11X_VA_GPIO + 0xf40));
	s5p_low_lvl_debug(buf, "EINTPENDING1", (S5PC11X_VA_GPIO + 0xf44));
	s5p_low_lvl_debug(buf, "EINTPENDING2", (S5PC11X_VA_GPIO + 0xf48));
	s5p_low_lvl_debug(buf, "EINTPENDING3", (S5PC11X_VA_GPIO + 0xf4c));
	s5p_low_lvl_debug(buf, "GPH0CON", S5PC11X_GPH0CON);
	s5p_low_lvl_debug(buf, "GPH1CON", S5PC11X_GPH1CON);
	s5p_low_lvl_debug(buf, "GPH2CON", S5PC11X_GPH2CON);
	s5p_low_lvl_debug(buf, "GPH3CON", S5PC11X_GPH3CON);

	s5p_low_lvl_debug(buf, "EINT0CON", S5PC11X_EINT30CON);
	s5p_low_lvl_debug(buf, "EINT1CON", S5PC11X_EINT31CON);
	s5p_low_lvl_debug(buf, "EINT2CON", S5PC11X_EINT32CON);
	s5p_low_lvl_debug(buf, "EINT3CON", S5PC11X_EINT33CON);

	s5p_low_lvl_debug(buf, "EINT0MSK", S5PC11X_EINT30MASK);
	s5p_low_lvl_debug(buf, "EINT1MSK", S5PC11X_EINT31MASK);
	s5p_low_lvl_debug(buf, "EINT2MSK", S5PC11X_EINT32MASK);
	s5p_low_lvl_debug(buf, "EINT3MSK", S5PC11X_EINT33MASK);
	
	s5p_low_lvl_debug(buf, "EINT00FLTCON", S5PC11X_EINT30FLTCON0);
	s5p_low_lvl_debug(buf, "EINT01FLTCON", S5PC11X_EINT30FLTCON1);
	s5p_low_lvl_debug(buf, "EINT10FLTCON", S5PC11X_EINT31FLTCON0);
	s5p_low_lvl_debug(buf, "EINT11FLTCON", S5PC11X_EINT31FLTCON1);

	s5p_low_lvl_debug(buf, "EINT20FLTCON", S5PC11X_EINT32FLTCON0);
	s5p_low_lvl_debug(buf, "EINT21FLTCON", S5PC11X_EINT32FLTCON1);
	s5p_low_lvl_debug(buf, "EINT31FLTCON", S5PC11X_EINT33FLTCON0);
	s5p_low_lvl_debug(buf, "EINT32FLTCON", S5PC11X_EINT33FLTCON1);


	s5p_low_lvl_debug(buf, "VIC0IRQSTATUS", S5PC110_VIC0REG(VIC_IRQ_STATUS));
	s5p_low_lvl_debug(buf, "VIC1IRQSTATUS", S5PC110_VIC1REG(VIC_IRQ_STATUS));
	s5p_low_lvl_debug(buf, "VIC2IRQSTATUS", S5PC110_VIC2REG(VIC_IRQ_STATUS));
	s5p_low_lvl_debug(buf, "VIC3IRQSTATUS", S5PC110_VIC3REG(VIC_IRQ_STATUS));
	s5p_low_lvl_debug(buf, "VIC0RAWSTATUS", S5PC110_VIC0REG(VIC_RAW_STATUS));
	s5p_low_lvl_debug(buf, "VIC1RAWSTATUS", S5PC110_VIC1REG(VIC_RAW_STATUS));
	s5p_low_lvl_debug(buf, "VIC2RAWSTATUS", S5PC110_VIC2REG(VIC_RAW_STATUS));
	s5p_low_lvl_debug(buf, "VIC3RAWSTATUS", S5PC110_VIC3REG(VIC_RAW_STATUS));

#endif

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

#ifdef CONFIG_REGULATOR
static int s5pc11x_pm_begin(suspend_state_t state)
{
	return regulator_suspend_prepare(state);
}
#endif

static struct platform_suspend_ops s5pc11x_pm_ops = {
#ifdef CONFIG_REGULATOR
	.begin		= s5pc11x_pm_begin,
#endif
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
	u32 tmp;

	printk(KERN_INFO "s5pc11x Power Management, (c) 2008 Samsung Electronics\n");

	tmp = __raw_readl(S5P_CLK_OUT);
	tmp |= (0xf << 12);
	__raw_writel(tmp , S5P_CLK_OUT);

	__raw_writel(0xffff, S5P_WAKEUP_MASK);

	/* set the irq configuration for wake */
	suspend_set_ops(&s5pc11x_pm_ops);
	return 0;
}
