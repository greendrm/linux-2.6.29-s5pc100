/*
 * Driver for S5K4BA (UXGA camera) from Samsung Electronics
 * 
 * 1/4" 2.0Mp CMOS Image Sensor SoC with an Embedded Image Processor
 *
 * Copyright (C) 2009, Jinsung Yang <jsgood.yang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-i2c-drv.h>
#include <media/s5k4ba_platform.h>

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif

#include "s5k4ba.h"

#define S5K4BA_DRIVER_NAME	"S5K4BA"

/* Default resolution & pixelformat. plz ref s5k4ba_platform.h */
#define DEFAULT_RES		WVGA	/* Index of resoultion */
#define DEFAUT_FPS_INDEX	S5K4BA_15FPS
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */

/*
 * Specification
 * Parallel : ITU-R. 656/601 YUV422, RGB565, RGB888 (Up to VGA), RAW10 
 * Serial : MIPI CSI2 (single lane) YUV422, RGB565, RGB888 (Up to VGA), RAW10
 * Resolution : 1280 (H) x 1024 (V)
 * Image control : Brightness, Contrast, Saturation, Sharpness, Glamour
 * Effect : Mono, Negative, Sepia, Aqua, Sketch
 * FPS : 15fps @full resolution, 30fps @VGA, 24fps @720p
 * Max. pixel clock frequency : 48MHz(upto)
 * Internal PLL (6MHz to 27MHz input frequency)
 */

/* Camera functional setting values configured by user concept */
struct s5k4ba_userset {
	signed int exposure_bias;	/* V4L2_CID_EXPOSURE */
	unsigned int ae_lock;
	unsigned int awb_lock;
	unsigned int auto_wb;	/* V4L2_CID_AUTO_WHITE_BALANCE */
	unsigned int manual_wb;	/* V4L2_CID_WHITE_BALANCE_PRESET */
	unsigned int wb_temp;	/* V4L2_CID_WHITE_BALANCE_TEMPERATURE */
	unsigned int effect;	/* Color FX (AKA Color tone) */
	unsigned int contrast;	/* V4L2_CID_CONTRAST */
	unsigned int saturation;	/* V4L2_CID_SATURATION */
	unsigned int sharpness;		/* V4L2_CID_SHARPNESS */
	unsigned int glamour;
};

struct s5k4ba_state {
	struct s5k4ba_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct s5k4ba_userset userset;
	int freq;	/* MCLK in KHz */
	int isize;
	int ver;
	int fps;
};

static inline struct s5k4ba_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct s5k4ba_state, sd);
}

/*
 * S5K4BA register structure : 2bytes address, 2bytes value
 * retry on write failure up-to 5 times
 */
static inline int s5k4ba_write(struct v4l2_subdev *sd, u8 addr, u8 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[1];
	unsigned char reg[2];
	int err = 0;
	int retry = 0;


	if (!client->adapter)
		return -ENODEV;

again:
	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = reg;

	reg[0] = addr & 0xff;
	reg[1] = val & 0xff;

	err = i2c_transfer(client->adapter, msg, 1);
	if (err >= 0)
		return err;	/* Returns here on success */

	/* abnormal case: retry 5 times */
	if (retry < 5) {
		dev_err(&client->dev, "%s:address:0x%02x%02x, value:0x%02x%02x\n",
				__func__, reg[0], reg[1], reg[2], reg[3]);
		retry++;
		goto again;
	}

	return err;
}

/*
 * Register configuration sets are served
 * Special purpose register address:
 * 	REG_DELAY: delay for specified ms
 * 	REG_CMD: address followed by special purpose commands
 * 		VAL_END: end of register set
 */
