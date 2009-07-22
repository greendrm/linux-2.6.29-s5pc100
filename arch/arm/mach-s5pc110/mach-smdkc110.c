/* linux/arch/arm/mach-s5pc100/mach-smdkc110.c
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
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/setup.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>
#include <mach/gpio.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/regs-rtc.h>
#include <plat/iic.h>
#include <plat/fimc.h>
#include <plat/csis.h>
#include <plat/fb.h>

#include <plat/nand.h>
#include <plat/partition.h>
#include <plat/s5pc110.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/adc.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-clock.h>

#ifdef CONFIG_USB_SUPPORT
#include <plat/regs-otg.h>
#include <plat/pll.h>
#include <linux/usb/ch9.h>

/* S3C_USB_CLKSRC 0: EPLL 1: CLK_48M */
#define S3C_USB_CLKSRC	1
#define OTGH_PHY_CLK_VALUE      (0x22)  /* UTMI Interface, otg_phy input clk 12Mhz Oscillator */
#endif

#if defined(CONFIG_PM)
#include <plat/pm.h>
#endif

#define UCON S3C2410_UCON_DEFAULT | S3C_UCON_PCLK
#define ULCON S3C2410_LCON_CS8 | S3C2410_LCON_PNONE | S3C2410_LCON_STOPB
#define UFCON S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE

extern struct sys_timer s5pc11x_timer;
extern void s5pc11x_reserve_bootmem(void);

static struct s3c24xx_uart_clksrc smdkc110_serial_clocks[] = {
        [0] = {
                .name           = "pclk",
                .divisor        = 1,
                .min_baud       = 0,
                .max_baud       = 0,
        },
};

static struct s3c2410_uartcfg smdkc110_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = smdkc110_serial_clocks,
                .clocks_size = ARRAY_SIZE(smdkc110_serial_clocks),
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = smdkc110_serial_clocks,
                .clocks_size = ARRAY_SIZE(smdkc110_serial_clocks),
	},
        [2] = {
                .hwport      = 2,
                .flags       = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = smdkc110_serial_clocks,
                .clocks_size = ARRAY_SIZE(smdkc110_serial_clocks),
        },
        [3] = {
                .hwport      = 3,
                .flags       = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = smdkc110_serial_clocks,
                .clocks_size = ARRAY_SIZE(smdkc110_serial_clocks),
        },
};

#ifdef CONFIG_FB_S3C_LTE480WV
static void lte480wv_cfg_gpio(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF0(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* mDNIe SEL: why we shall write 0x2 ? */
	writel(0x2, S5P_MDNIE_SEL);

	/* drive strength to max */
	writel(0xffffffff, S5PC11X_GPF0_BASE + 0xc);
	writel(0xffffffff, S5PC11X_GPF1_BASE + 0xc);
	writel(0xffffffff, S5PC11X_GPF2_BASE + 0xc);
	writel(0x000000ff, S5PC11X_GPF3_BASE + 0xc);
}

static int lte480wv_backlight_on(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPD0(3), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPD0(3), 1);
	gpio_free(S5PC11X_GPD0(3));

	return 0;
}

