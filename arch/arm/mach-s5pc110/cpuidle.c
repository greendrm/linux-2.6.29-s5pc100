/* arch/arm/mach-s5pc110/cpuidle.c
 * 
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *		
 * CPU idle driver for S5PC110
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/cpuidle.h>
#include <linux/io.h>
#include <asm/proc-fns.h>
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>

#include <mach/map.h>
#include <mach/regs-irq.h>
#include <plat/regs-clock.h>
#include <plat/pm.h>
#include <plat/regs-serial.h>
#include <mach/cpuidle.h>
#include <plat/regs-hsmmc.h>
#include <plat/regs-fb.h>
#include <plat/devs.h>

#include <mach/dma.h>
#include <plat/regs-otg.h>
#include <plat/regs-gpio.h>
#include <plat/dma.h>

#define S5PC110_MAX_STATES	1

extern void s5pc110_deepidle(void);

/* For saving & restoring VIC register before entering
 * deep-idle mode
 **/
static unsigned long vic_regs[4];
static unsigned long *regs_save;
static dma_addr_t phy_regs_save;

#define MAX_CHK_DEV	0xf

/* Specific device list for checking before entering
 * deep-idle mode
 **/
struct check_device_op {
	void __iomem		*base;
	struct platform_device	*pdev;

};

/* Array of checking devices list */
static struct check_device_op chk_dev_op[] = {
	{.base = 0, .pdev = &s3c_device_hsmmc0},
	{.base = 0, .pdev = &s3c_device_hsmmc1},
	{.base = 0, .pdev = &s3c_device_hsmmc2},
	{.base = 0, .pdev = &s3c_device_hsmmc3},
	{.base = 0, .pdev = &s3c_device_onenand},
	{.base = 0, .pdev = &s3c_device_i2c0},
	{.base = 0, .pdev = &s3c_device_i2c1},
	{.base = 0, .pdev = &s3c_device_i2c2},
	{.base = 0, .pdev = NULL},
};

/* Check if wether Framebuffer is working or not
 * if not retunr "0"
 **/
static int check_fb_op(void)
{
	unsigned long val;

	val = __raw_readl(S5PC11X_VA_LCD + S3C_VIDCON0);

	if (val & (S3C_VIDCON0_ENVID_ENABLE | S3C_VIDCON0_ENVID_F_ENABLE))
		return 1;
	else
		return 0;
}

/* If SD/MMC interface is working: return = 1 or not 0 */
static int check_sdmmc_op(unsigned int ch)
{
	unsigned int reg1, reg2;
	void __iomem *base_addr;

	if (unlikely(ch > 3)) {
		printk(KERN_ERR "Invalid ch[%d] for SD/MMC \n", ch);
		return 0;
	}

	base_addr = chk_dev_op[ch].base;
	/* Check CMDINHDAT[1] and CMDINHCMD [0] */
	reg1 = readl(base_addr + S3C_HSMMC_PRNSTS);
	/* Check CLKCON [2]: ENSDCLK */
	reg2 = readl(base_addr + S3C_HSMMC_CLKCON);

	if ((reg1 & (S3C_HSMMC_CMD_INHIBIT | S3C_HSMMC_DATA_INHIBIT)) ||
			(reg2 & (S3C_HSMMC_CLOCK_CARD_EN))) {
		printk(KERN_INFO "sdmmc[%d] is working\n", ch);
		return 1;
	} else {
		return 0;
	}
}

/* Check all sdmmc controller */
static int loop_sdmmc_check(void)
{
	unsigned int iter;

	for (iter = 0; iter < 4; iter++) {
		if (check_sdmmc_op(iter))
			return 1;
	}
	return 0;
}

/* Check onenand is working or not */

/* ONENAND_IF_STATUS(0xB060010C)
 * ORWB[0] = 	1b : busy
 * 		0b : Not busy
 **/
static int check_onenand_op(void)
{
	unsigned int val;
	void __iomem *base_addr;

	base_addr = chk_dev_op[4].base;

	val = __raw_readl(base_addr + 0x10c);

	if (val & 0x1) {
		printk(KERN_INFO "Onenand is working\n");
		return 1;
	}
	return 0;
}

/* Check P/MDMA is working or not */
static void __iomem *dma_base[S3C_DMA_CONTROLLERS];

