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

extern int max8698_set_dvsarm1(unsigned char val);
extern int max8698_set_dvsarm2(unsigned char val);
extern int max8698_set_dvsarm3(unsigned char val);
extern int max8698_set_dvsarm4(unsigned char val);
extern int max8698_set_dvsint1(unsigned char val);
extern int max8698_set_dvsint2(unsigned char val);

static struct clk * mpu_clk;
static struct regulator *arm_regulator;
static struct regulator *internal_regulator;
static unsigned long set1_gpio;
static unsigned long set2_gpio;
static unsigned long set3_gpio;


/* frequency */
static struct cpufreq_frequency_table s5pc110_freq_table[] = {
	{L0, 800*1000},
	{L1, 400*1000},
	{L2, 200*1000},
	{L3, 100*1000},
//	{L4, 83*1000},
	{0, CPUFREQ_TABLE_END},
};

#define DVSARM1	(1<<0)	//set1_gpio:LOW, set2_gpio:LOW
#define DVSARM2 (1<<1)	//set1_gpio:HIGH, set2_gpio:LOW
#define DVSARM3 (1<<2)	//set1_gpio:LOW, set2_gpio:HIGH
#define DVSARM4 (1<<3)	//set1_gpio:HIGH, set2_gpio:HIGH
#define DVSINT1	(1<<4)	//set3_gpio:LOW
#define DVSINT2	(1<<5)	//set3_gpio:HIGH


static const int s5pc110_volt_table[][3] = {
	{L0, DVSARM1, DVSINT1},
	{L1, DVSARM2, DVSINT1},
	{L2, DVSARM3, DVSINT1},
	{L3, DVSARM4, DVSINT1},
	/*{Lv, VDD_ARM(uV), VDD_INT(uV)}*/
};

static void s5pc110_set_volt(unsigned long val)
{
	
	switch(val) {
	case DVSARM1:
		gpio_set_value(set1_gpio, 0);
		gpio_set_value(set2_gpio, 0);
		break;
	case DVSARM2:
		gpio_set_value(set1_gpio, 1);
		gpio_set_value(set2_gpio, 0);
		break;
	case DVSARM3:
		gpio_set_value(set1_gpio, 0);
		gpio_set_value(set2_gpio, 1);
		break;
	case DVSARM4:
		gpio_set_value(set1_gpio, 1);
		gpio_set_value(set2_gpio, 1);
		break;
	case DVSINT1:
		gpio_set_value(set3_gpio, 0);
		break;
	case DVSINT2:
		gpio_set_value(set3_gpio, 1);
		break;
	default:
		printk(KERN_ERR "[%s]Voltage configuraiotn error! \n",__FUNCTION__);
		break;
	}
}

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
	unsigned int index,reg,arm_volt,int_volt;

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
	
	arm_volt = s5pc110_volt_table[index][1];
	int_volt = s5pc110_volt_table[index][2];
	
	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	if (freqs.new > freqs.old) {
		// Voltage up code
#if 0
		regulator_set_voltage(arm_regulator, arm_volt, arm_volt);
		regulator_set_voltage(internal_regulator, int_volt, int_volt);
#else
		s5pc110_set_volt(arm_volt);
		s5pc110_set_volt(int_volt);
#endif
	}
		
	reg = __raw_readl(S5P_CLK_DIV0);
	reg &=~CLK_DIV0_MASK;
	reg |= ((clkdiv0_val[index][0]<<0)|(clkdiv0_val[index][1]<<8)|(clkdiv0_val[index][2]<<12));
	__raw_writel(reg, S5P_CLK_DIV0);


	if (freqs.new < freqs.old) {
		// Voltage down
#if 0	
		regulator_set_voltage(arm_regulator, arm_volt, arm_volt);
		regulator_set_voltage(internal_regulator, int_volt, int_volt);
#else
		s5pc110_set_volt(arm_volt);
		s5pc110_set_volt(int_volt);
#endif
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

	err = gpio_request(S5PC11X_GPH0(4),"GPH0");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		gpio_direction_output(S5PC11X_GPH0(4), 0);
		set3_gpio = S5PC11X_GPH0(4);
	}

	err = gpio_request(S5PC11X_GPH1(6),"GPH1");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		gpio_direction_output(S5PC11X_GPH1(6), 0);
		set1_gpio = S5PC11X_GPH1(6);
	}

	err = gpio_request(S5PC11X_GPH1(7),"GPH1");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		gpio_direction_output(S5PC11X_GPH1(7), 0);
		set2_gpio = S5PC11X_GPH1(7);
	}

	max8698_set_dvsarm1(0x9);	// 1.2v
	max8698_set_dvsarm2(0x7);	// 1.1v
	max8698_set_dvsarm3(0x5);	// 1.0v
	max8698_set_dvsarm4(0x3);	// 0.9v
	max8698_set_dvsint1(0x9);	// 1.2v
	max8698_set_dvsint2(0x7);	// 1.1v
#endif
	

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

late_initcall(s5pc110_cpufreq_init);
