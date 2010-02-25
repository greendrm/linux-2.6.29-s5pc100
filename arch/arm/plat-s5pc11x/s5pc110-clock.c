/* linux/arch/arm/plat-s5pc11x/s5pc100-clock.c
 *
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/sysdev.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/cpu-freq.h>

#include <plat/regs-clock.h>
#include <plat/regs-audss.h>
#include <plat/clock.h>
#include <plat/cpu.h>
#include <plat/pll.h>

/* For S5PC100 EVT0 workaround
 * When we modify DIVarm value to change ARM speed D0_BUS parent clock is also changed
 * If we prevent from unwanted changing of bus clock, we should modify DIVd0_bus value also.
 */
#define PREVENT_BUS_CLOCK_CHANGE

extern void ChangeClkDiv0(unsigned int val);

/* fin_apll, fin_mpll and fin_epll are all the same clock, which we call
 * ext_xtal_mux for want of an actual name from the manual.
*/
static unsigned long s5pc11x_roundrate_clksrc(struct clk *clk, unsigned long rate);

/* xtat_rate
 * Only used in this code.
 */ 
static unsigned long xtal_rate;

struct clk clk_ext_xtal_mux = {
	.name		= "ext_xtal",
	.id		= -1,
};

struct clk clk_ext_xtal_usb = {
	.name		= "XusbXTI",
	.id		= -1,
	.rate		= 24000000,
};

struct clk clk_ext_xtal_rtc = {
	.name		= "XrtcXTI",
	.id		= -1,
	.rate		= 32768,
};

#define clk_fin_apll	clk_ext_xtal_mux
#define clk_fin_mpll	clk_ext_xtal_mux
#define clk_fin_epll	clk_ext_xtal_mux
#define clk_fin_vpll	clk_ext_xtal_mux

#define clk_fout_mpll	clk_mpll

struct clk_sources {
	unsigned int	nr_sources;
	struct clk	**sources;
};

struct clksrc_clk {
	struct clk		clk;
	unsigned int		mask;
	unsigned int		shift;

	struct clk_sources	*sources;

	unsigned int		divider_shift;
	void __iomem		*reg_divider;
	void __iomem		*reg_source;
};

struct clk clk_srclk = {
	.name		= "srclk",
	.id		= -1,
};

struct clk clk_fout_apll = {
	.name		= "fout_apll",
	.id		= -1,
};

static struct clk *clk_src_apll_list[] = {
	[0] = &clk_fin_apll,
	[1] = &clk_fout_apll,
};

static struct clk_sources clk_src_apll = {
	.sources	= clk_src_apll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_apll_list),
};

static unsigned long s5pc11x_clk_moutapll_get_rate(struct clk *clk)
{
	unsigned long rate;

	rate = s5pc11x_get_pll(xtal_rate, __raw_readl(S5P_APLL_CON),
						S5PC11X_PLL_APLL);

	return rate;
}

struct clksrc_clk clk_mout_apll = {
	.clk	= {
		.name		= "mout_apll",
		.id		= -1,
		.get_rate	= s5pc11x_clk_moutapll_get_rate,
	},
	.shift		= S5P_CLKSRC0_APLL_SHIFT,
	.mask		= S5P_CLKSRC0_APLL_MASK,
	.sources	= &clk_src_apll,
	.reg_source	= S5P_CLK_SRC0,
};


static unsigned long s5pc11x_clk_doutapll_get_rate(struct clk *clk)
{
  	unsigned long rate = clk_get_rate(clk->parent);

	rate /= (((__raw_readl(S5P_CLK_DIV0) & S5P_CLKDIV0_APLL_MASK) >> S5P_CLKDIV0_APLL_SHIFT) + 1);

	return rate;
}

int s5pc11x_clk_doutapll_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk *temp_clk = clk;
	unsigned int div;
	u32 val;

	rate = clk_round_rate(temp_clk, rate);
	div = clk_get_rate(temp_clk->parent) / rate;

	val = __raw_readl(S5P_CLK_DIV0);
	val &=~ S5P_CLKDIV0_APLL_MASK;
	val |= (div - 1) << S5P_CLKDIV0_APLL_SHIFT;
	__raw_writel(val, S5P_CLK_DIV0);

	return 0;
}

static unsigned long s5pc11x_doutapll_roundrate(struct clk *clk,
					      unsigned long rate)
{
	unsigned long parent_rate = clk_get_rate(clk->parent);
	int div;

	if (rate > parent_rate)
		rate = parent_rate;
	else {
		div = parent_rate / rate;

		div ++;
		
		rate = parent_rate / div;
	}

	return rate;
}


struct clk clk_dout_apll = {
	.name = "dout_apll",
	.id = -1,
	.parent = &clk_mout_apll.clk,
	.get_rate = s5pc11x_clk_doutapll_get_rate,
	.set_rate = s5pc11x_clk_doutapll_set_rate,
	.round_rate = s5pc11x_doutapll_roundrate,
};

static unsigned long s5pc11x_clk_douta2m_get_rate(struct clk *clk)
{
  	unsigned long rate = clk_get_rate(clk->parent);

	rate /= (((__raw_readl(S5P_CLK_DIV0) & S5P_CLKDIV0_A2M_MASK) >> S5P_CLKDIV0_A2M_SHIFT) + 1);

	return rate;
}