static int check_dma_op(void)
{
	int i, j;
	unsigned int val;

	for (i = 0 ; i < S3C_DMA_CONTROLLERS ; i++) {

		for (j = 0 ; j < S3C_CHANNELS_PER_DMA ; j++) {
			val = __raw_readl(dma_base[i] + S3C_DMAC_CS(j));
			if (val & 0xf) {
				printk(KERN_INFO "DMA[%d][%d] is working\n", i, j);
				return 1;
			}
		}
	}

	return 0;
}

/* Check USBOTG is woring or not*/
static int check_usbotg_op(void)
{
	unsigned int val;

	val = __raw_readl(S3C_UDC_OTG_GOTGCTL);

	if (val & 0x3) {
		printk(KERN_INFO "USBOTG is working\n");
		return 1;
	}
	return 0;
}

/* Check I2C */
static int check_i2c_op(void)
{
	unsigned int val, ch;
	void __iomem *base_addr;

	for (ch = 0; ch < 3; ch++) {

		base_addr = chk_dev_op[(5 + ch)].base;

		val = __raw_readl(base_addr + 0x04);

		if (val & (1<<5)) {
			printk(KERN_INFO "I2C ch:%d is working\n", ch);
			return 1;
		}
	}

	return 0;
}

/* Before entering, deep-idle mode GPIO Powe Down Mode
 * Configuration register has to be set with same state
 * in Normal Mode
 **/
#define GPIO_OFFSET		0x20
#define GPIO_CON_PDN_OFFSET	0x10
#define GPIO_PUD_PDN_OFFSET	0x14
#define GPIO_PUD_OFFSET		0x08

static void s5pc110_gpio_pdn_conf(void)
{
	void __iomem *gpio_base = S5PC11X_GPA0_BASE;
	unsigned int val;

	do {
		/* Keep the previous state in deep-idle mode */
		__raw_writel(0xffff, gpio_base + GPIO_CON_PDN_OFFSET);

		/* Pull up-down state in deep-idle is same as normal */
		val = __raw_readl(gpio_base + GPIO_PUD_OFFSET);
		__raw_writel(val, gpio_base + GPIO_PUD_PDN_OFFSET);

		gpio_base += GPIO_OFFSET;

	} while (gpio_base <= S5PC11X_MP28_BASE);
}

static void s5pc110_enter_idle(void)
{
	unsigned long tmp;

	tmp = __raw_readl(S5P_IDLE_CFG);
	tmp &= ~((3<<30)|(3<<28)|(1<<0));
	tmp |= ((2<<30)|(2<<28));
	__raw_writel(tmp, S5P_IDLE_CFG);

	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;
	__raw_writel(tmp, S5P_PWR_CFG);

	cpu_do_idle();
}

static void s5pc110_enter_deepidle(void)
{
	unsigned long tmp;

	/* store the physical address of the register recovery block */
	__raw_writel(phy_regs_save, S5P_INFORM2);

	__raw_writel(DEEPIDLE_MODE, S5P_INFORM1);
	/* ensure INF_REG0  has the resume address */
	__raw_writel(virt_to_phys(s5pc110_cpu_resume), S5P_INFORM0);

	/* Save current VIC_INT_ENABLE register*/
	vic_regs[0] = __raw_readl(S5PC110_VIC0REG(VIC_INT_ENABLE));
	vic_regs[1] = __raw_readl(S5PC110_VIC1REG(VIC_INT_ENABLE));
	vic_regs[2] = __raw_readl(S5PC110_VIC2REG(VIC_INT_ENABLE));
	vic_regs[3] = __raw_readl(S5PC110_VIC3REG(VIC_INT_ENABLE));

	/* Disable all interrupt through VIC */
	__raw_writel(0xffffffff, S5PC110_VIC0REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC1REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC2REG(VIC_INT_ENABLE_CLEAR));
	__raw_writel(0xffffffff, S5PC110_VIC3REG(VIC_INT_ENABLE_CLEAR));

	/* GPIO Power Down Control */
	s5pc110_gpio_pdn_conf();

	/* Wakeup source configuration for deep-idle */
	tmp = __raw_readl(S5P_WAKEUP_MASK);
	/* Wakeup sources are all enable */
	tmp &= ~0xffff;
	__raw_writel(tmp, S5P_WAKEUP_MASK);

	/* Clear wakeup status register */
	tmp = __raw_readl(S5P_WAKEUP_STAT);
	__raw_writel(tmp, S5P_WAKEUP_STAT);

	/* IDLE config register set */
	/* TOP Memory retention off */
	/* TOP Memory LP mode       */
	/* ARM_L2_Cacheret on       */
	tmp = __raw_readl(S5P_IDLE_CFG);
	tmp &= ~(0x3f << 26);
	tmp |= ((1<<30) | (1<<28) | (1<<26) | (1<<0));
	__raw_writel(tmp, S5P_IDLE_CFG);

	/* Entering deep-idle mode with WFI instruction */
	if (s5pc110_cpu_save(regs_save) == 0)
		s5pc110_deepidle();

	tmp = __raw_readl(S5P_IDLE_CFG);
	tmp &= ~((3<<30)|(3<<28)|(3<<26)|(1<<0));
	tmp |= ((2<<30)|(2<<28));
	__raw_writel(tmp, S5P_IDLE_CFG);

	/* Release retention GPIO/MMC/UART IO */
	tmp = __raw_readl(S5P_OTHERS);
	tmp |= ((1<<31) | (1<<30) | (1<<29) | (1<<28));
	__raw_writel(tmp, S5P_OTHERS);

	__raw_writel(vic_regs[0], S5PC110_VIC0REG(VIC_INT_ENABLE));
	__raw_writel(vic_regs[1], S5PC110_VIC1REG(VIC_INT_ENABLE));
	__raw_writel(vic_regs[2], S5PC110_VIC2REG(VIC_INT_ENABLE));
	__raw_writel(vic_regs[3], S5PC110_VIC3REG(VIC_INT_ENABLE));
}