static int lte480wv_reset_lcd(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPH0(6), "GPH0");
	if (err) {
		printk(KERN_ERR "failed to request GPH0 for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPH0(6), 1);
	mdelay(100);

	gpio_set_value(S5PC11X_GPH0(6), 0);
	mdelay(10);

	gpio_set_value(S5PC11X_GPH0(6), 1);
	mdelay(10);

	gpio_free(S5PC11X_GPH0(6));

	return 0;
}

static struct s3c_platform_fb lte480wv_data __initdata = {
	.hw_ver	= 0x60,
	.clk_name = "lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_WORD | FB_SWAP_HWORD,

	.cfg_gpio = lte480wv_cfg_gpio,
	.backlight_on = lte480wv_backlight_on,
	.reset_lcd = lte480wv_reset_lcd,
};
#endif

#ifdef CONFIG_FB_S3C_TL2796
static void tl2796_cfg_gpio(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF0(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5PC11X_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PC11X_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* mDNIe SEL: why we shall write 0x2 ? */
	writel(0x2, S5P_MDNIE_SEL);

	/* drive strength to max */
	writel(0xffffffff, S5PC11X_VA_GPIO + 0x12c);
	writel(0xffffffff, S5PC11X_VA_GPIO + 0x14c);
	writel(0xffffffff, S5PC11X_VA_GPIO + 0x16c);
	writel(0x000000ff, S5PC11X_VA_GPIO + 0x18c);

	s3c_gpio_cfgpin(S5PC11X_GPB(4), S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(S5PC11X_GPB(5), S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(S5PC11X_GPB(6), S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(S5PC11X_GPB(7), S3C_GPIO_SFN(1));

	s3c_gpio_setpull(S5PC11X_GPB(4), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC11X_GPB(5), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC11X_GPB(6), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC11X_GPB(7), S3C_GPIO_PULL_NONE);

	gpio_direction_output(S5PC11X_GPH0(5), 1);
}

static int tl2796_backlight_on(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPD0(3), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPD0(3), 1);
	gpio_free(S5PC11X_GPD0(3));

	return 0;
}

static int tl2796_reset_lcd(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PC11X_GPH0(6), "GPH0");
	if (err) {
		printk(KERN_ERR "failed to request GPH0 for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5PC11X_GPH0(6), 1);
	mdelay(100);

	gpio_set_value(S5PC11X_GPH0(6), 0);
	mdelay(10);

	gpio_set_value(S5PC11X_GPH0(6), 1);
	mdelay(10);

	gpio_free(S5PC11X_GPH0(6));

	return 0;
}

static struct s3c_platform_fb tl2796_data __initdata = {
	.hw_ver	= 0x60,
	.clk_name = "lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_HWORD,

	.cfg_gpio = tl2796_cfg_gpio,
	.backlight_on = tl2796_backlight_on,
	.reset_lcd = tl2796_reset_lcd,
};

#define LCD_BUS_NUM 	3
#define DISPLAY_CS	S5PC11X_GPB(5)
static struct spi_board_info spi_board_info[] __initdata = {
    	{
	    	.modalias	= "tl2796",
		.platform_data	= NULL,
		.max_speed_hz	= 1200000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};

#define DISPLAY_CLK	S5PC11X_GPB(4)
#define DISPLAY_SI	S5PC11X_GPB(7)
static struct spi_gpio_platform_data tl2796_spi_gpio_data = {
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
		.platform_data	= &tl2796_spi_gpio_data,
	},
};
#endif

struct map_desc smdkc110_iodesc[] = {};

static struct platform_device *smdkc110_devices[] __initdata = {
	&s3c_device_fb,
	&s3c_device_dm9000,
	&s3c_device_mfc,
#ifdef CONFIG_FB_S3C_TL2796
	&s3c_device_spi_gpio,
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
        &s3c_device_hsmmc0,
#endif        

#ifdef CONFIG_S3C_DEV_HSMMC1
        &s3c_device_hsmmc1,
#endif        
        
#ifdef CONFIG_S3C_DEV_HSMMC2        
        &s3c_device_hsmmc2,
#endif        
        
#ifdef CONFIG_S3C_DEV_HSMMC3
        &s3c_device_hsmmc3,        
#endif                
};

static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.delay 			= 10000,
	.presc 			= 49,
	.oversampling_shift	= 2,
	.resol_bit 		= 12,
	.s3c_adc_con		= ADC_TYPE_2,
};

static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
        /* s5pc100 supports 12-bit resolution */
        .delay  = 10000,
        .presc  = 49,
        .resolution = 12,
};

static struct i2c_board_info i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c08", 0x50), },
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	{ I2C_BOARD_INFO("24c128", 0x57), },
};

#if defined(CONFIG_TIMER_PWM)
static struct platform_pwm_backlight_data smdk_backlight_data = {
        .pwm_id         = 0,
        .max_brightness = 255,
        .dft_brightness = 255,
        .pwm_period_ns  = 78770,
};

static struct platform_device smdk_backlight_device = {
        .name           = "pwm-backlight",
        .dev            = {
                .parent = &s3c_device_timer[0].dev,
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
#define smdk_backlight_register()       do { } while (0)
#endif

static void __init smdkc110_map_io(void)
{
	s3c_device_nand.name = "s5pc100-nand";
	s5pc11x_init_io(smdkc110_iodesc, ARRAY_SIZE(smdkc110_iodesc));
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdkc110_uartcfgs, ARRAY_SIZE(smdkc110_uartcfgs));
	s5pc11x_reserve_bootmem();
}

static void __init smdkc110_dm9000_set(void)
{
	unsigned int tmp;

#if 0
	tmp = 0xfffffff0;
	__raw_writel(tmp, (S5PC11X_SROM_BW+0x18));
	tmp = __raw_readl(S5PC11X_SROM_BW);
	tmp &= ~(0xf<<20);
	tmp |= (0xd<<20);
	__raw_writel(tmp, S5PC11X_SROM_BW);


	tmp = __raw_readl(S5PC11X_MP01CON);
	tmp     &= ~(0xf<<20);
	tmp |=(2<<20);
	__raw_writel(tmp,(S5PC11X_MP01CON));
/* #else */
	tmp = 0xfffffff0;
	__raw_writel(tmp, (S5PC11X_SROM_BW+0x14));
	tmp = __raw_readl(S5PC11X_SROM_BW);
	tmp &= ~(0xf<<16);
	tmp |= (0x2<<16);
	__raw_writel(tmp, S5PC11X_SROM_BW);


	tmp = __raw_readl(S5PC11X_MP01CON);
	tmp     &= ~(0xf<<16);
	tmp |=(2<<16);
	__raw_writel(tmp,(S5PC11X_MP01CON));

#endif

}

static void __init smdkc110_machine_init(void)
{
	smdkc110_dm9000_set();

#ifdef CONFIG_FB_S3C_LTE480WV
	s3cfb_set_platdata(&lte480wv_data);
#endif

#ifdef CONFIG_FB_S3C_TL2796
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&tl2796_data);
#endif

	/* Setting up the HS-MMC clock using doutMpll */
	writel(((readl(S5P_CLK_SRC4) & ~(0xffff << 0)) | 0x6666), S5P_CLK_SRC4);
	writel(((readl(S5P_CLK_DIV4) & ~(0xffff << 0)) | 0x1111), S5P_CLK_DIV4);

	platform_add_devices(smdkc110_devices, ARRAY_SIZE(smdkc110_devices));

#if defined(CONFIG_PM)
//	s5pc11x_pm_init();
#endif
}

static void __init smdkc110_fixup(struct machine_desc *desc,
					struct tag *tags, char **cmdline,
					struct meminfo *mi)
{
#if defined(CONFIG_S5PC110_H_TYPE)
	mi->bank[0].start = 0x30000000;
	mi->bank[0].size = 128 * SZ_1M;
	mi->bank[0].node = 0;
#else
	mi->bank[0].start = 0x30000000;
	mi->bank[0].size = 64 * SZ_1M;
	mi->bank[0].node = 0;
#endif

	mi->bank[1].start = 0x40000000;
	mi->bank[1].size = 128 * SZ_1M;
	mi->bank[1].node = 1;

	mi->nr_banks = 2;

}

MACHINE_START(SMDKC110, "SMDKC110")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5PC11X_PA_SDRAM + 0x100,
	.fixup		= smdkc110_fixup,
	.init_irq	= s5pc110_init_irq,
	.map_io		= smdkc110_map_io,
	.init_machine	= smdkc110_machine_init,
	.timer		= &s5pc11x_timer,
MACHINE_END


#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void) {

}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void) {

}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_clk_en(void) {

}

EXPORT_SYMBOL(usb_host_clk_en);
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

	end = S5PC11X_GPH3(rows);

	/* Set all the necessary GPH2 pins to special-function 0 */
	for (gpio = S5PC11X_GPH3(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	end = S5PC11X_GPH2(columns);

	/* Set all the necessary GPK pins to special-function 0 */
	for (gpio = S5PC11X_GPH2(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}

EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);
#endif