int s5pc11x_clk_douta2m_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk *temp_clk = clk;
	unsigned int div;
	u32 val;

	rate = clk_round_rate(temp_clk, rate);
	div = clk_get_rate(temp_clk->parent) / rate;

	val = __raw_readl(S5P_CLK_DIV0);
	val &=~ S5P_CLKDIV0_A2M_MASK;
	val |= (div - 1) << S5P_CLKDIV0_A2M_SHIFT;
	__raw_writel(val, S5P_CLK_DIV0);

	return 0;
}

static unsigned long s5pc11x_douta2m_roundrate(struct clk *clk,
					      unsigned long rate)
{
	unsigned long parent_rate = clk_get_rate(clk->parent);
	int div;

	if (rate > parent_rate)
		rate = parent_rate;
	else {
		div = parent_rate / rate;

		div ++;
		
		rate = parent_rate / div;
	}

	return rate;
}

struct clk clk_dout_a2m = {
	.name = "dout_a2m",
	.id = -1,
	.parent = &clk_mout_apll.clk,
	.get_rate = s5pc11x_clk_douta2m_get_rate,
	.set_rate = s5pc11x_clk_douta2m_set_rate,
	.round_rate = s5pc11x_douta2m_roundrate,
};


static int fout_enable(struct clk *clk, int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	unsigned int epll_con = __raw_readl(S5P_EPLL_CON) & ~ ctrlbit;

	if(enable)
	   __raw_writel(epll_con | ctrlbit, S5P_EPLL_CON);
	else
	   __raw_writel(epll_con, S5P_EPLL_CON);

	return 0;
}

static unsigned long fout_get_rate(struct clk *clk)
{
	return clk->rate;
}

static int fout_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int epll_con;

#if defined(CONFIG_CPU_S5PC110_EVT1)
	unsigned int epll_con1;
#endif

	epll_con = __raw_readl(S5P_EPLL_CON);

	epll_con &= ~S5P_EPLLVAL(0x1, 0x1ff, 0x3f, 0x7); /* Clear V, M, P & S */

#if defined(CONFIG_CPU_S5PC110_EVT1)
	epll_con1 = __raw_readl(S5P_EPLL_CON1);

	switch (rate) {
	case 48000000:
		epll_con |= S5P_EPLLVAL(0, 48, 3, 3);
		break;
	case 96000000:
		epll_con |= S5P_EPLLVAL(0, 48, 3, 2);
		break;
	case 144000000:
		epll_con |= S5P_EPLLVAL(1, 72, 3, 2);
		break;
	case 192000000:
		epll_con |= S5P_EPLLVAL(0, 48, 3, 1);
		break;
	case 288000000:
		epll_con |= S5P_EPLLVAL(1, 72, 3, 1);
		break;
	case 32750000:
	case 32768000:
		epll_con |= S5P_EPLLVAL(1, 65, 3, 4);
		epll_con1 = S5P_EPLL_CON1_MASK & 35127;
		break;
	case 45158400:
	case 45000000:
	case 45158000:
		epll_con |= S5P_EPLLVAL(0, 45, 3, 3);
		epll_con1 = S5P_EPLL_CON1_MASK & 10355;
		break;
	case 49125000:
	case 49152000:
		epll_con |= S5P_EPLLVAL(0, 49, 3, 3);
		epll_con1 = S5P_EPLL_CON1_MASK & 9961;
		break;
	case 67737600:
	case 67738000:
		epll_con |= S5P_EPLLVAL(1, 67, 3, 3);
		epll_con1 = S5P_EPLL_CON1_MASK & 48366;
		break;
	case 73800000:
	case 73728000:
		epll_con |= S5P_EPLLVAL(1, 73, 3, 3);
		epll_con1 = S5P_EPLL_CON1_MASK & 47710;
		break;
	case 36000000:
		epll_con |= S5P_EPLLVAL(1, 72, 3, 4);
		break;
	case 60000000:
		epll_con |= S5P_EPLLVAL(1, 60, 3, 3);
		break;
	case 72000000:
		epll_con |= S5P_EPLLVAL(1, 72, 3, 3);
		break;
	case 80000000:
		epll_con |= S5P_EPLLVAL(1, 80, 3, 3);
		break;
	case 84000000:
		epll_con |= S5P_EPLLVAL(0, 42, 3, 2);
		break;
	case 50000000:
		epll_con |= S5P_EPLLVAL(0, 50, 3, 3);
		break;
	default:
		printk(KERN_ERR "Invalid Clock Freq!\n");
		return -EINVAL;
	}

	__raw_writel(epll_con1, S5P_EPLL_CON1);