static struct cpuidle_driver s5pc110_idle_driver = {
	.name =         "s5pc110_idle",
	.owner =        THIS_MODULE,
};

static DEFINE_PER_CPU(struct cpuidle_device, s5pc110_cpuidle_device);

/* Actual code that puts the SoC in different idle states */
static int s5pc110_enter_idle_normal(struct cpuidle_device *dev,
			       struct cpuidle_state *state)
{
	struct timeval before, after;
	int idle_time;

	local_irq_disable();
	do_gettimeofday(&before);

	s5pc110_enter_idle();

	do_gettimeofday(&after);
	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
			(after.tv_usec - before.tv_usec);
	return idle_time;
}

static int s5pc110_idle_bm_check(void)
{
	if (loop_sdmmc_check() || check_fb_op() ||
			check_onenand_op() || check_dma_op() ||
			check_usbotg_op() || check_i2c_op())
		return 1;
	else
		return 0;

}

/* Actual code that puts the SoC in different idle states */
static int s5pc110_enter_idle_lpaudio(struct cpuidle_device *dev,
			       struct cpuidle_state *state)
{
	struct timeval before, after;
	int idle_time;

	local_irq_disable();
	do_gettimeofday(&before);

	s5pc110_enter_deepidle();

	do_gettimeofday(&after);
	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
			(after.tv_usec - before.tv_usec);
	return idle_time;
}

static int s5pc110_enter_idle_bm(struct cpuidle_device *dev,
				struct cpuidle_state *state)
{
	struct cpuidle_state *new_state = state;

	if (s5pc110_idle_bm_check()) {
		BUG_ON(!dev->safe_state);
		new_state = dev->safe_state;
	}

	dev->last_state = new_state;

	if (new_state == &dev->states[0])
		return s5pc110_enter_idle_normal(dev, new_state);
	else
		return s5pc110_enter_idle_lpaudio(dev, new_state);
}

