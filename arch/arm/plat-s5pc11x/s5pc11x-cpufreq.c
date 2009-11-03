/*
 *  linux/arch/arm/plat-s5pc11x/s5pc11x-cpufreq.c
 *
 *  CPU frequency scaling for S5PC110
 *
 *  Copyright (C) 2008 Samsung Electronics
 *
 *  Based on cpu-sa1110.c, Copyright (C) 2001 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
//#include <mach/hardware.h>
#include <asm/system.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-clock.h>
#include <plat/cpu-freq.h>
#include <plat/pll.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>

static struct clk * mpu_clk;
static struct regulator *arm_regulator;
static struct regulator *internal_regulator;

struct s3c_cpufreq_freqs s3c_freqs;

#define OVERCLOCKED_FREQ	1000*1000	// 1GHz
#define BOOTUP_FREQ		800*1000	// 800MHz

/* frequency */
static struct cpufreq_frequency_table s5pc110_freq_table[] = {
	{L0, 1000*1000},	// OVerclocked
	{L1, 800*1000},
	{L2, 400*1000},
//	{L2, 200*1000},
	{L3, 100*1000},
	{0, CPUFREQ_TABLE_END},
};

struct s5pc11x_dvs_conf {
	const unsigned long	lvl;		// DVFS level : L0,L1,L2,L3...
	unsigned long		arm_volt;	// uV
	unsigned long		int_volt;	// uV
};

static struct s5pc11x_dvs_conf s5pc110_dvs_conf[] = {
	{
		.lvl		= L0,
		.arm_volt	= 1300000,
		.int_volt	= 1200000,
	}, {
		.lvl		= L1,
		.arm_volt	= 1200000,
		.int_volt	= 1200000,

	}, {
		.lvl		= L2,
		.arm_volt	= 1200000,
		.int_volt	= 1200000,
		
	}, { 
		.lvl		= L3,
		.arm_volt	= 1000000,
		.int_volt	= 1000000,
	},
};

static u32 clkdiv_val[4][9] = {
/*{ APLL, A2M, HCLK_MSYS, PCLK_MSYS,
 *	HCLK_DSYS, PCLK_DSYS, HCLK_PSYS, PCLK_PSYS,
 *		ONEDRAM}
 */		
	{0, 4, 4, 1, 3, 1, 4, 1, 3},	/* L0 : [1000/200/100][166/83][133/66] */
	{0, 3, 3, 1, 3, 1, 4, 1, 3},	/* L1 : [800/200/100][166/83][133/66] */
	{1, 3, 1, 1, 3, 1, 4, 1, 3},	/* L2 : [400/200/100][166/83][133/66] */
	{7, 7, 0, 0, 7, 0, 9, 0, 7},	/* L3 : [100/100/50][83/83][66/66] */
};

static struct s3c_freq s5pc110_clk_info[] = {
	{ 
		.fclk 		= 1000000, 
		.armclk 	= 1000000, 
		.hclk_tns 	= 0, 
		.hclk 		= 133000, 
		.pclk 		= 66000, 
		.hclk_msys 	= 200000, 
		.pclk_msys 	= 100000, 
		.hclk_dsys 	= 166000, 
		.pclk_dsys 	= 83000,
	}, { 
		.fclk 		= 800000, 
		.armclk 	= 800000, 
		.hclk_tns 	= 0, 
		.hclk 		= 133000, 
		.pclk 		= 66000, 
		.hclk_msys 	= 200000, 
		.pclk_msys 	= 100000, 
		.hclk_dsys 	= 166000, 
		.pclk_dsys 	= 83000,
	}, { 
		.fclk 		= 800000, 
		.armclk 	= 400000, 
		.hclk_tns 	= 0, 
		.hclk 		= 133000, 
		.pclk 		= 66000, 
		.hclk_msys 	= 200000, 
		.pclk_msys 	= 100000, 
		.hclk_dsys 	= 166000, 
		.pclk_dsys 	= 83000,
	}, { 
		.fclk 		= 800000, 
		.armclk 	= 100000, 
		.hclk_tns 	= 0, 
		.hclk 		= 66000, 
		.pclk 		= 66000, 
		.hclk_msys 	= 100000, 
		.pclk_msys 	= 50000, 
		.hclk_dsys 	= 83000, 
		.pclk_dsys 	= 83000,
	},	
};

