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

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-clock.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>

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

int s5pc11x_clk_main0_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_MAIN0, clk, enable);
}

int s5pc11x_clk_main1_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_MAIN1, clk, enable);
}

int s5pc11x_clk_main2_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_MAIN2, clk, enable);
}

int s5pc11x_clk_peri0_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_PERI0, clk, enable);
}

int s5pc11x_clk_peri1_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_PERI1, clk, enable);
}

int s5pc11x_clk_sclk0_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_SCLK0, clk, enable);
}

int s5pc11x_clk_sclk1_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_SCLK1, clk, enable);
}

int s5pc11x_sclk0_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_SCLKGATE0, clk, enable);
}

int s5pc11x_sclk1_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_SCLKGATE1, clk, enable);
}

int s5pc11x_clk_ip0_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_IP0, clk, enable);
}

int s5pc11x_clk_ip1_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_IP1, clk, enable);
}

int s5pc11x_clk_ip2_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_IP2, clk, enable);
}

int s5pc11x_clk_ip3_ctrl(struct clk *clk, int enable)
{
	return s5pc11x_clk_gate(S5P_CLKGATE_IP3, clk, enable);
}

int s5pc11x_clk_ip4_ctrl(struct clk *clk, int enable)
{
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
	{
		.name           = "otg",
		.id             = -1,
		.parent         = &clk_h133,
		.enable         = s5pc11x_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_USBOTG,
	},
};

static struct clk init_clocks[] = {
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
		.name		= "lcd",
		.id		= -1,
		.parent		= &clk_h166,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_FIMD,
	}, {
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
        }, {
                .name           = "cfcon",
                .id             = 0,
                .parent         = &clk_h133,
                .enable         = s5pc11x_clk_ip1_ctrl,
                .ctrlbit        = S5P_CLKGATE_IP1_CFCON,
        }, {
		.name		= "timers",
		.id		= -1,
		.parent		= &clk_p66,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PWM,
	},
};

static struct clk *clks[] __initdata = {
	&clk_ext,
	&clk_epll,
	&clk_27m,
};

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
