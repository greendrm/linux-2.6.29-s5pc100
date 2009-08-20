/* linux/arch/arm/mach-s5p6442/mach-smdk6442.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/partitions.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>

#include <asm/irq.h>
#include <asm/mach-types.h>
#include <asm/setup.h>

#include <plat/regs-serial.h>
#include <plat/iic.h>
#include <plat/fb.h>

#include <plat/nand.h>
#include <plat/partition.h>
#include <plat/s5p6442.h>
#include <plat/clock.h>
#include <plat/regs-clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/adc.h>
#include <plat/pm.h>
#include <plat/pll.h>
#include <plat/regs-rtc.h>
#include <plat/spi.h>

#include <mach/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>

#ifdef CONFIG_USB_SUPPORT
#include <plat/regs-otg.h>
#include <linux/usb/ch9.h>

/* S3C_USB_CLKSRC 0: EPLL 1: CLK_48M */
#define S3C_USB_CLKSRC	1
#define OTGH_PHY_CLK_VALUE      (0x02)  /* UTMI Interface, Cristal, 12Mhz clk for PLL */
#endif

#define UCON S3C2410_UCON_DEFAULT | S3C2410_UCON_UCLK
#define ULCON S3C2410_LCON_CS8 | S3C2410_LCON_PNONE | S3C2410_LCON_STOPB
#define UFCON S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE

extern void s3c64xx_reserve_bootmem(void);

extern struct sys_timer s5p64xx_timer;

#if defined(CONFIG_SPI_CNTRLR_0) || defined(CONFIG_SPI_CNTRLR_1)
static void s3c_cs_suspend(int pin, pm_message_t pm)
{
	/* Whatever need to be done */
}

static void s3c_cs_resume(int pin)
{
	/* Whatever need to be done */
}

static void cs_set(int pin, int lvl)
{
	unsigned int val;

	val = __raw_readl(S5P64XX_GPNDAT);
	val &= ~(1<<pin);

	if(lvl == CS_HIGH)
	   val |= (1<<pin);

	__raw_writel(val, S5P64XX_GPNDAT);
}

static void s3c_cs_setF13(int pin, int lvl)
{
	cs_set(13, lvl);
}

static void s3c_cs_setF14(int pin, int lvl)
{
	cs_set(14, lvl);
}

static void s3c_cs_setF15(int pin, int lvl)
{
	cs_set(15, lvl);
}

static void cs_cfg(int pin, int pull)
{
	unsigned int val;

	val = __raw_readl(S5P64XX_GPNCON);
	val &= ~(3<<(pin*2));
	val |= (1<<(pin*2)); /* Output Mode */
	__raw_writel(val, S5P64XX_GPNCON);

	val = __raw_readl(S5P64XX_GPNPUD);
	val &= ~(3<<(pin*2));

	if(pull == CS_HIGH)
	   val |= (2<<(pin*2));	/* PullUp */
	else
	   val |= (1<<(pin*2)); /* PullDown */

	__raw_writel(val, S5P64XX_GPNPUD);
}

static void s3c_cs_configF13(int pin, int mode, int pull)
{
	cs_cfg(13, pull);
}

static void s3c_cs_configF14(int pin, int mode, int pull)
{
	cs_cfg(14, pull);
}

static void s3c_cs_configF15(int pin, int mode, int pull)
{
	cs_cfg(15, pull);
}

static void s3c_cs_set(int pin, int lvl)
{
	if(lvl == CS_HIGH)
	   s3c_gpio_setpin(pin, 1);
	else
	   s3c_gpio_setpin(pin, 0);
}

static void s3c_cs_config(int pin, int mode, int pull)
{
	s3c_gpio_cfgpin(pin, mode);

	if(pull == CS_HIGH)
	   s3c_gpio_setpull(pin, S3C_GPIO_PULL_UP);
	else
	   s3c_gpio_setpull(pin, S3C_GPIO_PULL_DOWN);
}
#endif