int s5pc110_verify_speed(struct cpufreq_policy *policy)
{

	if (policy->cpu)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, s5pc110_freq_table);
}

unsigned int s5pc110_getspeed(unsigned int cpu)
{
	unsigned long rate;

	if (cpu)
		return 0;

	rate = clk_get_rate(mpu_clk) / KHZ_T;

	return rate;
}

static int pm_mode = 0;

static int s5pc110_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
	int ret = 0;
	unsigned long arm_clk;
	unsigned int index ,reg , arm_volt, int_volt;
	unsigned int pll_changing = 0;
	unsigned int bus_speed_changing = 0;

	s3c_freqs.freqs.old = s5pc110_getspeed(0);

	if (cpufreq_frequency_table_target(policy, s5pc110_freq_table, target_freq, relation, &index)) {
		ret = -EINVAL;
		goto out;
	}

	arm_clk = s5pc110_freq_table[index].frequency;

	s3c_freqs.freqs.new = arm_clk;
	s3c_freqs.freqs.cpu = 0;

	if (s3c_freqs.freqs.new == s3c_freqs.freqs.old)
		return 0;
	
	arm_volt = s5pc110_dvs_conf[index].arm_volt;
	int_volt = s5pc110_dvs_conf[index].int_volt;

	/* iNew clock inforamtion update */
	memcpy(&s3c_freqs.new, &s5pc110_clk_info[index], sizeof(struct s3c_freq));

	if (!pm_mode) {	

		cpufreq_notify_transition(&s3c_freqs.freqs, CPUFREQ_PRECHANGE);

		if (s3c_freqs.freqs.new > s3c_freqs.freqs.old) {
			// Voltage up code
			regulator_set_voltage(arm_regulator, arm_volt, arm_volt);
			regulator_set_voltage(internal_regulator, int_volt, int_volt);
		}
	}

	/* Check if there need to change PLL */
	if (s3c_freqs.new.fclk != s3c_freqs.old.fclk)	
		pll_changing = 1;
		
	/* Check if there need to change System bus clock */
	if (s3c_freqs.new.hclk_msys != s3c_freqs.old.hclk_msys)
		bus_speed_changing = 1;

	if (index == L3) {
		__raw_writel(0x30c, S5P_VA_DMC1 + 0x30);	// DRAM refresh counter setting
#if defined(CONFIG_S5PC110_AC_TYPE)
		__raw_writel(0x287, S5P_VA_DMC0 + 0x30);
#else
		__raw_writel(0x30c, S5P_VA_DMC0 + 0x30);
#endif

	}

/* APLL should be changed in this level
 * APLL -> MPLL(for stable transition) -> APLL
 * Some clock source's clock API  are not prepared. Do not use clock API
 * in below code.
 */
	
	if (pll_changing) {
		__raw_writel(0x40d, S5P_VA_DMC1 + 0x30);
			
		reg = __raw_readl(S5P_CLK_SRC0);
		reg &=~(S5P_CLKSRC0_MUX200_MASK);	
		reg |= (0x1 << S5P_CLKSRC0_MUX200_SHIFT); // SCLKAPLL -> SCLKMPLL	
		__raw_writel(reg, S5P_CLK_SRC0);

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT0);
		} while (reg & (0x1<<18));
	
		/* Change APLL to MPLL in MFC_MUX and G3D MUX */
		reg = __raw_readl(S5P_CLK_DIV2);
		reg &=~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK);
		reg |= (0x3<<S5P_CLKDIV2_G3D_SHIFT)|(0x3<<S5P_CLKDIV2_MFC_MASK);	
		__raw_writel(reg, S5P_CLK_DIV2);

		do {
			reg = __raw_readl(S5P_CLK_DIV_STAT0);
		} while (reg & ((1<<16)|(1<<17)));

		reg = __raw_readl(S5P_CLK_SRC2);
		reg &=~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK);
		reg |= (1<<S5P_CLKSRC2_G3D_SHIFT) | (1<<S5P_CLKSRC2_MFC_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC2);

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT1);
		} while (reg & ((1<<7)|(1<<3)));

