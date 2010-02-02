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
#include <plat/regs-audss.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>

static int powerdomain_set(struct powerdomain *pd, int enable)
{
	unsigned long ctrlbit;
	void __iomem *reg;
	void __iomem *stable_reg;
	unsigned long reg_dat;

	if (pd == NULL)
		return -EINVAL;

	ctrlbit = pd->pd_ctrlbit;
	reg = (void __iomem *)pd->pd_reg;
	stable_reg = (void __iomem *)pd->pd_stable_reg;
	
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
	.nr_clks	= 0,
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<3),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

static struct powerdomain pd_tv = {
	.nr_clks	= 0,
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<4),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

static struct powerdomain pd_mfc = {
	.nr_clks	= 0,
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<1),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

static struct powerdomain pd_cam = {
	.nr_clks	= 0,
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<5),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

/* No way to set .pd in s5pc110-clock.c */
struct powerdomain pd_audio = {
	.nr_clks	= 0,
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<7),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

static struct powerdomain pd_irom = {
	.nr_clks	= 0,
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<20),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

static struct powerdomain pd_g3d = {
	.nr_clks	= 0,
	.pd_reg		= S5P_NORMAL_CFG,
	.pd_stable_reg	= S5P_BLK_PWR_STAT,
	.pd_ctrlbit	= (0x1<<2),
	.ref_count	= 0,
	.pd_set		= powerdomain_set,
};

static void s5pc11x_register_clks_on_pd(struct clk *clks)
{

	if (clks->pd != NULL) {
		clks->pd->pd_clks[clks->pd->nr_clks] = clks;
		clks->pd->nr_clks++;
		
	}
	
}

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

int s5pc11x_audss_clkctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_AUDSS, clk, enable);
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
		.name           = "mipi-csis",
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
		.pd		= &pd_g3d,
	}, {
		.name           = "g2d",
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_G2D,
		.pd		= &pd_lcd,
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
		.name           = "fimd",
		.id             = -1,
		.parent         = &clk_h166,
		.enable         = s5pc11x_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_FIMD,
		.pd		= &pd_lcd,
	}, {
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
		.ctrlbit	= S5P_CLKGATE_IP3_I2C_HDMI_DDC,
	}, {
		.name		= "i2c",
		.id		= 2,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C2,
	}, {
		.name		= "i2c-hdmiphy",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C_HDMI_PHY,
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
		.name		= "iis",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S0, /* I2S0 is v5.0 */
	}, {
		.name		= "iis",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S1, /* I2S1 is v3.2 */
	}, {
		.name		= "iis",
		.id		= 2,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S2, /* I2S2 is v3.2 */
	}, {
		.name		= "ac97",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_AC97,
	},
	/* PCM device */
	{
		.name		= "pcm",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PCM0,
	}, {
		.name		= "pcm",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PCM1,
	},

	/* SPDIF device */
	{
		.name		= "spdif",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_SPDIF,
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

/* Disable all power domain
 * FIXME : Do not turn off power domain in case of S5PC110 EVT0.
 *	This code should be fixed after revision.
 */
	powerdomain_set(&pd_lcd, 0);
	powerdomain_set(&pd_tv, 0);
	powerdomain_set(&pd_mfc, 1);
	powerdomain_set(&pd_cam, 0);
	powerdomain_set(&pd_audio, 1);
	powerdomain_set(&pd_g3d, 1);
}

void __init s5pc11x_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	s3c24xx_register_clocks(clks, ARRAY_SIZE(clks));

	clkp = init_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks); ptr++, clkp++) {
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
		s5pc11x_register_clks_on_pd(clkp);
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

	s5pc11x_init_clocks_power_disabled();

}