#else
	switch (rate) {
	case 48000000:
			epll_con |= S5P_EPLLVAL(0, 96, 6, 3);
			break;
	case 96000000:
			epll_con |= S5P_EPLLVAL(0, 96, 6, 2);
			break;
	case 144000000:
			epll_con |= S5P_EPLLVAL(1, 144, 6, 2);
			break;
	case 192000000:
			epll_con |= S5P_EPLLVAL(0, 96, 6, 1);
			break;
	case 288000000:
			epll_con |= S5P_EPLLVAL(1, 144, 6, 1);
			break;
	case 32750000:
	case 32768000:
			epll_con |= S5P_EPLLVAL(1, 131, 6, 4);
			break;
	case 45000000:
	case 45158000:
			epll_con |= S5P_EPLLVAL(0, 271, 18, 3);
			break;
	case 49125000:
	case 49152000:
			epll_con |= S5P_EPLLVAL(0, 131, 8, 3);
			break;
	case 67737600:
	case 67738000:
			epll_con |= S5P_EPLLVAL(1, 271, 12, 3);
			break;
	case 73800000:
	case 73728000:
			epll_con |= S5P_EPLLVAL(1, 295, 12, 3);
			break;
	case 36000000:
			epll_con |= S5P_EPLLVAL(0, 72, 6, 3); /* 36M = 0.75*48M */
			break;
	case 60000000:
			epll_con |= S5P_EPLLVAL(0, 120, 6, 3); /* 60M = 1.25*48M */
			break;
	case 72000000:
			epll_con |= S5P_EPLLVAL(0, 144, 6, 3); /* 72M = 1.5*48M */
			break;
	case 80000000:
			epll_con |= S5P_EPLLVAL(1, 160, 6, 3);
			break;
	case 84000000:
			epll_con |= S5P_EPLLVAL(0, 84, 6, 2);
			break;
	case 50000000:
			epll_con |= S5P_EPLLVAL(0, 100, 6, 3);
			break;
	default:
			printk(KERN_ERR "Invalid Clock Freq!\n");
			return -EINVAL;
	}
#endif

	__raw_writel(epll_con, S5P_EPLL_CON);

	clk->rate = rate;

	return 0;
}

struct clk clk_fout_epll = {
	.name		= "fout_epll",
	.id		= -1,
	.ctrlbit	= S5P_EPLL_EN,
	.enable		= fout_enable,
	.get_rate	= fout_get_rate,
	.set_rate	= fout_set_rate,
};

static struct clk *clk_src_epll_list[] = {
	[0] = &clk_fin_epll,
	[1] = &clk_fout_epll,
};

static struct clk_sources clk_src_epll = {
	.sources	= clk_src_epll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_epll_list),
};

struct clksrc_clk clk_mout_epll = {
	.clk	= {
		.name		= "mout_epll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_EPLL_SHIFT,
	.mask		= S5P_CLKSRC0_EPLL_MASK,
	.sources	= &clk_src_epll,
	.reg_source	= S5P_CLK_SRC0,
};

static struct clk *clk_src_vpll_list[] = {
	[0] = &clk_27m,
	[1] = &clk_srclk,
};

static struct clk_sources clk_src_vpll = {
	.sources	= clk_src_vpll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_vpll_list),
};

struct clksrc_clk clk_mout_vpll = {
	.clk	= {
		.name		= "mout_vpll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_VPLL_SHIFT,
	.mask		= S5P_CLKSRC0_VPLL_MASK,
	.sources	= &clk_src_vpll,
	.reg_source	= S5P_CLK_SRC0,
};

static struct clk *clk_src_mpll_list[] = {
	[0] = &clk_fin_mpll,
	[1] = &clk_fout_mpll,
};

static struct clk_sources clk_src_mpll = {
	.sources	= clk_src_mpll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_mpll_list),
};

struct clksrc_clk clk_mout_mpll = {
	.clk = {
		.name		= "mout_mpll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_MPLL_SHIFT,
	.mask		= S5P_CLKSRC0_MPLL_MASK,
	.sources	= &clk_src_mpll,
	.reg_source	= S5P_CLK_SRC0,
};

static unsigned long s5pc11x_clk_doutmpll_get_rate(struct clk *clk)
{
	unsigned long rate = clk_get_rate(clk->parent);

	return rate;
}

struct clk clk_dout_ck166 = {
	.name		= "dout_ck166",
	.id		= -1,
	.parent		= &clk_mout_mpll.clk,
	.get_rate	= s5pc11x_clk_doutmpll_get_rate,
};

static unsigned long s5pc11x_clk_sclk_hdmi_get_rate(struct clk *clk)
{
	unsigned long rate = clk_get_rate(clk->parent);

	return rate;
}

struct clk clk_sclk_hdmi = {
	.name		= "sclk_hdmi",
	.id		= -1,
	.parent		= &clk_mout_vpll.clk,
	.get_rate	= s5pc11x_clk_sclk_hdmi_get_rate,
};

static struct clk *clkset_spi_list[] = {
	&clk_srclk,	/*Dummy clock sources. To be changed when the */
	&clk_srclk,	/*actual clock source gets defined*/
	&clk_sclk_hdmi,
	&clk_srclk,
	&clk_srclk,
	&clk_srclk,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk
};

static struct clk_sources clkset_spi = {
	.sources	= clkset_spi_list,
	.nr_sources	= ARRAY_SIZE(clkset_spi_list),
};

static struct clk *clkset_uart_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	NULL,
	NULL,
};

static struct clk_sources clkset_uart = {
	.sources	= clkset_uart_list,
	.nr_sources	= ARRAY_SIZE(clkset_uart_list),
};

static struct clk *clkset_mmc0_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,	
	&clk_mout_vpll.clk,
	&clk_fin_epll,
};

static struct clk *clkset_mmc1_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,	
	&clk_mout_vpll.clk,
	&clk_fin_epll,
};

static struct clk *clkset_mmc2_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,	
	&clk_mout_vpll.clk,
	&clk_fin_epll,
};

static struct clk *clkset_mmc3_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,	
	&clk_mout_vpll.clk,
	&clk_fin_epll,
};