#if defined(CONFIG_S5PC110_H_TYPE)
		/* DMC0 source clock : SCLKA2M -> SCLKMPLL */
		__raw_writel(0x50e, S5P_VA_DMC0 + 0x30);

		reg = __raw_readl(S5P_CLK_DIV6);
		reg &=~(S5P_CLKDIV6_ONEDRAM_MASK);
		reg |= (0x3 << S5P_CLKDIV6_ONEDRAM_SHIFT);
		__raw_writel(reg, S5P_CLK_DIV6);		
	
		do {
			reg = __raw_readl(S5P_CLK_DIV_STAT1);
		} while (reg & (1<<15));

		reg = __raw_readl(S5P_CLK_SRC6);
		reg &=~(S5P_CLKSRC6_ONEDRAM_MASK);
		reg |= (0x1<<S5P_CLKSRC6_ONEDRAM_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC6); 

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT1);
		} while (reg & (1<<11));
#endif
	}
	
	reg = __raw_readl(S5P_CLK_DIV0);
	
	reg &=~(S5P_CLKDIV0_APLL_MASK | S5P_CLKDIV0_A2M_MASK \
		| S5P_CLKDIV0_HCLK200_MASK | S5P_CLKDIV0_PCLK100_MASK \
		| S5P_CLKDIV0_HCLK166_MASK | S5P_CLKDIV0_PCLK83_MASK \
		| S5P_CLKDIV0_HCLK133_MASK | S5P_CLKDIV0_PCLK66_MASK);
	
	reg |= ((clkdiv_val[index][0]<<S5P_CLKDIV0_APLL_SHIFT)|(clkdiv_val[index][1]<<S5P_CLKDIV0_A2M_SHIFT) \
		|(clkdiv_val[index][2]<<S5P_CLKDIV0_HCLK200_SHIFT)|(clkdiv_val[index][3]<<S5P_CLKDIV0_PCLK100_SHIFT) \
		|(clkdiv_val[index][4]<<S5P_CLKDIV0_HCLK166_SHIFT)|(clkdiv_val[index][5]<<S5P_CLKDIV0_PCLK83_SHIFT) \
		|(clkdiv_val[index][6]<<S5P_CLKDIV0_HCLK133_SHIFT)|(clkdiv_val[index][7]<<S5P_CLKDIV0_PCLK66_SHIFT));
	
	__raw_writel(reg, S5P_CLK_DIV0);

	do {
		reg = __raw_readl(S5P_CLK_DIV_STAT0);
	} while (reg & 0xff);

	if (pll_changing) {
		reg = __raw_readl(S5P_CLK_SRC6);
		reg &=~S5P_CLKSRC6_HPM_MASK;
		reg |= (1<<S5P_CLKSRC6_HPM_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC6);

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT1);
		} while (reg & (1<<18));
		
		// Deselect APLL output : FOUTAPLL -> FINPLL
		reg = __raw_readl(S5P_CLK_SRC0);
		reg &=~S5P_CLKSRC0_APLL_MASK;
		reg |= 0<<S5P_CLKSRC0_APLL_SHIFT;
		__raw_writel(reg, S5P_CLK_SRC0);
		
		// Power OFF PLL
		reg = __raw_readl(S5P_APLL_CON);
		reg &=~(1<<31);
		__raw_writel(reg, S5P_APLL_CON);
		
		// Lock time = 300us*24Mhz = 7200(0x1c20)
		__raw_writel(0x1c20, S5P_APLL_LOCK);

		if (index == L0)
			__raw_writel(APLL_VAL_1000, S5P_APLL_CON);
		else
			__raw_writel(APLL_VAL_800, S5P_APLL_CON);

		do {
			reg = __raw_readl(S5P_APLL_CON);
		} while (!(reg & (0x1<<29)));
		
		// Select APLL output : FINPLL -> FOUTAPLL
		reg = __raw_readl(S5P_CLK_SRC0);
		reg &=~S5P_CLKSRC0_APLL_MASK;
		reg |= 1<<S5P_CLKSRC0_APLL_SHIFT;
		__raw_writel(reg, S5P_CLK_SRC0);
		
		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT0);
		} while (reg & (1<<2));
			
		/* Change MPLL to APLL in MFC_MUX and G3D MUX */
		reg = __raw_readl(S5P_CLK_DIV2);
		reg &=~(S5P_CLKDIV2_G3D_MASK | S5P_CLKDIV2_MFC_MASK);
		reg |= (0x0<<S5P_CLKDIV2_G3D_SHIFT)|(0x0<<S5P_CLKDIV2_MFC_MASK);	
		__raw_writel(reg, S5P_CLK_DIV2);

		do {
			reg = __raw_readl(S5P_CLK_DIV_STAT0);
		} while (reg & ((1<<16)|(1<<17)));

		reg = __raw_readl(S5P_CLK_SRC2);
		reg &=~(S5P_CLKSRC2_G3D_MASK | S5P_CLKSRC2_MFC_MASK);
		reg |= (0<<S5P_CLKSRC2_G3D_SHIFT) | (0<<S5P_CLKSRC2_MFC_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC2);

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT1);
		} while (reg & ((1<<7)|(1<<3)));

		/* Change MPLL to APLL in MSYS_MUX and HPM_MUX */	
		reg = __raw_readl(S5P_CLK_SRC0);
		reg &=~(S5P_CLKSRC0_MUX200_MASK);	
		reg |= (0x0 << S5P_CLKSRC0_MUX200_SHIFT); // SCLKMPLL -> SCLKAPLL	
		__raw_writel(reg, S5P_CLK_SRC0);

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT0);
		} while (reg & (0x1<<18));
	
		__raw_writel(0x618, S5P_VA_DMC1 + 0x30);

