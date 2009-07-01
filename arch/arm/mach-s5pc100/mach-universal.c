/*
 * linux/arch/arm/mach-s5pc100/mach-universal.c
 *
 * Copyright (C) 2009 Samsung Electronics Co.Ltd
 * Author: InKi Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/fb.h>
#include <plat/gpio-cfg.h>
#include <plat/iic.h>
#include <plat/regs-serial.h>
#include <plat/s5pc100.h>

#include <mach/gpio.h>
#include <mach/map.h>

extern struct sys_timer s5pc1xx_timer;
extern void s5pc1xx_reserve_bootmem(void);

static struct s3c24xx_uart_clksrc universal_serial_clocks[] = {
#if defined(CONFIG_SERIAL_S5PC1XX_HSUART)
/* HS-UART Clock using SCLK */
        [0] = {
                .name           = "uclk1",
                .divisor        = 1,
                .min_baud       = 0,
                .max_baud       = 0,
        },
#else
        [0] = {
                .name           = "pclk",
                .divisor        = 1,
                .min_baud       = 0,
                .max_baud       = 0,
        },
#endif
};

static struct s3c2410_uartcfg universal_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = universal_serial_clocks,
                .clocks_size = ARRAY_SIZE(universal_serial_clocks),
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = universal_serial_clocks,
                .clocks_size = ARRAY_SIZE(universal_serial_clocks),
	},
        [2] = {
                .hwport      = 2,
                .flags       = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = universal_serial_clocks,
                .clocks_size = ARRAY_SIZE(universal_serial_clocks),
        },
        [3] = {
                .hwport      = 3,
                .flags       = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
                .clocks      = universal_serial_clocks,
                .clocks_size = ARRAY_SIZE(universal_serial_clocks),
        },
};

/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
	/* TODO */
};

/* I2C1 */
static struct i2c_board_info i2c_devs1[] __initdata = {
	/* TODO */
};

/* GPIO I2C 2.6V */
#define I2C_GPIO_26V_BUS	2
static struct i2c_gpio_platform_data universal_i2c_gpio_26v_data = {
	.sda_pin	= S5PC1XX_GPJ3(6),
	.scl_pin	= S5PC1XX_GPJ3(7),
};

static struct platform_device universal_i2c_gpio_26v = {
	.name		= "i2c-gpio",
	.id		= I2C_GPIO_26V_BUS,
	.dev		= {
		.platform_data	= &universal_i2c_gpio_26v_data,
	},
};

static struct i2c_board_info i2c_gpio_26v_devs[] __initdata = {
	/* TODO */
};

/* GPIO I2C 2.6V - HDMI */
#define I2C_GPIO_HDMI_BUS	3
static struct i2c_gpio_platform_data universal_i2c_gpio_hdmi_data = {
	.sda_pin	= S5PC1XX_GPJ4(0),
	.scl_pin	= S5PC1XX_GPJ4(3),
};

static struct platform_device universal_i2c_gpio_hdmi = {
	.name		= "i2c-gpio",
	.id		= I2C_GPIO_HDMI_BUS,
	.dev		= {
		.platform_data	= &universal_i2c_gpio_hdmi_data,
	},
};

static struct i2c_board_info i2c_gpio_hdmi_devs[] __initdata = {
	/* TODO */
};

/* GPIO I2C 2.8V */
#define I2C_GPIO_28V_BUS	4
static struct i2c_gpio_platform_data universal_i2c_gpio_28v_data = {
	.sda_pin	= S5PC1XX_GPH2(4),
	.scl_pin	= S5PC1XX_GPH2(5),
};

static struct platform_device universal_i2c_gpio_28v = {
	.name		= "i2c-gpio",
	.id		= I2C_GPIO_28V_BUS,
	.dev		= {
		.platform_data	= &universal_i2c_gpio_28v_data,
	},
};

static struct i2c_board_info i2c_gpio_28v_devs[] __initdata = {
	/* TODO */
};

#define LCD_BUS_NUM 	3
#define DISPLAY_CS	S5PC1XX_GPK3(5)
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

#define DISPLAY_CLK	S5PC1XX_GPK3(6)
#define DISPLAY_SI	S5PC1XX_GPK3(7)
static struct spi_gpio_platform_data tl2796_spi_gpio_data = {
	.sck	= DISPLAY_CLK,
	.mosi	= DISPLAY_SI,
	.miso	= -1,

	.num_chipselect	= 1,
};

static struct platform_device universal_spi_gpio = {
	.name	= "spi_gpio",
	.id	= LCD_BUS_NUM,
	.dev	= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &tl2796_spi_gpio_data,
	},
};

static void tl2796_gpio_setup(void)
{
	gpio_request(S5PC1XX_GPH1(7), "MLCD_RST");
	gpio_request(S5PC1XX_GPJ1(3), "MLCD_ON");
}