static int s5k4ba_write_regs(struct v4l2_subdev *sd,
					const unsigned char *reglist)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k4ba_regset_type *regset = (struct s5k4ba_regset_type *)reglist;
	struct s5k4ba_reg *reg = (struct s5k4ba_reg *)regset->regset;
	int i, err = 0;

	for (i = 0; i< regset->len; i++) {
		/* TODO: need burst mode in case of 0x0F12 is comming with address */
		if (reg->addr == REG_DELAY) {
			/* delay command (in ms) */
			mdelay(reg->val);
			dev_dbg(&client->dev, "%s:delay for %dms\n", __func__, reg->val);
		} else {
			/* or programme register */
			err = s5k4ba_write(sd, reg->addr, reg->val);
			dev_dbg(&client->dev, "%s:0x%04x,0x%04x programmed\n", __func__,
					reg->addr, reg->val);
		}

		if (err < 0) {
			dev_dbg(&client->dev, "%s:failed on 0x%04x,0x%04x\n", __func__,
					reg->addr, reg->val);
			return err;
		}
	}

	return 0;
}

static const char *s5k4ba_querymenu_wb_preset[] = {
	"WB Tungsten", "WB Fluorescent", "WB sunny", "WB cloudy", NULL
};

static const char *s5k4ba_querymenu_effect_mode[] = {
	"Effect Sepia", "Effect Aqua", "Effect Monochrome",
	"Effect Negative", "Effect Sketch", NULL
};

static const char *s5k4ba_querymenu_ev_bias_mode[] = {
	"-3EV",	"-2,1/2EV", "-2EV", "-1,1/2EV",
	"-1EV", "-1/2EV", "0", "1/2EV",
	"1EV", "1,1/2EV", "2EV", "2,1/2EV",
	"3EV", NULL
};

static struct v4l2_queryctrl s5k4ba_controls[] = {
	{
		/*
		 * For now, we just support in preset type
		 * to be close to generic WB system,
		 * we define color temp range for each preset
		 */
		.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "White balance in kelvin",
		.minimum = 0,
		.maximum = 10000,
		.step = 1,
		.default_value = 0,	/* FIXME */
	},
	{
		.id = V4L2_CID_WHITE_BALANCE_PRESET,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "White balance preset",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5k4ba_querymenu_wb_preset) - 2,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_AUTO_WHITE_BALANCE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Auto white balance",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_EXPOSURE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Exposure bias",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5k4ba_querymenu_ev_bias_mode) - 2,
		.step = 1,
		.default_value = (ARRAY_SIZE(s5k4ba_querymenu_ev_bias_mode) - 2) / 2,	/* 0 EV */
	},
	{
		.id = V4L2_CID_COLORFX,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5k4ba_querymenu_effect_mode) - 2,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_CONTRAST,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Contrast",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
	{
		.id = V4L2_CID_SATURATION,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Saturation",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
	{
		.id = V4L2_CID_SHARPNESS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Sharpness",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
};

const char **s5k4ba_ctrl_get_menu(u32 id)
{
	switch (id) {
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return s5k4ba_querymenu_wb_preset;

	case V4L2_CID_COLORFX:
		return s5k4ba_querymenu_effect_mode;

	case V4L2_CID_EXPOSURE:
		return s5k4ba_querymenu_ev_bias_mode;

	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *s5k4ba_find_qctrl(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(s5k4ba_controls); i++)
		if (s5k4ba_controls[i].id == id)
			return &s5k4ba_controls[i];

	return NULL;
}

static int s5k4ba_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(s5k4ba_controls); i++) {
		if (s5k4ba_controls[i].id == qc->id) {
			memcpy(qc, &s5k4ba_controls[i], sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int s5k4ba_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;

	qctrl.id = qm->id;
	s5k4ba_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm, &qctrl, s5k4ba_ctrl_get_menu(qm->id));
}

/*
 * Clock configuration
 * Configure expected MCLK from host and return EINVAL if not supported clock
 * frequency is expected
 * 	freq : in Hz
 * 	flag : not supported for now
 */
static int s5k4ba_s_crystal_freq(struct v4l2_subdev *sd, u32 freq, u32 flags)
{
	int err = -EINVAL;

	return err;
}

static int s5k4ba_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = -EINVAL;

	return err;
}

static int s5k4ba_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = -EINVAL;

	return err;
}
static int s5k4ba_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
	int err = -EINVAL;

	return err;
}