#if defined(CONFIG_S5PC110_H_TYPE)
		/* DMC0 source clock : SCLKMPLL -> SCLKA2M */
		reg = __raw_readl(S5P_CLK_SRC6);
		reg &=~(S5P_CLKSRC6_ONEDRAM_MASK);
		reg |= (0x0<<S5P_CLKSRC6_ONEDRAM_SHIFT);
		__raw_writel(reg, S5P_CLK_SRC6); 

		do {
			reg = __raw_readl(S5P_CLK_MUX_STAT1);
		} while (reg & (1<<11));
		
		reg = __raw_readl(S5P_CLK_DIV6);
		reg &=~(S5P_CLKDIV6_ONEDRAM_MASK);
		reg |= (0x0 << S5P_CLKDIV6_ONEDRAM_SHIFT);
		__raw_writel(reg, S5P_CLK_DIV6);		
	
		do {
			reg = __raw_readl(S5P_CLK_DIV_STAT1);
		} while (reg & (1<<15));

		__raw_writel(0x618, S5P_VA_DMC0 + 0x30);
#endif

	}


/* L3 level need to change memory bus speed, hence onedram clock divier and 
 * memory refresh parameter should be changed 
 * Only care L2 <-> L3 transition
 */
	if (bus_speed_changing) {
		reg = __raw_readl(S5P_CLK_DIV6);
		reg &=~S5P_CLKDIV6_ONEDRAM_MASK;
		reg |= (clkdiv_val[index][8]<<S5P_CLKDIV6_ONEDRAM_SHIFT);
		__raw_writel(reg, S5P_CLK_DIV6);

		do {
			reg = __raw_readl(S5P_CLK_DIV_STAT1);
		} while (reg & (1<<15));

		if (index == L2) {
	                __raw_writel(0x618, S5P_VA_DMC1 + 0x30);        // DRAM refresh counter setting
#if defined(CONFIG_S5PC110_AC_TYPE)
	                __raw_writel(0x50e, S5P_VA_DMC0 + 0x30);
#else
        	        __raw_writel(0x618, S5P_VA_DMC0 + 0x30);
#endif
		}
	}

	if (!pm_mode) {
		if (s3c_freqs.freqs.new < s3c_freqs.freqs.old) {
			// Voltage down
			regulator_set_voltage(arm_regulator, arm_volt, arm_volt);
			regulator_set_voltage(internal_regulator, int_volt, int_volt);
		}

		cpufreq_notify_transition(&s3c_freqs.freqs, CPUFREQ_POSTCHANGE);
	}
	mpu_clk->rate = s3c_freqs.freqs.new * KHZ_T;

	memcpy(&s3c_freqs.old, &s3c_freqs.new, sizeof(struct s3c_freq));
	
	printk("Perf changed[L%d], relation[%d]\n",index, relation);