int s5pc110_setup_lpaudio(unsigned int mode)
{
	struct cpuidle_device *device;

	int ret = 0;

	cpuidle_pause_and_lock();
	device = &per_cpu(s5pc110_cpuidle_device, smp_processor_id());
	cpuidle_disable_device(device);

	switch (mode) {
	case NORMAL_MODE:
		device->state_count = 1;
		/* Wait for interrupt state */
		device->states[0].enter = s5pc110_enter_idle_normal;
		device->states[0].exit_latency = 1;	/* uS */
		device->states[0].target_residency = 10000;
		device->states[0].flags = CPUIDLE_FLAG_TIME_VALID;
		strcpy(device->states[0].name, "IDLE");
		strcpy(device->states[0].desc, "ARM clock gating - WFI");
		break;
	case LPAUDIO_MODE:
		device->state_count = 2;
		/* Wait for interrupt state */
		device->states[0].enter = s5pc110_enter_idle_normal;
		device->states[0].exit_latency = 1;	/* uS */
		device->states[0].target_residency = 10000;
		device->states[0].flags = CPUIDLE_FLAG_TIME_VALID;
		strcpy(device->states[0].name, "IDLE");
		strcpy(device->states[0].desc, "S5PC110 normal idle");

		/* Wait for interrupt and TOP block retention */
		device->states[1].enter = s5pc110_enter_idle_bm;
		device->states[1].exit_latency = 30;	/* uS */
		device->states[1].target_residency = 10000;
		device->states[1].flags = CPUIDLE_FLAG_TIME_VALID |
						CPUIDLE_FLAG_CHECK_BM;
		strcpy(device->states[1].name, "DEEP IDLE");
		strcpy(device->states[1].desc, "S5PC110 deep idle");

		/* Set safe_state of second idle */
		device->safe_state = &device->states[0];

		break;
	default:
		printk(KERN_ERR "Can't find cpuidle mode %d\n", mode);
		device->state_count = 1;

		/* Wait for interrupt state */
		device->states[0].enter = s5pc110_enter_idle_normal;
		device->states[0].exit_latency = 1;	/* uS */
		device->states[0].target_residency = 10000;
		device->states[0].flags = CPUIDLE_FLAG_TIME_VALID;
		strcpy(device->states[0].name, "IDLE");
		strcpy(device->states[0].desc, "ARM clock gating - WFI");
		break;
	}
	cpuidle_enable_device(device);
	cpuidle_resume_and_unlock();

	return ret;
}
EXPORT_SYMBOL(s5pc110_setup_lpaudio);

/* Initialize CPU idle by registering the idle states */
static int s5pc110_init_cpuidle(void)
{
	struct cpuidle_device *device;
	struct platform_device *pdev;
	struct resource *res;
	int i = 0;

	cpuidle_register_driver(&s5pc110_idle_driver);

	device = &per_cpu(s5pc110_cpuidle_device, smp_processor_id());
	device->state_count = 1;

	/* Wait for interrupt state */
	device->states[0].enter = s5pc110_enter_idle_normal;
	device->states[0].exit_latency = 1;	/* uS */
	device->states[0].target_residency = 10000;
	device->states[0].flags = CPUIDLE_FLAG_TIME_VALID;
	strcpy(device->states[0].name, "IDLE");
	strcpy(device->states[0].desc, "ARM clock gating - WFI");

	if (cpuidle_register_device(device)) {
		printk(KERN_ERR "s5pc110_init_cpuidle: Failed registering\n");
		return -EIO;
	}
	regs_save = dma_alloc_coherent(NULL, 4096, &phy_regs_save, GFP_KERNEL);
	if (regs_save == NULL) {
		printk(KERN_ERR "DMA alloc error\n");
		return -ENOMEM;
	}
	printk(KERN_INFO "cpuidle: phy_regs_save:0x%x\n", phy_regs_save);

	/* Allocate memory region to access IP's directly */
	for (i = 0 ; i < MAX_CHK_DEV ; i++) {

		pdev = chk_dev_op[i].pdev;

		if (pdev == NULL)
			break;

		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res) {
			printk(KERN_ERR "failed to get io memory region\n");
			return -EINVAL;
		}
		/* ioremap for register block */
		chk_dev_op[i].base = ioremap(res->start, 4096);

		if (!chk_dev_op[i].base) {
			printk(KERN_ERR "failed to remap io region\n");
			return -EINVAL;
		}
	}

	/* M,PDMA0,1 controller memory region allocation */
	dma_base[0] = ioremap(S3C_PA_DMA, 4096);
	if (dma_base[0] == NULL) {
		printk(KERN_ERR "M2M-DMA ioremap failed\n");
		return -EINVAL;
	}

	dma_base[1] = ioremap(S3C_PA_PDMA, 4096);
	if (dma_base[1] == NULL) {
		printk(KERN_ERR "PDMA0 ioremap failed\n");
		return -EINVAL;
	}

	dma_base[2] = ioremap(S3C_PA_PDMA + 0x100000, 4096);
	if (dma_base[2] == NULL) {
		printk(KERN_ERR "PDMA1 ioremap failed\n");
		return -EINVAL;
	}

	return 0;
}

device_initcall(s5pc110_init_cpuidle);