static struct clk_sources clkset_mmc0 = {
	.sources	= clkset_mmc0_list,
	.nr_sources	= ARRAY_SIZE(clkset_mmc0_list),
};

static struct clk_sources clkset_mmc1 = {
	.sources	= clkset_mmc1_list,
	.nr_sources	= ARRAY_SIZE(clkset_mmc1_list),
};

static struct clk_sources clkset_mmc2 = {
	.sources	= clkset_mmc2_list,
	.nr_sources	= ARRAY_SIZE(clkset_mmc2_list),
};

static struct clk_sources clkset_mmc3 = {
	.sources	= clkset_mmc3_list,
	.nr_sources	= ARRAY_SIZE(clkset_mmc3_list),
};

static struct clk *clkset_fimd_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
};

static struct clk_sources clkset_fimd = {
	.sources	= clkset_fimd_list,
	.nr_sources	= ARRAY_SIZE(clkset_fimd_list),
};

static struct clk *clkset_cam0_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk *clkset_cam1_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_cam0 = {
	.sources	= clkset_cam0_list,
	.nr_sources	= ARRAY_SIZE(clkset_cam0_list),
};

static struct clk_sources clkset_cam1 = {
	.sources	= clkset_cam1_list,
	.nr_sources	= ARRAY_SIZE(clkset_cam0_list),
};

static struct clk *clkset_fimc0_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk *clkset_fimc1_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk *clkset_fimc2_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_fimc0 = {
	.sources	= clkset_fimc0_list,
	.nr_sources	= ARRAY_SIZE(clkset_fimc0_list),
};

static struct clk_sources clkset_fimc1 = {
	.sources	= clkset_fimc1_list,
	.nr_sources	= ARRAY_SIZE(clkset_fimc1_list),
};

static struct clk_sources clkset_fimc2 = {
	.sources	= clkset_fimc2_list,
	.nr_sources	= ARRAY_SIZE(clkset_fimc2_list),
};

static struct clk *clkset_pwi_list[] = {
	&clk_srclk,
	&clk_mout_epll.clk,
	&clk_mout_mpll.clk,
	NULL,
};

static struct clk_sources clkset_pwi = {
	.sources	= clkset_pwi_list,
	.nr_sources	= ARRAY_SIZE(clkset_pwi_list),
};

/* The peripheral clocks are all controlled via clocksource followed
 * by an optional divider and gate stage. We currently roll this into
 * one clock which hides the intermediate clock from the mux.
 *
 * Note, the JPEG clock can only be an even divider...
 *
 * The scaler and LCD clocks depend on the S3C64XX version, and also
 * have a common parent divisor so are not included here.
 */

static inline struct clksrc_clk *to_clksrc(struct clk *clk)
{
	return container_of(clk, struct clksrc_clk, clk);
}

static unsigned long s5pc11x_getrate_clksrc(struct clk *clk)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	unsigned long rate = clk_get_rate(clk->parent);
	u32 clkdiv = __raw_readl(sclk->reg_divider);

	clkdiv >>= sclk->divider_shift;
	clkdiv &= 0xf;
	clkdiv++;

	rate /= clkdiv;
	return rate;
}

static int s5pc11x_setrate_clksrc(struct clk *clk, unsigned long rate)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	void __iomem *reg = sclk->reg_divider;
	unsigned int div;
	u32 val;

	rate = clk_round_rate(clk, rate);
	div = clk_get_rate(clk->parent) / rate;

	val = __raw_readl(reg);
	val &= ~sclk->mask;
	val |= (div - 1) << sclk->divider_shift;
	__raw_writel(val, reg);

	return 0;
}

static int s5pc11x_setparent_clksrc(struct clk *clk, struct clk *parent)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	struct clk_sources *srcs = sclk->sources;
	u32 clksrc = __raw_readl(sclk->reg_source);
	int src_nr = -1;
	int ptr;

	for (ptr = 0; ptr < srcs->nr_sources; ptr++)
		if (srcs->sources[ptr] == parent) {
			src_nr = ptr;
			break;
		}

	if (src_nr >= 0) {
		clk->parent = parent;

		clksrc &= ~sclk->mask;
		clksrc |= src_nr << sclk->shift;

		__raw_writel(clksrc, sclk->reg_source);
		return 0;
	}

	return -EINVAL;
}

static unsigned long s5pc11x_roundrate_clksrc(struct clk *clk,
					      unsigned long rate)
{
	unsigned long parent_rate = clk_get_rate(clk->parent);
	int div;

	if (rate >= parent_rate)
		rate = parent_rate;
	else {
		div = parent_rate / rate;
		if(parent_rate % rate)
			div++;

		if (div == 0)
			div = 1;
		if (div > 16)
			div = 16;

		rate = parent_rate / div;
	}

	return rate;
}