static int s5k4ba_enum_frameintervals(struct v4l2_subdev *sd, struct v4l2_frmivalenum *fival)
{
	int err = -EINVAL;

	return err;
}

static int s5k4ba_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmtdesc)
{
	int err = -EINVAL;

	return err;
}

static int s5k4ba_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = -EINVAL;

	return err;
}

static int s5k4ba_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	int err = -EINVAL;

	return err;
}

static int s5k4ba_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	int err = -EINVAL;

	return err;
}

static int s5k4ba_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k4ba_state *state = to_state(sd);
	struct s5k4ba_userset userset = state->userset;
	int err = -EINVAL;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		ctrl->value = userset.exposure_bias;
		err = 0;
		break;
	case V4L2_CID_AUTO_WHITE_BALANCE:
		ctrl->value = userset.auto_wb;
		err = 0;
		break;
	case V4L2_CID_WHITE_BALANCE_PRESET:
		ctrl->value = userset.manual_wb;
		err = 0;
		break;
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		ctrl->value = userset.wb_temp;
		err = 0;
		break;
	case V4L2_CID_COLORFX:
		ctrl->value = userset.effect;
		err = 0;
		break;
	case V4L2_CID_CONTRAST:
		ctrl->value = userset.contrast;
		err = 0;
		break;
	case V4L2_CID_SATURATION:
		ctrl->value = userset.saturation;
		err = 0;
		break;
	case V4L2_CID_SHARPNESS:
		ctrl->value = userset.saturation;
		err = 0;
		break;
	default:
		dev_err(&client->dev, "%s: no such ctrl\n", __func__);
		break;
	}
	
	return err;
}

static int s5k4ba_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
#ifdef S5K4BA_COMPLETE
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k4ba_state *state = to_state(sd);
	struct s5k4ba_userset userset = state->userset;
	int err = -EINVAL;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		dev_dbg(&client->dev, "%s:V4L2_CID_EXPOSURE\n", __func__);
		err = s5k4ba_write_regs(sd, s5k4ba_regs_ev_bias[ctrl->value]);
		break;
	case V4L2_CID_AUTO_WHITE_BALANCE:
		dev_dbg(&client->dev, "%s:V4L2_CID_AUTO_WHITE_BALANCE\n", __func__);
		err = s5k4ba_write_regs(sd, s5k4ba_regs_awb_enable[ctrl->value]);
		break;
	case V4L2_CID_WHITE_BALANCE_PRESET:
		dev_dbg(&client->dev, "%s:V4L2_CID_WHITE_BALANCE_PRESET\n", __func__);
		err = s5k4ba_write_regs(sd, s5k4ba_regs_wb_preset[ctrl->value]);
		break;
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		dev_dbg(&client->dev, "%s:V4L2_CID_WHITE_BALANCE_TEMPERATURE\n", __func__);
		err = s5k4ba_write_regs(sd, s5k4ba_regs_wb_temperature[ctrl->value]);
		break;
	case V4L2_CID_COLORFX:
		dev_dbg(&client->dev, "%s:V4L2_CID_COLORFX\n", __func__);
		err = s5k4ba_write_regs(sd, s5k4ba_regs_color_effect[ctrl->value]);
		break;
	case V4L2_CID_CONTRAST:
		dev_dbg(&client->dev, "%s:V4L2_CID_CONTRAST\n", __func__);
		err = s5k4ba_write_regs(sd, s5k4ba_regs_contrast_bias[ctrl->value]);
		break;
	case V4L2_CID_SATURATION:
		dev_dbg(&client->dev, "%s:V4L2_CID_SATURATION\n", __func__);
		err = s5k4ba_write_regs(sd, s5k4ba_regs_saturation_bias[ctrl->value]);
		break;
	case V4L2_CID_SHARPNESS:
		dev_dbg(&client->dev, "%s:V4L2_CID_SHARPNESS\n", __func__);
		err = s5k4ba_write_regs(sd, s5k4ba_regs_sharpness_bias[ctrl->value]);
		break;
	default:
		dev_err(&client->dev, "%s: no such control\n", __func__);
		break;
	}

	if (err < 0)
		goto out;
	else
		return 0;