/* Configure GPIO pins for RGB Interface */
static void rgb_cfg_gpio(struct platform_device *dev)
{
	int i;

	for (i = 0; i < 8; i++)
		s3c_gpio_cfgpin(S5PC1XX_GPF0(i), S3C_GPIO_SFN(2));

	for (i = 0; i < 8; i++)
		s3c_gpio_cfgpin(S5PC1XX_GPF1(i), S3C_GPIO_SFN(2));

	for (i = 0; i < 8; i++)
		s3c_gpio_cfgpin(S5PC1XX_GPF2(i), S3C_GPIO_SFN(2));

	for (i = 0; i < 4; i++)
		s3c_gpio_cfgpin(S5PC1XX_GPF3(i), S3C_GPIO_SFN(2));
}

static int tl2796_power_on(struct platform_device *dev)
{
	/* set gpio data for MLCD_RST to HIGH */
	gpio_direction_output(S5PC1XX_GPH1(7), 1);
	/* set gpio data for MLCD_ON to HIGH */
	gpio_direction_output(S5PC1XX_GPJ1(3), 1);
	mdelay(25);

	/* set gpio data for MLCD_RST to LOW */
	gpio_direction_output(S5PC1XX_GPH1(7), 0);
	udelay(20);
	/* set gpio data for MLCD_RST to HIGH */
	gpio_direction_output(S5PC1XX_GPH1(7), 1);
	mdelay(20);

	return 0;
}

static int tl2796_reset(struct platform_device *dev)
{
	/* set gpio pin for MLCD_RST to LOW */
	gpio_direction_output(S5PC1XX_GPH1(7), 0);
	udelay(1);	/* Shorter than 5 usec */
	/* set gpio pin for MLCD_RST to HIGH */
	gpio_direction_output(S5PC1XX_GPH1(7), 1);
	mdelay(10);

	return 0;
}

static struct s3c_platform_fb fb_data __initdata = {
	.hw_ver = 0x50,
	.clk_name = "lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_WORD | FB_SWAP_HWORD,

	.cfg_gpio =  rgb_cfg_gpio,
	.backlight_on = tl2796_power_on,
	.reset_lcd = tl2796_reset,
};

struct map_desc universal_iodesc[] = {};

static struct platform_device *universal_devices[] __initdata = {
	&s3c_device_fb,
	&s3c_device_mfc,
	&universal_spi_gpio,
	&s3c_device_i2c0,
	&s3c_device_i2c1,
	&universal_i2c_gpio_26v,
	&universal_i2c_gpio_hdmi,
	&universal_i2c_gpio_28v,
};

static void __init universal_i2c_gpio_init(void)
{
	s3c_gpio_setpull(S5PC1XX_GPH2(4), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC1XX_GPH2(5), S3C_GPIO_PULL_NONE);

	s3c_gpio_setpull(S5PC1XX_GPJ3(6), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC1XX_GPJ3(7), S3C_GPIO_PULL_NONE);

	s3c_gpio_setpull(S5PC1XX_GPJ4(0), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC1XX_GPJ4(3), S3C_GPIO_PULL_NONE);
}

static void __init universal_map_io(void)
{
	s5pc1xx_init_io(universal_iodesc, ARRAY_SIZE(universal_iodesc));
	s3c24xx_init_clocks(0);
#if defined(CONFIG_SERIAL_S5PC1XX_HSUART)
        writel((readl(S5P_CLK_DIV2) & ~(0xf << 0)), S5P_CLK_DIV2);
#endif
	s3c24xx_init_uarts(universal_uartcfgs, ARRAY_SIZE(universal_uartcfgs));
	s5pc1xx_reserve_bootmem();
}

static void __init universal_machine_init(void)
{
	tl2796_gpio_setup();

	universal_i2c_gpio_init();

	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	i2c_register_board_info(I2C_GPIO_26V_BUS, i2c_gpio_26v_devs,
				ARRAY_SIZE(i2c_gpio_26v_devs));
	i2c_register_board_info(I2C_GPIO_HDMI_BUS, i2c_gpio_hdmi_devs,
				ARRAY_SIZE(i2c_gpio_hdmi_devs));
	i2c_register_board_info(I2C_GPIO_28V_BUS, i2c_gpio_28v_devs,
				ARRAY_SIZE(i2c_gpio_28v_devs));

	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&fb_data);
	platform_add_devices(universal_devices, ARRAY_SIZE(universal_devices));
}

MACHINE_START(UNIVERSAL, "UNIVERSAL")
	/* Maintainer: InKi Dae <inki.dae@samsung.com> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5PC1XX_PA_SDRAM + 0x100,

	.init_irq	= s5pc100_init_irq,
	.map_io		= universal_map_io,
	.init_machine	= universal_machine_init,
	.timer		= &s5pc1xx_timer,
MACHINE_END