static struct clksrc_clk clk_mmc0 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 0,
		.ctrlbit        = S5P_CLKGATE_IP2_HSMMC0,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_MMC0_SHIFT,
	.mask		= S5P_CLKSRC4_MMC0_MASK,
	.sources	= &clkset_mmc0,
	.divider_shift	= S5P_CLKDIV4_MMC0_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_mmc1 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 1,
		.ctrlbit        = S5P_CLKGATE_IP2_HSMMC1,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_MMC1_SHIFT,
	.mask		= S5P_CLKSRC4_MMC1_MASK,
	.sources	= &clkset_mmc1,
	.divider_shift	= S5P_CLKDIV4_MMC1_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_mmc2 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 2,
		.ctrlbit        = S5P_CLKGATE_IP2_HSMMC2,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_MMC2_SHIFT,
	.mask		= S5P_CLKSRC4_MMC2_MASK,
	.sources	= &clkset_mmc2,
	.divider_shift	= S5P_CLKDIV4_MMC2_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_mmc3 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 3,
		.ctrlbit        = S5P_CLKGATE_IP2_HSMMC3,
		.enable		= s5pc11x_clk_ip2_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_MMC3_SHIFT,
	.mask		= S5P_CLKSRC4_MMC3_MASK,
	.sources	= &clkset_mmc3,
	.divider_shift	= S5P_CLKDIV4_MMC3_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_uart_uclk1 = {
	.clk	= {
		.name		= "uclk1",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_IP3_UART0,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_UART0_SHIFT,
	.mask		= S5P_CLKSRC4_UART0_MASK,
	.sources	= &clkset_uart,
	.divider_shift	= S5P_CLKDIV4_UART0_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_spi0 = {
	.clk	= {
		.name		= "spi-bus",
		.id		= 0,
		.ctrlbit        = S5P_CLKGATE_IP3_SPI0,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC5_SPI0_SHIFT,
	.mask		= S5P_CLKSRC5_SPI0_MASK,
	.sources	= &clkset_spi,
	.divider_shift	= S5P_CLKDIV5_SPI0_SHIFT,
	.reg_divider	= S5P_CLK_DIV5,
	.reg_source	= S5P_CLK_SRC5,
};

static struct clksrc_clk clk_spi1 = {
	.clk	= {
		.name		= "spi-bus",
		.id		= 1,
		.ctrlbit        = S5P_CLKGATE_IP3_SPI1,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC5_SPI1_SHIFT,
	.mask		= S5P_CLKSRC5_SPI1_MASK,
	.sources	= &clkset_spi,
	.divider_shift	= S5P_CLKDIV5_SPI1_SHIFT,
	.reg_divider	= S5P_CLK_DIV5,
	.reg_source	= S5P_CLK_SRC5,
};

static struct clksrc_clk clk_spi2 = {
	.clk	= {
		.name		= "spi-bus",
		.id		= 2,
		.ctrlbit        = S5P_CLKGATE_IP3_SPI2,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC5_SPI2_SHIFT,
	.mask		= S5P_CLKSRC5_SPI2_MASK,
	.sources	= &clkset_spi,
	.divider_shift	= S5P_CLKDIV5_SPI2_SHIFT,
	.reg_divider	= S5P_CLK_DIV5,
	.reg_source	= S5P_CLK_SRC5,
};

static struct clksrc_clk clk_fimd = {
	.clk	= {
		.name		= "sclk_fimd",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_IP1_FIMD,
		.enable		= s5pc11x_clk_ip1_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC1_FIMD_SHIFT,
	.mask		= S5P_CLKSRC1_FIMD_MASK,
	.sources	= &clkset_fimd,
	.divider_shift	= S5P_CLKDIV1_FIMD_SHIFT,
	.reg_divider	= S5P_CLK_DIV1,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_cam0 = {
	.clk	= {
		.name		= "sclk_cam0",
		.id		= -1,
		.ctrlbit        = NULL,
		.enable		= NULL,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC1_CAM0_SHIFT,
	.mask		= S5P_CLKSRC1_CAM0_MASK,
	.sources	= &clkset_cam0,
	.divider_shift	= S5P_CLKDIV1_CAM0_SHIFT,
	.reg_divider	= S5P_CLK_DIV1,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_cam1 = {
	.clk	= {
		.name		= "sclk_cam1",
		.id		= -1,
		.ctrlbit        = NULL,
		.enable		= NULL,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC1_CAM1_SHIFT,
	.mask		= S5P_CLKSRC1_CAM1_MASK,
	.sources	= &clkset_cam1,
	.divider_shift	= S5P_CLKDIV1_CAM1_SHIFT,
	.reg_divider	= S5P_CLK_DIV1,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_fimc0 = {
	.clk	= {
		.name		= "lclk_fimc",
		.id		= 0,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC0,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC3_FIMC0_LCLK_SHIFT,
	.mask		= S5P_CLKSRC3_FIMC0_LCLK_MASK,
	.sources	= &clkset_fimc2,
	.divider_shift	= S5P_CLKDIV3_FIMC0_LCLK_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clksrc_clk clk_fimc1 = {
	.clk	= {
		.name		= "lclk_fimc",
		.id		= 1,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC1 ,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC3_FIMC1_LCLK_SHIFT,
	.mask		= S5P_CLKSRC3_FIMC1_LCLK_MASK,
	.sources	= &clkset_fimc1,
	.divider_shift	= S5P_CLKDIV3_FIMC1_LCLK_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clksrc_clk clk_fimc2 = {
	.clk	= {
		.name		= "lclk_fimc",
		.id		= 2,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC2 ,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC3_FIMC2_LCLK_SHIFT,
	.mask		= S5P_CLKSRC3_FIMC2_LCLK_MASK,
	.sources	= &clkset_fimc2,
	.divider_shift	= S5P_CLKDIV3_FIMC2_LCLK_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clksrc_clk sclk_fimc = {
	.clk	= {
		.name		= "sclk_fimc",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC2,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC3_FIMC2_LCLK_SHIFT,
	.mask		= S5P_CLKSRC3_FIMC2_LCLK_MASK,
	.sources	= &clkset_fimc0,
	.divider_shift	= S5P_CLKDIV3_FIMC2_LCLK_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC3,
};

struct clk clk_i2s_cd0 = {
	.name		= "i2s_cdclk0",
	.id		= -1,
};

struct clk clk_i2s_cd1 = {
	.name		= "i2s_cdclk1",
	.id		= -1,
};

struct clk clk_pcm_cd0 = {
	.name		= "pcm_cdclk0",
	.id		= -1,
};

struct clk clk_pcm_cd1 = {
	.name		= "pcm_cdclk1",
	.id		= -1,
};

static struct clk *clkset_audio0_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_audio0 = {
	.sources	= clkset_audio0_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio0_list),
};

static struct clksrc_clk clk_audio0 = {
	.clk	= {
		.name		= "sclk_audio",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_IP3_I2S0,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC6_AUDIO0_SHIFT,
	.mask		= S5P_CLKSRC6_AUDIO0_MASK,
	.sources	= &clkset_audio0,
	.divider_shift	= S5P_CLKDIV6_AUDIO0_SHIFT,
	.reg_divider	= S5P_CLK_DIV6,
	.reg_source	= S5P_CLK_SRC6,
};

static struct clk *clkset_audio1_list[] = {
	&clk_i2s_cd1,
	&clk_pcm_cd1,
	NULL,
	NULL,
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_audio1 = {
	.sources	= clkset_audio1_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio1_list),
};

static struct clksrc_clk clk_audio1 = {
	.clk	= {
		.name		= "audio-bus",
		.id		= 1,
		.ctrlbit        = S5P_CLKGATE_IP3_I2S1,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC6_AUDIO1_SHIFT,
	.mask		= S5P_CLKSRC6_AUDIO1_MASK,
	.sources	= &clkset_audio1,
	.divider_shift	= S5P_CLKDIV6_AUDIO1_SHIFT,
	.reg_divider	= S5P_CLK_DIV6,
	.reg_source	= S5P_CLK_SRC6,
};

static struct clk *clkset_audio2_list[] = {
	NULL,
	&clk_pcm_cd0,
	NULL,
	NULL,
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_audio2 = {
	.sources	= clkset_audio2_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio2_list),
};

static struct clksrc_clk clk_audio2 = {
	.clk	= {
		.name		= "audio-bus",
		.id		= 2,
		.ctrlbit        = S5P_CLKGATE_IP3_I2S2,
		.enable		= s5pc11x_clk_ip3_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC6_AUDIO2_SHIFT,
	.mask		= S5P_CLKSRC6_AUDIO2_MASK,
	.sources	= &clkset_audio2,
	.divider_shift	= S5P_CLKDIV6_AUDIO2_SHIFT,
	.reg_divider	= S5P_CLK_DIV6,
	.reg_source	= S5P_CLK_SRC6,
};

static struct clk *clkset_i2smain_list[] = {
	NULL, /* XXTI */
	&clk_fout_epll,
};

extern struct powerdomain pd_audio;

static struct clk_sources clkset_i2smain_clk = {
	.sources	= clkset_i2smain_list,
	.nr_sources	= ARRAY_SIZE(clkset_i2smain_list),
};

static struct clksrc_clk clk_i2smain = {
	.clk	= {
		.name		= "i2smain_clk", /* C110 calls it Main CLK */
		.id		= -1,
		.pd		= &pd_audio,
		.set_parent	= s5pc11x_setparent_clksrc,
	},
	.shift		= S5P_AUDSS_CLKSRC_MAIN_SHIFT,
	.mask		= S5P_AUDSS_CLKSRC_MAIN_MASK,
	.sources	= &clkset_i2smain_clk,
	.reg_source	= S5P_CLKSRC_AUDSS,
};

static struct clk *clkset_audss_hclk_list[] = {
	&clk_i2smain.clk,
	&clk_i2s_cd0,
};

static struct clk_sources clkset_audss_hclk = {
	.sources	= clkset_audss_hclk_list,
	.nr_sources	= ARRAY_SIZE(clkset_audss_hclk_list),
};

static struct clksrc_clk clk_audss_hclk = {
	.clk	= {
		.name		= "audss_hclk", /* C110 calls it BUSCLK */
		.id		= -1,
		.pd		= &pd_audio,
		.ctrlbit        = S5P_AUDSS_CLKGATE_HCLKI2S,
		.enable		= s5pc11x_audss_clkctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_AUDSS_CLKSRC_BUSCLK_SHIFT,
	.mask		= S5P_AUDSS_CLKSRC_BUSCLK_MASK,
	.sources	= &clkset_audss_hclk,
	.divider_shift	= S5P_AUDSS_CLKDIV_BUSCLK_SHIFT,
	.reg_divider	= S5P_CLKDIV_AUDSS,
	.reg_source	= S5P_CLKSRC_AUDSS,
};

static struct clk *clkset_i2sclk_list[] = {
	&clk_i2smain.clk,
	&clk_i2s_cd0,
	&clk_audio0.clk,
};

static struct clk_sources clkset_i2sclk = {
	.sources	= clkset_i2sclk_list,
	.nr_sources	= ARRAY_SIZE(clkset_i2sclk_list),
};

static struct clksrc_clk clk_i2sclk = {
	.clk	= {
		.name		= "audio-bus",
		.id		= 0,
		.pd		= &pd_audio,
		.ctrlbit        = S5P_AUDSS_CLKGATE_CLKI2S,
		.enable		= s5pc11x_audss_clkctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_AUDSS_CLKSRC_I2SCLK_SHIFT,
	.mask		= S5P_AUDSS_CLKSRC_I2SCLK_MASK,
	.sources	= &clkset_i2sclk,
	.divider_shift	= S5P_AUDSS_CLKDIV_I2SCLK_SHIFT,
	.reg_divider	= S5P_CLKDIV_AUDSS,
	.reg_source	= S5P_CLKSRC_AUDSS,
};

/* g2d */
static struct clk *clkset_g2d_list[] = {
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_g2d = {
	.sources	= clkset_g2d_list,
	.nr_sources	= ARRAY_SIZE(clkset_g2d_list),
};

static struct clksrc_clk clk_g2d = {
	.clk	= {
		.name		= "sclk_g2d",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_IP0_G2D,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC2_G2D_SHIFT,
	.mask		= S5P_CLKSRC2_G2D_MASK,
	.sources	= &clkset_g2d,
	.divider_shift	= S5P_CLKDIV2_G2D_SHIFT,
	.reg_divider	= S5P_CLK_DIV2,
	.reg_source	= S5P_CLK_SRC2,
};

/* mfc */
static struct clk *clkset_mfc_list[] = {
	&clk_dout_a2m,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_mfc = {
	.sources	= clkset_mfc_list,
	.nr_sources	= ARRAY_SIZE(clkset_mfc_list),
};

static struct clksrc_clk clk_mfc = {
	.clk	= {
		.name		= "sclk_mfc",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_IP0_MFC,
		.enable		= s5pc11x_clk_ip0_ctrl,
		.set_parent	= s5pc11x_setparent_clksrc,
		.get_rate	= s5pc11x_getrate_clksrc,
		.set_rate	= s5pc11x_setrate_clksrc,
		.round_rate	= s5pc11x_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC2_MFC_SHIFT,
	.mask		= S5P_CLKSRC2_MFC_MASK,
	.sources	= &clkset_mfc,
	.divider_shift	= S5P_CLKDIV2_MFC_SHIFT,
	.reg_divider	= S5P_CLK_DIV2,
	.reg_source	= S5P_CLK_SRC2,
};

/* Clock initialisation code */

static struct clksrc_clk *init_parents[] = {
	&clk_mout_apll,
	&clk_mout_epll,
	&clk_mout_mpll,
	&clk_mout_vpll,
	&clk_mmc0,
	&clk_mmc1,
	&clk_mmc2,
	&clk_mmc3,	
	&clk_uart_uclk1,
	&clk_spi0,
	&clk_spi1,
	&clk_spi2,
	&clk_audio0,
	&clk_audio1,
	&clk_audio2,
	&clk_i2sclk,
	&clk_audss_hclk,
	&clk_i2smain,
	&clk_fimd,
	&clk_cam0,
	&clk_cam1,
	&clk_fimc0,
	&clk_fimc1,
	&clk_fimc2,
	&sclk_fimc,
	&clk_g2d,
	&clk_mfc,
};

static void __init_or_cpufreq s5pc11x_set_clksrc(struct clksrc_clk *clk)
{
	struct clk_sources *srcs = clk->sources;
	u32 clksrc = __raw_readl(clk->reg_source);

	clksrc &= clk->mask;
	clksrc >>= clk->shift;

	if (clksrc > srcs->nr_sources || !srcs->sources[clksrc]) {
		printk(KERN_ERR "%s: bad source %d\n",
		       clk->clk.name, clksrc);
		return;
	}

	clk->clk.parent = srcs->sources[clksrc];

	printk(KERN_INFO "%s: source is %s (%d), rate is %ld\n",
	       clk->clk.name, clk->clk.parent->name, clksrc,
	       clk_get_rate(&clk->clk));
}

#define GET_DIV(clk, field) ((((clk) & field##_MASK) >> field##_SHIFT) + 1)

void __init_or_cpufreq s5pc110_setup_clocks(void)
{
	struct clk *xtal_clk;
	unsigned long xtal;
	unsigned long armclk;
	unsigned long hclk200;
	unsigned long hclk166;
	unsigned long hclk133;
	unsigned long pclk100;
	unsigned long pclk83;
	unsigned long pclk66;
	unsigned long apll;
	unsigned long mpll;
	unsigned long vpll;
	unsigned long epll;
	unsigned int ptr;
	u32 clkdiv0, clkdiv1;

	printk(KERN_DEBUG "%s: registering clocks\n", __func__);

	clkdiv0 = __raw_readl(S5P_CLK_DIV0);
	clkdiv1 = __raw_readl(S5P_CLK_DIV1);

	printk(KERN_DEBUG "%s: clkdiv0 = %08x, clkdiv1 = %08x\n", __func__, clkdiv0, clkdiv1);

	xtal_clk = clk_get(NULL, "xtal");
	BUG_ON(IS_ERR(xtal_clk));

	xtal_rate = xtal = clk_get_rate(xtal_clk);
	clk_put(xtal_clk);

	printk(KERN_DEBUG "%s: xtal is %ld\n", __func__, xtal);

	apll = s5pc11x_get_pll(xtal, __raw_readl(S5P_APLL_CON), S5PC11X_PLL_APLL);
	mpll = s5pc11x_get_pll(xtal, __raw_readl(S5P_MPLL_CON), S5PC11X_PLL_MPLL);
	epll = s5pc11x_get_pll(xtal, __raw_readl(S5P_EPLL_CON), S5PC11X_PLL_EPLL);
	vpll = s5pc11x_get_pll(xtal, __raw_readl(S5P_VPLL_CON), S5PC11X_PLL_VPLL);

	printk(KERN_INFO "S5PC100: PLL settings, A=%ld, M=%ld, E=%ld, H=%ld\n",
	       apll, mpll, epll, vpll);

	armclk = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_APLL);
	if(__raw_readl(S5P_CLK_SRC0)&(1<<S5P_CLKSRC0_MUX200_SHIFT)) {
		hclk200 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK200);
	} else {
		hclk200 = armclk / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK200);
	}
	if(__raw_readl(S5P_CLK_SRC0)&(1<<S5P_CLKSRC0_MUX166_SHIFT)) {
		hclk166 = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_A2M);
		hclk166 = hclk166 / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK166);
	} else {
		hclk166 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK166);
	}
	if(__raw_readl(S5P_CLK_SRC0)&(1<<S5P_CLKSRC0_MUX133_SHIFT)) {
		hclk133 = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_A2M);
		hclk133 = hclk133 / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK133);
	} else {
		hclk133 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_HCLK133);
	}
		
	pclk100 = hclk200 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK100);
	pclk83 = hclk166 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK83);
	pclk66 = hclk133 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLK66);


	printk(KERN_INFO "S5PC110: ARMCLK=%ld, HCLKM=%ld, HCLKD=%ld, HCLKP=%ld, PCLKM=%ld, PCLKD=%ld, PCLKP=%ld\n",
	       armclk, hclk200, hclk166, hclk133, pclk100, pclk83, pclk66);
	
	clk_fout_apll.rate = apll;
	clk_fout_mpll.rate = mpll;
	clk_fout_epll.rate = epll;
	clk_mout_vpll.clk.rate = vpll;

	clk_f.rate = armclk;
	clk_h200.rate = hclk200;
	clk_p100.rate = pclk100;
	clk_h166.rate = hclk166;
	clk_p83.rate = pclk83;
	clk_h133.rate = hclk133;
	clk_p66.rate = pclk66;
	clk_h.rate = hclk133;
	clk_p.rate = pclk66;

	clk_set_parent(&clk_mmc0.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mmc1.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mmc2.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mmc3.clk, &clk_mout_mpll.clk);

	clk_set_parent(&clk_spi0.clk, &clk_mout_epll.clk);
	clk_set_parent(&clk_spi1.clk, &clk_mout_epll.clk);
	clk_set_parent(&clk_spi2.clk, &clk_mout_epll.clk);

	clk_set_parent(&clk_i2smain.clk, &clk_fout_epll);
	clk_set_parent(&clk_i2sclk.clk, &clk_i2smain.clk);
	clk_set_parent(&clk_audio0.clk, &clk_mout_epll.clk);
	clk_set_parent(&clk_audio1.clk, &clk_mout_epll.clk);
	clk_set_parent(&clk_audio2.clk, &clk_mout_epll.clk);

	clk_set_parent(&clk_g2d.clk, &clk_mout_mpll.clk);

	for (ptr = 0; ptr < ARRAY_SIZE(init_parents); ptr++)
		s5pc11x_set_clksrc(init_parents[ptr]);

	clk_set_rate(&clk_mmc0.clk, 50*MHZ);	
	clk_set_rate(&clk_mmc1.clk, 50*MHZ);
	clk_set_rate(&clk_mmc2.clk, 50*MHZ);
	clk_set_rate(&clk_mmc3.clk, 50*MHZ);

	clk_set_rate(&clk_g2d.clk, 250*MHZ);
}

static struct clk *clks[] __initdata = {
	&clk_ext_xtal_mux,
	&clk_ext_xtal_usb,
	&clk_ext_xtal_rtc,
	&clk_mout_epll.clk,
	&clk_fout_epll,
	&clk_mout_mpll.clk,
	&clk_dout_ck166,
	&clk_mout_vpll.clk,
	&clk_sclk_hdmi,
	&clk_srclk,
	&clk_mmc0.clk,
	&clk_mmc1.clk,
	&clk_mmc2.clk,
	&clk_mmc3.clk,	
	&clk_uart_uclk1.clk,
	&clk_spi0.clk,
	&clk_spi1.clk,
	&clk_spi2.clk,
	&clk_i2s_cd0,
	&clk_i2s_cd1,
	&clk_audio0.clk,
	&clk_audio1.clk,
	&clk_audio2.clk,
	&clk_i2sclk.clk,
	&clk_audss_hclk.clk,
	&clk_i2smain.clk,
	&clk_fimd.clk,
	&clk_dout_apll,
	&clk_dout_a2m,
	&clk_cam0.clk,
	&clk_cam1.clk,
	&clk_fimc0.clk,
	&clk_fimc1.clk,
	&clk_fimc2.clk,
	&sclk_fimc.clk,
	&clk_g2d.clk,
};

void __init s5pc110_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	for (ptr = 0; ptr < ARRAY_SIZE(clks); ptr++) {
		clkp = clks[ptr];
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}
}