#if defined(CONFIG_SPI_CNTRLR_0)
static struct s3c_spi_pdata s3c_slv_pdata_0[] __initdata = {
	[0] = {	/* Slave-0 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5P64XX_GPC(3),
		.cs_mode      = S5P64XX_GPC_OUTPUT(3),
		.cs_set       = s3c_cs_set,
		.cs_config    = s3c_cs_config,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
	[1] = {	/* Slave-1 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5P64XX_GPF(13),
		.cs_mode      = S5P64XX_GPF_OUTPUT(13),
		.cs_set       = s3c_cs_setF13,
		.cs_config    = s3c_cs_configF13,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
};
#endif

#if defined(CONFIG_SPI_CNTRLR_1)
static struct s3c_spi_pdata s3c_slv_pdata_1[] __initdata = {
	[0] = {	/* Slave-0 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5P64XX_GPC(7),
		.cs_mode      = S5P64XX_GPC_OUTPUT(7),
		.cs_set       = s3c_cs_set,
		.cs_config    = s3c_cs_config,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
	[1] = {	/* Slave-1 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5P64XX_GPF(14),
		.cs_mode      = S5P64XX_GPF_OUTPUT(14),
		.cs_set       = s3c_cs_setF14,
		.cs_config    = s3c_cs_configF14,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
	[2] = {	/* Slave-2 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5P64XX_GPF(15),
		.cs_mode      = S5P64XX_GPF_OUTPUT(15),
		.cs_set       = s3c_cs_setF15,
		.cs_config    = s3c_cs_configF15,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
};
#endif

static struct spi_board_info s3c_spi_devs[] __initdata = {
#if defined(CONFIG_SPI_CNTRLR_0)
	[0] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-0 as 1st Slave */
		.bus_num	 = 0,
		.irq		 = IRQ_SPI0,
		.chip_select	 = 0,
	},
	[1] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-0 as 2nd Slave */
		.bus_num	 = 0,
		.irq		 = IRQ_SPI0,
		.chip_select	 = 1,
	},
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
	[2] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-1 as 1st Slave */
		.bus_num	 = 1,
		.irq		 = IRQ_SPI1,
		.chip_select	 = 0,
	},
	[3] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-1 as 2nd Slave */
		.bus_num	 = 1,
		.irq		 = IRQ_SPI1,
		.chip_select	 = 1,
	},
	[4] = {
		.modalias	 = "mmc_spi", /* MMC SPI */
		.mode		 = SPI_MODE_0 | SPI_CS_HIGH,	/* CPOL=0, CPHA=0 & CS is Active High */
		.max_speed_hz    = 100000,
		/* Connected to SPI-1 as 3rd Slave */
		.bus_num	 = 1,
		.irq		 = IRQ_SPI1,
		.chip_select	 = 2,
	},
#endif
};

static struct s3c2410_uartcfg smdk6442_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
	},
	[2] = {
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
	},
};

#ifdef CONFIG_FB_S3C_AMS320
static void ams320_cfg_gpio(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5P64XX_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5P64XX_GPF0(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5P64XX_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5P64XX_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5P64XX_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5P64XX_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5P64XX_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5P64XX_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* drive strength to max */
	writel(0xffffffff, S5P64XX_VA_GPIO + 0x12c);
	writel(0xffffffff, S5P64XX_VA_GPIO + 0x14c);
	writel(0xffffffff, S5P64XX_VA_GPIO + 0x16c);
	writel(0x000000ff, S5P64XX_VA_GPIO + 0x18c);

	writel(0x10, S5P_MDNIE_SEL);

	udelay(200);

	s3c_gpio_cfgpin(S5P64XX_GPH3(5), S3C_GPIO_SFN(1));	/* LCD RESET */
	s3c_gpio_cfgpin(S5P64XX_GPF3(5), S3C_GPIO_SFN(1));	/* SPI CS */
	s3c_gpio_cfgpin(S5P64XX_GPD1(0), S3C_GPIO_SFN(1));	/* SPI Clock */
	s3c_gpio_cfgpin(S5P64XX_GPD1(1), S3C_GPIO_SFN(1));	/* SPI Data */

	s3c_gpio_setpull(S5P64XX_GPH3(5), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPF3(5), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPD1(0), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPD1(1), S3C_GPIO_PULL_NONE);
}

static int ams320_reset_lcd(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5P64XX_GPH3(5), "GPH3");
	if (err) {
		printk(KERN_ERR "failed to request GPH3 for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5P64XX_GPH3(5), 1);
	mdelay(3);

	gpio_set_value(S5P64XX_GPH3(5), 0);
	mdelay(20);

	gpio_set_value(S5P64XX_GPH3(5), 1);

	gpio_free(S5P64XX_GPH3(5));

	return 0;
}

static struct s3c_platform_fb ams320_data __initdata = {
	.hw_ver	= 0x60,
	.clk_name = "lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_HWORD | FB_SWAP_WORD,

	.cfg_gpio = ams320_cfg_gpio,
	.reset_lcd = ams320_reset_lcd,
};

#define LCD_BUS_NUM 	3
#define DISPLAY_CS	S5P64XX_GPF3(5)
static struct spi_board_info spi_board_info[] __initdata = {
    	{
	    	.modalias	= "ams320",
		.platform_data	= NULL,
		.max_speed_hz	= 300,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};

#define DISPLAY_CLK	S5P64XX_GPD1(1)
#define DISPLAY_SI	S5P64XX_GPD1(0)
static struct spi_gpio_platform_data ams320_spi_gpio_data = {
	.sck	= DISPLAY_CLK,
	.mosi	= DISPLAY_SI,
	.miso	= -1,
	.num_chipselect	= 1,
};

static struct platform_device s3c_device_spi_gpio = {
	.name	= "spi_gpio",
	.id	= LCD_BUS_NUM,
	.dev	= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &ams320_spi_gpio_data,
	},
};
#endif

struct map_desc smdk6442_iodesc[] = {};

static struct platform_device *smdk6442_devices[] __initdata = {
	&s3c_device_fb,

#ifdef CONFIG_FB_S3C_AMS320
	&s3c_device_spi_gpio,
#endif

//	&s3c_device_wdt,
//	&s3c_device_rtc,
//	&s3c_device_i2c0,
//	&s3c_device_i2c1,

	&s3c_device_ts,
	&s3c_device_keypad,
	&s3c_device_smc911x,
//	&s3c_device_lcd,
//	&s3c_device_nand,

#ifdef CONFIG_S5P64XX_ADC
	&s3c_device_adc,
#endif

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
#endif

#if defined CONFIG_USB_GADGET_S3C_OTGD 
	&s3c_device_usbgadget,
#endif
};

static struct i2c_board_info i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c08", 0x50), },
	//{ I2C_BOARD_INFO("WM8580", 0x10), },
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	{ I2C_BOARD_INFO("24c128", 0x57), },	/* Samsung S524AD0XD1 */
	{ I2C_BOARD_INFO("WM8580", 0x1a), },
	//{ I2C_BOARD_INFO("WM8580", 0x1b), },
};

static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.delay 			= 10000,
	.presc 			= 49,
	.oversampling_shift	= 2,
	.resol_bit 		= 12,
	.s3c_adc_con		= ADC_TYPE_2,
};

