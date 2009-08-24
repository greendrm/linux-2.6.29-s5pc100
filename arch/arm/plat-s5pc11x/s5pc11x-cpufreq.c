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

//#include <mach/hardware.h>
#include <asm/system.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-clock.h>
#include <plat/cpu-freq.h>
#include <plat/pll.h>
#include <plat/clock.h>

static struct clk * mpu_clk;

/* frequency */
static struct cpufreq_frequency_table s5pc110_freq_table[] = {
	{L0, 800*1000},
	{L1, 400*1000},
	{L2, 200*1000},
	{L3, 100*1000},
//	{L4, 83*1000},
	{0, CPUFREQ_TABLE_END},
};

/* TODO: Add support for SDRAM timing changes */

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

static int s5pc110_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
	struct cpufreq_freqs freqs;
	int ret = 0;
	unsigned long arm_clk;
	unsigned int index,reg;

	freqs.old = s5pc110_getspeed(0);

	if (cpufreq_frequency_table_target(policy, s5pc110_freq_table, target_freq, relation, &index)) {
		ret = -EINVAL;
		goto out;
	}

	arm_clk = s5pc110_freq_table[index].frequency;

	freqs.new = arm_clk;
	freqs.cpu = 0;

	if (freqs.new == freqs.old)
		return -EINVAL;

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	if (freqs.new > freqs.old) {
		
		// Add voltage up code
	}
		
	reg = __raw_readl(S5P_CLK_DIV0);
	reg &=~CLK_DIV0_MASK;
	reg |= ((clkdiv0_val[index][0]<<0)|(clkdiv0_val[index][1]<<8)|(clkdiv0_val[index][2]<<12));
	__raw_writel(reg, S5P_CLK_DIV0);


	if (freqs.new < freqs.old) {
		
		// Add voltage up code
	}
	
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	mpu_clk->rate = freqs.new * KHZ_T;

	printk("Perf changed[L%d]\n",index);

out: 	
	return ret;
}

static int __init s5pc110_cpu_init(struct cpufreq_policy *policy)
{
	u32 reg;

#ifdef CLK_OUT_PROBING
	
	reg = __raw_readl(S5P_CLK_OUT);
	reg &=~(0x1f << 12 | 0xf << 20);	// Mask Out CLKSEL bit field and DIVVAL
	reg |= (0xf << 12 | 0x1 << 20);		// CLKSEL = ARMCLK/4, DIVVAL = 1 
	__raw_writel(reg, S5P_CLK_OUT);
#endif
	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	if (policy->cpu != 0)
		return -EINVAL;
	policy->cur = policy->min = policy->max = s5pc110_getspeed(0);

	cpufreq_frequency_table_get_attr(s5pc110_freq_table, policy->cpu);

	policy->cpuinfo.transition_latency = 40000;	//1us

	return cpufreq_frequency_table_cpuinfo(policy, s5pc110_freq_table);
}

static struct cpufreq_driver s5pc110_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= s5pc110_verify_speed,
	.target		= s5pc110_target,
	.get		= s5pc110_getspeed,
	.init		= s5pc110_cpu_init,
	.name		= "s5pc110",
};

static int __init s5pc110_cpufreq_init(void)
{
	return cpufreq_register_driver(&s5pc110_driver);
}

arch_initcall(s5pc110_cpufreq_init);
