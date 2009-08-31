/* linux/arch/arm/plat-s5pc11x/clock.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5PC1XX Base clock support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-clock.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>

static int powerdomain_set(struct powerdomain *pd, int enable)
{
	unsigned long ctrlbit = pd->pd_ctrlbit;
	void __iomem *reg = (void __iomem *)(pd->pd_reg);
	void __iomem *stable_reg = (void __iomem *)(pd->pd_stable_reg);
	unsigned long reg_dat;

	if (IS_ERR(pd) || pd == NULL)
		return -EINVAL;

	reg_dat = __raw_readl(reg);

	if (enable) {
		__raw_writel(reg_dat|ctrlbit, reg);
		do {

		} while(!(__raw_readl(stable_reg)&ctrlbit));

	} else {
		__raw_writel(reg_dat &~(ctrlbit), reg);
	}

	return 0;
}

static struct powerdomain pd_lcd = {
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<3),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

static struct powerdomain pd_tv = {
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<4),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

static struct powerdomain pd_mfc = {
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<1),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

static struct powerdomain pd_cam = {
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<5),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

static struct powerdomain pd_audio = {
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<7),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

static struct powerdomain pd_irom = {
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<20),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

struct clk clk_27m = {
	.name		= "clk_27m",
	.id		= -1,
	.rate		= 27000000,
};

static int inline s5pc11x_clk_gate(void __iomem *reg,
				struct clk *clk,
				int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	u32 con;

	con = __raw_readl(reg);

	if (enable)
		con |= ctrlbit;
	else
		con &= ~ctrlbit;

	__raw_writel(con, reg);
	return 0;
}


int s5pc11x_clk_ip0_ctrl(struct clk *clk, int enable)
{
	if ((clk->ctrlbit)&(S5P_CLKGATE_IP0_RESERVED|S5P_CLKGATE_IP0_ALWAYS_ON))
		return 0;

	return s5pc11x_clk_gate(S5P_CLKGATE_IP0, clk, enable);
}

int s5pc11x_clk_ip1_ctrl(struct clk *clk, int enable)
{
	if ((clk->ctrlbit)&(S5P_CLKGATE_IP1_RESERVED|S5P_CLKGATE_IP1_ALWAYS_ON))
		return 0;

	return s5pc11x_clk_gate(S5P_CLKGATE_IP1, clk, enable);
}

int s5pc11x_clk_ip2_ctrl(struct clk *clk, int enable)
{
	if ((clk->ctrlbit)&(S5P_CLKGATE_IP2_RESERVED|S5P_CLKGATE_IP2_ALWAYS_ON))
		return 0;

	return s5pc11x_clk_gate(S5P_CLKGATE_IP2, clk, enable);
}

int s5pc11x_clk_ip3_ctrl(struct clk *clk, int enable)
{
	if ((clk->ctrlbit)&(S5P_CLKGATE_IP3_RESERVED|S5P_CLKGATE_IP3_ALWAYS_ON))
		return 0;

	return s5pc11x_clk_gate(S5P_CLKGATE_IP3, clk, enable);
}

int s5pc11x_clk_ip4_ctrl(struct clk *clk, int enable)
{
	if ((clk->ctrlbit)&(S5P_CLKGATE_IP4_RESERVED|S5P_CLKGATE_IP4_ALWAYS_ON))
		return 0;

	return s5pc11x_clk_gate(S5P_CLKGATE_IP4, clk, enable);
}

int s5pc11x_clk_block_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_BLOCK, clk, enable);
}

int s5pc11x_clk_bus0_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_BUS0, clk, enable);
}

int s5pc11x_clk_bus1_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_BUS1, clk, enable);
}

static struct clk init_clocks_disable[] = {

};

static struct clk init_clocks[] = {
	/* Multimedia */
	{
		.name           = "fimc",
		.id             = 0,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC0,
		.pd		= &pd_cam,
	}, {
		.name           = "fimc",
		.id             = 1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC1,
		.pd		= &pd_cam,
	}, {
		.name           = "fimc",
		.id             = 2,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC2,
		.pd		= &pd_cam,
	}, {
		.name           = "mfc",
		.id             = -1,
		.parent         = &clk_h200,
		.enable         = s5pc11x_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_MFC,
		.pd		= &pd_mfc,
	}, {
		.name           = "jpeg",
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_JPEG,
		.pd		= &pd_cam,
	}, {
		.name           = "rotator",
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_ROTATOR,
		.pd		= &pd_cam,
	}, {
		.name           = "ipc",
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_IPC,
		.pd		= &pd_cam,
	}, {
		.name           = "csis",
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_CSIS,
		.pd		= &pd_cam,
	}, {
		.name           = "g3d",
		.id             = -1,
		.parent         = &clk_h200,
		.enable         = s5pc11x_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_G3D,
	},

	/* Connectivity and Multimedia */
	{
		.name           = "otg",
		.id             = -1,
		.parent         = &clk_h133,
		.enable         = s5pc11x_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_USBOTG,
	}, {
		.name           = "usb-host",
		.id             = -1,
		.parent         = &clk_h133,
		.enable         = s5pc11x_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_USBHOST,
	}, {
		.name           = "hdmi",
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_HDMI,
		.pd		= &pd_tv,
	}, {
		.name           = "tvenc",
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_TVENC,
		.pd		= &pd_tv,
	}, {
		.name           = "mixer",
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_MIXER,
		.pd		= &pd_tv,
	}, {
		.name           = "vp",
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_VP,
		.pd		= &pd_tv,
	}, {
		.name           = "dsim",
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_DSIM,
		.pd		= &pd_lcd,
	}, {
		.name           = "mie",
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_MIE,
		.pd		= &pd_lcd,
	}, {
		.name           = "lcd",	// fimd ?
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_FIMD,
		.pd		= &pd_lcd,
	},  {
                .name           = "cfcon",
                .id             = 0,
                .parent         = &clk_h133,
                .enable         = s5pc11x_clk_ip1_ctrl,
                .ctrlbit        = S5P_CLKGATE_IP1_CFCON,
        },  {
                .name           = "nandxl",
                .id             = 0,
                .parent         = &clk_h133,
                .enable         = s5pc11x_clk_ip1_ctrl,
                .ctrlbit        = S5P_CLKGATE_IP1_NANDXL,
        },

	/* Connectivity */
	{
		.name		= "hsmmc",
		.id		= 0,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC0,
	}, {
		.name		= "hsmmc",
		.id		= 1,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC1,
	}, {
		.name		= "hsmmc",
		.id		= 2,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC2,
	}, {
		.name		= "hsmmc",
		.id		= 3,
		.parent		= &clk_h133,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC3,
        },

	/* Peripherals */
	{
		.name		= "systimer",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_SYSTIMER,
	}, {
		.name		= "watchdog",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_WDT,
	}, {
		.name		= "rtc",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_RTC,
	}, {
		.name		= "uart",
		.id		= 0,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART0,
	}, {
		.name		= "uart",
		.id		= 1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART1,
	}, {
		.name		= "uart",
		.id		= 2,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART2,
	}, {
		.name		= "uart",
		.id		= 3,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART3,
	}, {
		.name		= "i2c",
		.id		= 0,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C0,
	}, {
		.name		= "i2c",
		.id		= 1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C1,
	}, {
		.name		= "i2c",
		.id		= 2,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C2,
	}, {
                .name           = "spi",
                .id             = 0,
                .parent         = &clk_p66,
                .enable         = s5pc11x_clk_ip3_ctrl,
                .ctrlbit        = S5P_CLKGATE_IP3_SPI0,
        }, {
                .name           = "spi",
                .id             = 1,
                .parent         = &clk_p66,
                .enable         = s5pc11x_clk_ip3_ctrl,
                .ctrlbit        = S5P_CLKGATE_IP3_SPI1,
        }, {
                .name           = "spi",
                .id             = 2,
                .parent         = &clk_p66,
                .enable         = s5pc11x_clk_ip3_ctrl,
                .ctrlbit        = S5P_CLKGATE_IP3_SPI2,
        }, {
		.name		= "timers",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PWM,
	}, {
		.name		= "adc",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_TSADC,
	}, {
		.name		= "keypad",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_KEYIF,
	},

	/* Audio (D1_5) devices */
	{
		.name		= "i2s_v50",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S0, /* I2S0 is v5.0 */
	}, {
		.name		= "i2s_v32",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S0, /* I2S1 is v3.2 */
	}, {
		.name		= "i2s_v32",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S0, /* I2S2 is v3.2 */
	},




};