static struct s3c_adc_mach_info s3c_adc_platform = {
	/* s3c6410 support 12-bit resolution */
	.delay	= 	10000,
	.presc 	= 	49,
	.resolution = 	12,
};

#if defined(CONFIG_HAVE_PWM)
static struct platform_pwm_backlight_data smdk_backlight_data = {
	.pwm_id		= 1,
	.max_brightness	= 255,
	.dft_brightness	= 255,
	.pwm_period_ns	= 78770,
};

static struct platform_device smdk_backlight_device = {
	.name		= "pwm-backlight",
	.dev		= {
		.parent = &s3c_device_timer[1].dev,
		.platform_data = &smdk_backlight_data,
	},
};

static void __init smdk_backlight_register(void)
{
	int ret = platform_device_register(&smdk_backlight_device);
	if (ret)
		printk(KERN_ERR "smdk: failed to register backlight device: %d\n", ret);
}
#else
#define smdk_backlight_register()	do { } while (0)
#endif

void smdk6442_setup_sdhci0 (void);

static void __init smdk6442_map_io(void)
{
	s3c_device_nand.name = "s5p6442-nand";

	s5p64xx_init_io(smdk6442_iodesc, ARRAY_SIZE(smdk6442_iodesc));
	s3c24xx_init_clocks(12000000);
	s3c24xx_init_uarts(smdk6442_uartcfgs, ARRAY_SIZE(smdk6442_uartcfgs));

	s3c64xx_reserve_bootmem();
}

static void __init smdk6442_smc911x_set(void)
{
	unsigned int tmp;

	tmp = __raw_readl(S5P64XX_VA_GPIO + 0x2A0);
	tmp &= ~(0xf << 20);
	tmp |= (0x2 << 20);
	__raw_writel(tmp,(S5P64XX_VA_GPIO + 0x2A0));

	tmp = __raw_readl(S5P64XX_SROM_BW);
	tmp &= ~(0xf << 20);
	tmp |= S5P64XX_SROM_BW_DATA_WIDTH5_16BIT | (1 << 21);

	__raw_writel(tmp, S5P64XX_SROM_BW);

	__raw_writel(S5P64XX_SROM_BCn_TACS(0) | S5P64XX_SROM_BCn_TCOS(4) |
			S5P64XX_SROM_BCn_TACC(13) | S5P64XX_SROM_BCn_TCOH(1) |
			S5P64XX_SROM_BCn_TCAH(4) | S5P64XX_SROM_BCn_TACP(6) |
			S5P64XX_SROM_BCn_PMC_NORMAL, S5P64XX_SROM_BC5);

	tmp = gpio_request(S5P64XX_GPH1(3),"GPH1");

	if (tmp){
		printk("gpio(GPH1_3) request error : %d\n",tmp);
	}else{
		s3c_gpio_cfgpin(S5P64XX_GPH1(3),S5P64XX_GPH1_3_EXT_INT1_3);
	}

}

static void __init smdk6442_machine_init(void)
{
//	s3c_device_nand.dev.platform_data = &s3c_nand_mtd_part_info;

	smdk6442_smc911x_set();

#ifdef CONFIG_FB_S3C_AMS320
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&ams320_data);
#endif