out:
	dev_dbg(&client->dev, "%s:vidioc_s_ctrl failed\n", __func__);
	return err;
#else
	return 0;
#endif
}

static int s5k4ba_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k4ba_state *state = to_state(sd);
	struct s5k4ba_userset userset = state->userset;
	int err = -EINVAL;

	v4l_info(client, "%s: camera initialization start\n", __FUNCTION__);
#if 0
	err = s5k4ba_write_regs(sd, s5k4ba_init);
	if (err < 0)
		return -EIO;	/* FIXME */
#endif
	return 0;
}

static const struct v4l2_subdev_core_ops s5k4ba_core_ops = {
	.s_config = s5k4ba_s_config,	/* initializing API */
	.queryctrl = s5k4ba_queryctrl,
	.querymenu = s5k4ba_querymenu,
	.g_ctrl = s5k4ba_g_ctrl,
	.s_ctrl = s5k4ba_s_ctrl,
};

static const struct v4l2_subdev_video_ops s5k4ba_video_ops = {
	.s_crystal_freq = s5k4ba_s_crystal_freq,
	.g_fmt = s5k4ba_g_fmt,
	.s_fmt = s5k4ba_s_fmt,
	.s_crystal_freq = s5k4ba_s_crystal_freq,
	.enum_framesizes = s5k4ba_enum_framesizes,
	.enum_frameintervals = s5k4ba_enum_frameintervals,
	.enum_fmt = s5k4ba_enum_fmt,
	.try_fmt = s5k4ba_try_fmt,
	.g_parm = s5k4ba_g_parm,
	.s_parm = s5k4ba_s_parm,
};

static const struct v4l2_subdev_ops s5k4ba_ops = {
	.core = &s5k4ba_core_ops,
	.video = &s5k4ba_video_ops,
};

static int s5k4ba_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct s5k4ba_state *state;
	struct v4l2_subdev *sd;
	struct s5k4ba_platform_data *pdata;

	state = kzalloc(sizeof(struct s5k4ba_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	printk(KERN_INFO "##################S5K4BA#############\n");
	pdata = client->dev.platform_data;
#if 1
	if (!pdata) {
		dev_err(&client->dev, "No platform data?\n");
		return -ENODEV;
	}
#endif
	sd = &state->sd;
	strcpy(sd->name, S5K4BA_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &s5k4ba_ops);
#if 1
	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	if (!(pdata->default_width && pdata->default_height)) {
		/* TODO: assign driver default resolution */
	} else {
		state->pix.width = pdata->default_width;
		state->pix.height = pdata->default_height;
	}

	if (!pdata->pixelformat)
		state->pix.pixelformat = DEFAULT_FMT;
	else
		state->pix.pixelformat = pdata->pixelformat;
#endif	
	return 0;
}


static int s5k4ba_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id s5k4ba_id[] = {
	{ S5K4BA_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, s5k4ba_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = S5K4BA_DRIVER_NAME,
	.probe = s5k4ba_probe,
	.remove = s5k4ba_remove,
	.id_table = s5k4ba_id,
};

MODULE_DESCRIPTION("Samsung Electronics S5K4BA UXGA camera driver");
MODULE_AUTHOR("Jinsung Yang <jsgood.yang@samsung.com>");
MODULE_LICENSE("GPL");