static struct clk *clks[] __initdata = {
	&clk_ext,
	&clk_epll,
	&clk_27m,
};

/* Disable all IP's clock and MM power domain. This will decrease power
 * consumption in normal mode.
 * In kernel booting sequence, basically disable all IP's clock and MM power domain.
 * If D/D uses specific clock, use clock API.
 */
void s5pc11x_init_clocks_power_disabled(void)
{
	struct clk clk_dummy;
	unsigned long shift = 0;

	/* Disable all clock except essential clock */
	do {
		clk_dummy.ctrlbit = (1<<shift);
		s5pc11x_clk_ip0_ctrl(&clk_dummy, 0);
		s5pc11x_clk_ip1_ctrl(&clk_dummy, 0);
		s5pc11x_clk_ip2_ctrl(&clk_dummy, 0);
		s5pc11x_clk_ip3_ctrl(&clk_dummy, 0);
		s5pc11x_clk_ip4_ctrl(&clk_dummy, 0);

		shift ++;

	} while (shift < 32);

	/* Disable all power domain */
	powerdomain_set(&pd_lcd, 0);
	powerdomain_set(&pd_tv, 1);
	powerdomain_set(&pd_mfc, 0);
	powerdomain_set(&pd_cam, 0);
	powerdomain_set(&pd_audio, 0);

}

void __init s5pc11x_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	s5pc11x_init_clocks_power_disabled();

	s3c24xx_register_clocks(clks, ARRAY_SIZE(clks));

	clkp = init_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks); ptr++, clkp++) {
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}

	clkp = init_clocks_disable;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks_disable); ptr++, clkp++) {

		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}

		(clkp->enable)(clkp, 0);
	}
}