//	s3c_i2c0_set_platdata(NULL);
//	s3c_i2c1_set_platdata(NULL);

	s3c_ts_set_platdata(&s3c_ts_platform);
	s3c_adc_set_platdata(&s3c_adc_platform);

//	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
//	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

	/* fb */
//	s3cfb_set_platdata(&fb_data);

	platform_add_devices(smdk6442_devices, ARRAY_SIZE(smdk6442_devices));

//	s5p6442_pm_init();

//	smdk_backlight_register();

}

static void __init smdk6442_fixup(struct machine_desc *desc,
					struct tag *tags, char **cmdline,
					struct meminfo *mi)
{

	mi->bank[0].start = 0x20000000;
	mi->bank[0].size = 128 * SZ_1M;
	mi->bank[0].node = 0;

	mi->nr_banks = 1;

}

MACHINE_START(SMDK6442, "SMDK6442")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P64XX_PA_SDRAM + 0x100,
	.fixup		= smdk6442_fixup,
	.init_irq	= s5p6442_init_irq,
	.map_io		= smdk6442_map_io,
	.init_machine	= smdk6442_machine_init,
	.timer		= &s5p64xx_timer,
MACHINE_END

#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void) {
	writel(1, S5P_USB_PHY_CONTROL);
	writel(0xa0, S3C_USBOTG_PHYPWR);		/* Power up */
        writel(OTGH_PHY_CLK_VALUE, S3C_USBOTG_PHYCLK);
	writel(0x1, S3C_USBOTG_RSTCON);
	udelay(50);
	writel(0x0, S3C_USBOTG_RSTCON);
	udelay(50);
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void) {
	writel(readl(S3C_USBOTG_PHYPWR)|(0x1F<<1), S3C_USBOTG_PHYPWR);
	writel(0, S5P_USB_PHY_CONTROL);
}
EXPORT_SYMBOL(otg_phy_off);

#endif

#if defined(CONFIG_RTC_DRV_S3C)
/* RTC common Function for samsung APs*/
unsigned int s3c_rtc_set_bit_byte(void __iomem *base, uint offset, uint val)
{
	writeb(val, base + offset);

	return 0;
}

unsigned int s3c_rtc_read_alarm_status(void __iomem *base)
{
	return 1;
}

void s3c_rtc_set_pie(void __iomem *base, uint to)
{
	unsigned int tmp;

	tmp = readw(base + S3C2410_RTCCON) & ~S3C_RTCCON_TICEN;

        if (to)
                tmp |= S3C_RTCCON_TICEN;

        writew(tmp, base + S3C2410_RTCCON);
}

void s3c_rtc_set_freq_regs(void __iomem *base, uint freq, uint s3c_freq)
{
	unsigned int tmp;

        tmp = readw(base + S3C2410_RTCCON) & (S3C_RTCCON_TICEN | S3C2410_RTCCON_RTCEN );
        writew(tmp, base + S3C2410_RTCCON);
        s3c_freq = freq;
        tmp = (32768 / freq)-1;
        writel(tmp, base + S3C2410_TICNT);
}

void s3c_rtc_enable_set(struct platform_device *pdev,void __iomem *base, int en)
{
	unsigned int tmp;

	if (!en) {
		tmp = readw(base + S3C2410_RTCCON);
		writew(tmp & ~ (S3C2410_RTCCON_RTCEN | S3C_RTCCON_TICEN), base + S3C2410_RTCCON);
	} else {
		/* re-enable the device, and check it is ok */
		if ((readw(base+S3C2410_RTCCON) & S3C2410_RTCCON_RTCEN) == 0){
			dev_info(&pdev->dev, "rtc disabled, re-enabling\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp|S3C2410_RTCCON_RTCEN, base+S3C2410_RTCCON);
		}

		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CNTSEL)){
			dev_info(&pdev->dev, "removing RTCCON_CNTSEL\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp& ~S3C2410_RTCCON_CNTSEL, base+S3C2410_RTCCON);
		}

		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CLKRST)){
			dev_info(&pdev->dev, "removing RTCCON_CLKRST\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp & ~S3C2410_RTCCON_CLKRST, base+S3C2410_RTCCON);
		}
	}
}
#endif

#if defined(CONFIG_KEYPAD_S3C) || defined (CONFIG_KEYPAD_S3C_MODULE)
void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;

	end = S5P64XX_GPH3(rows);

	/* Set all the necessary GPH2 pins to special-function 0 */
	for (gpio = S5P64XX_GPH3(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	}

	end = S5P64XX_GPH2(columns);

	/* Set all the necessary GPK pins to special-function 0 */
	for (gpio = S5P64XX_GPH2(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		//s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}

EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);
#endif