out: 	
	return ret;
}

#ifdef CONFIG_PM
static int s5pc110_cpufreq_suspend(struct cpufreq_policy *policy, pm_message_t pmsg)
{
	int ret = 0;

	pm_mode = 1;

	ret = s5pc110_target(policy, BOOTUP_FREQ, 0);

	return ret;
}

static int s5pc110_cpufreq_resume(struct cpufreq_policy *policy)
{
	int ret = 0;

	pm_mode = 0;

	return ret;
}
#endif

static int __init s5pc110_cpu_init(struct cpufreq_policy *policy)
{
	u32 reg;
	u32 err;

#ifdef CLK_OUT_PROBING
	
	reg = __raw_readl(S5P_CLK_OUT);
	reg &=~(0x1f << 12 | 0xf << 20);	// Mask Out CLKSEL bit field and DIVVAL
	reg |= (0xf << 12 | 0x1 << 20);		// CLKSEL = ARMCLK/4, DIVVAL = 1 
	__raw_writel(reg, S5P_CLK_OUT);
#endif
	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);
#if defined(CONFIG_REGULATOR)
	arm_regulator = regulator_get(NULL, "vddarm");
	if (IS_ERR(arm_regulator)) {
		printk(KERN_ERR "failed to get regulater resource %s\n","vddarm");
		return PTR_ERR(arm_regulator);
	}
	internal_regulator = regulator_get(NULL, "vddint");
	if (IS_ERR(internal_regulator)) {
		printk(KERN_ERR "failed to get regulater resource %s\n","vddint");
		return PTR_ERR(internal_regulator);
	}

#endif

	if (policy->cpu != 0)
		return -EINVAL;
	policy->cur = policy->min = policy->max = s5pc110_getspeed(0);

	cpufreq_frequency_table_get_attr(s5pc110_freq_table, policy->cpu);

	policy->cpuinfo.transition_latency = 40000;	//1us

	memcpy(&s3c_freqs.old, &s5pc110_clk_info[1], sizeof(struct s3c_freq));

	return cpufreq_frequency_table_cpuinfo(policy, s5pc110_freq_table);
}

static struct cpufreq_driver s5pc110_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= s5pc110_verify_speed,
	.target		= s5pc110_target,
	.get		= s5pc110_getspeed,
	.init		= s5pc110_cpu_init,
	.name		= "s5pc110",
#ifdef CONFIG_PM
	.suspend	= s5pc110_cpufreq_suspend,
	.resume		= s5pc110_cpufreq_resume,
#endif
};

static int __init s5pc110_cpufreq_init(void)
{
	return cpufreq_register_driver(&s5pc110_driver);
}

late_initcall(s5pc110_cpufreq_init);
