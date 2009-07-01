/*
 * mcs6000_ts.c - Touch screen driver for MELFAS MCS-6000 controller
 *
 * Copyright (C) 2009 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * Based on wm97xx-core.c
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c/mcs6000_ts.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/workqueue.h>

/* Registers */
#define MCS6000_TS_STATUS		0x00
#define STATUS_OFFSET			0
#define STATUS_NO			(0 << STATUS_OFFSET)
#define STATUS_INIT			(1 << STATUS_OFFSET)
#define STATUS_SENSING			(2 << STATUS_OFFSET)
#define STATUS_COORD			(3 << STATUS_OFFSET)
#define STATUS_GESTURE			(4 << STATUS_OFFSET)
#define ERROR_OFFSET			4
#define ERROR_NO			(0 << ERROR_OFFSET)
#define ERROR_POWER_ON_RESET		(1 << ERROR_OFFSET)
#define ERROR_INT_RESET			(2 << ERROR_OFFSET)
#define ERROR_EXT_RESET			(3 << ERROR_OFFSET)
#define ERROR_INVALID_REG_ADDRESS	(8 << ERROR_OFFSET)
#define ERROR_INVALID_REG_VALUE		(9 << ERROR_OFFSET)

#define MCS6000_TS_OP_MODE		0x01
#define RESET_OFFSET			0
#define RESET_NO			(0 << RESET_OFFSET)
#define RESET_EXT_SOFT			(1 << RESET_OFFSET)
#define OP_MODE_OFFSET			1
#define OP_MODE_SLEEP			(0 << OP_MODE_OFFSET)
#define OP_MODE_ACTIVE			(1 << OP_MODE_OFFSET)
#define GESTURE_OFFSET			4
#define GESTURE_DISABLE			(0 << GESTURE_OFFSET)
#define GESTURE_ENABLE			(1 << GESTURE_OFFSET)
#define PROXIMITY_OFFSET		5
#define PROXIMITY_DISABLE		(0 << PROXIMITY_OFFSET)
#define PROXIMITY_ENABLE		(1 << PROXIMITY_OFFSET)
#define SCAN_MODE_OFFSET		6
#define SCAN_MODE_INTERRUPT		(0 << SCAN_MODE_OFFSET)
#define SCAN_MODE_POLLING		(1 << SCAN_MODE_OFFSET)
#define REPORT_RATE_OFFSET		7
#define REPORT_RATE_40			(0 << REPORT_RATE_OFFSET)
#define REPORT_RATE_80			(1 << REPORT_RATE_OFFSET)

#define MCS6000_TS_SENS_CTL		0x02
#define MCS6000_TS_FILTER_CTL		0x03
#define PRI_FILTER_OFFSET		0
#define SEC_FILTER_OFFSET		4

#define MCS6000_TS_X_SIZE_UPPER		0x08
#define MCS6000_TS_X_SIZE_LOWER		0x09
#define MCS6000_TS_Y_SIZE_UPPER		0x0A
#define MCS6000_TS_Y_SIZE_LOWER		0x0B

#define MCS6000_TS_INPUT_INFO		0x10
#define INPUT_TYPE_OFFSET		0
#define INPUT_TYPE_NONTOUCH		(0 << INPUT_TYPE_OFFSET)
#define INPUT_TYPE_SINGLE		(1 << INPUT_TYPE_OFFSET)
#define INPUT_TYPE_DUAL			(2 << INPUT_TYPE_OFFSET)
#define INPUT_TYPE_PALM			(3 << INPUT_TYPE_OFFSET)
#define INPUT_TYPE_PROXIMITY		(7 << INPUT_TYPE_OFFSET)
#define GESTURE_CODE_OFFSET		3
#define GESTURE_CODE_NO			(0 << GESTURE_CODE_OFFSET)

#define MCS6000_TS_X_POS_UPPER		0x11
#define MCS6000_TS_X_POS_LOWER		0x12
#define MCS6000_TS_Y_POS_UPPER		0x13
#define MCS6000_TS_Y_POS_LOWER		0x14
#define MCS6000_TS_Z_POS		0x15
#define MCS6000_TS_WIDTH		0x16
#define MCS6000_TS_GESTURE_VAL		0x17
#define MCS6000_TS_MODULE_REV		0x20
#define MCS6000_TS_FIRMWARE_VER		0x21

/* Touchscreen absolute values */
#define MCS6000_MAX_XC			0x3ff
#define MCS6000_MAX_YC			0x3ff

/* definition about Pressure */
#define DEFAULT_PRESSURE               0x100

static int abs_p[3] = {0, 256, 0};
module_param_array(abs_p, int, NULL, 0);
MODULE_PARM_DESC(abs_p, "Touchscreen absolute Pressure min, max, fuzz");

/* definition about FW related  */
#define FW_NOT_YET
//#undef FW_NOT_YET

#if defined(FW_NOT_YET)
#define MCS6000_TS_BASE			0x0
#else
#define MCS6000_TS_BASE			MCS6000_TS_INPUT_INFO
#endif

enum mcs6000_ts_read_offset {
	READ_INPUT_INFO,
	READ_X_POS_UPPER,
	READ_X_POS_LOWER,
	READ_Y_POS_UPPER,
	READ_Y_POS_LOWER,
#if defined(FW_NOT_YET)
	READ_FIRMWARE_VERSION,
	READ_HARDWARE_VERSION,
#endif
	READ_BLOCK_SIZE,
};

/* Each client has this additional data */
struct mcs6000_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct ts_event_work;
	struct mcs6000_ts_platform_data *platform_data;

	unsigned int irq;
	atomic_t irq_disable;
};

static struct i2c_driver mcs6000_ts_driver;

static void mcs6000_ts_input_read(struct mcs6000_ts_data *data)
{
	struct i2c_client *client = data->client;
	u8 buffer[READ_BLOCK_SIZE];
	int err;
	int x;
	int y;

	err = i2c_smbus_read_i2c_block_data(client, MCS6000_TS_BASE,
			READ_BLOCK_SIZE, buffer);
	if (err < 0) {
		dev_err(&client->dev, "%s, err[%d]\n", __func__, err);
		return;
	}

	switch (buffer[READ_INPUT_INFO]) {
	case INPUT_TYPE_NONTOUCH:
		input_report_abs(data->input_dev, ABS_PRESSURE, 0);
		input_report_key(data->input_dev, BTN_TOUCH, 0);
		input_sync(data->input_dev);
		break;
	case INPUT_TYPE_SINGLE:
		x = (buffer[READ_X_POS_UPPER] << 8) | buffer[READ_X_POS_LOWER];
		y = (buffer[READ_Y_POS_UPPER] << 8) | buffer[READ_Y_POS_LOWER];

		input_report_key(data->input_dev, BTN_TOUCH, 1);
		input_report_abs(data->input_dev, ABS_X, x);
		input_report_abs(data->input_dev, ABS_Y, y);
		input_report_abs(data->input_dev, ABS_PRESSURE,
			DEFAULT_PRESSURE);
		input_sync(data->input_dev);
		break;
	case INPUT_TYPE_DUAL:
		x = (buffer[READ_X_POS_UPPER] << 8) | buffer[READ_X_POS_LOWER];
		y = (buffer[READ_Y_POS_UPPER] << 8) | buffer[READ_Y_POS_LOWER];

		input_report_key(data->input_dev, BTN_TOUCH, 1);
		input_report_abs(data->input_dev, ABS_X, x);
		input_report_abs(data->input_dev, ABS_Y, y);
		input_report_abs(data->input_dev, ABS_PRESSURE,
			DEFAULT_PRESSURE);
		input_sync(data->input_dev);
		/* TODO */
		break;
	case INPUT_TYPE_PALM:
		/* TODO */
		break;
	case INPUT_TYPE_PROXIMITY:
		/* TODO */
		break;
	default:
		dev_err(&client->dev, "Unknown ts input type %d\n",
				buffer[READ_INPUT_INFO]);
		break;
	}
}

static void mcs6000_ts_irq_worker(struct work_struct *work)
{
	struct mcs6000_ts_data *data = container_of(work,
			struct mcs6000_ts_data, ts_event_work);

	mcs6000_ts_input_read(data);

	atomic_dec(&data->irq_disable);
	enable_irq(data->irq);
}

static irqreturn_t mcs6000_ts_interrupt(int irq, void *dev_id)
{
	struct mcs6000_ts_data *data = dev_id;

	if (!work_pending(&data->ts_event_work)) {
		disable_irq_nosync(data->irq);
		atomic_inc(&data->irq_disable);
		schedule_work(&data->ts_event_work);
	}

	return IRQ_HANDLED;
}

static int mcs6000_ts_input_init(struct mcs6000_ts_data *data)
{
	struct input_dev *input_dev;
	int ret = 0;

	INIT_WORK(&data->ts_event_work, mcs6000_ts_irq_worker);

	data->input_dev = input_allocate_device();
	if (data->input_dev == NULL) {
		ret = -ENOMEM;
		goto err_input;
	}

	input_dev = data->input_dev;
	input_dev->name = "MELPAS MCS-6000 Touchscreen";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &data->client->dev;
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	input_set_abs_params(input_dev, ABS_X, 0, MCS6000_MAX_XC, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, MCS6000_MAX_YC, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, abs_p[0], abs_p[1],
		abs_p[2], 0);

	ret = input_register_device(data->input_dev);
	if (ret < 0)
		goto err_register;

	ret = request_irq(data->irq, mcs6000_ts_interrupt, IRQF_TRIGGER_LOW,
			"mcs6000_ts_input", data);
	if (ret < 0) {
		dev_err(&data->client->dev, "Failed to register interrupt\n");
		goto err_irq;
	}

	input_set_drvdata(input_dev, data);

	return 0;
err_irq:
	input_unregister_device(data->input_dev);
	data->input_dev = NULL;
err_register:
	input_free_device(data->input_dev);
err_input:
	return ret;
}

static void mcs6000_ts_phys_init(struct mcs6000_ts_data *data)
{
	struct i2c_client *client = data->client;
	struct mcs6000_ts_platform_data *platform_data = data->platform_data;

	/* Touch reset & sleep mode */
	i2c_smbus_write_byte_data(client, MCS6000_TS_OP_MODE,
			RESET_EXT_SOFT | OP_MODE_SLEEP);

	/* Touch size */
	i2c_smbus_write_byte_data(client, MCS6000_TS_X_SIZE_UPPER,
			platform_data->x_size >> 8);
	i2c_smbus_write_byte_data(client, MCS6000_TS_X_SIZE_LOWER,
			platform_data->x_size & 0xff);
	i2c_smbus_write_byte_data(client, MCS6000_TS_Y_SIZE_UPPER,
			platform_data->y_size >> 8);
	i2c_smbus_write_byte_data(client, MCS6000_TS_Y_SIZE_LOWER,
			platform_data->y_size & 0xff);

	/* Touch active mode & 80 report rate */
	i2c_smbus_write_byte_data(data->client, MCS6000_TS_OP_MODE,
			OP_MODE_ACTIVE | REPORT_RATE_80);
}

static int __devinit mcs6000_ts_probe(struct i2c_client *client,
		const struct i2c_device_id *idp)
{
	struct mcs6000_ts_data *data;
	int ret;

	data = kzalloc(sizeof(struct mcs6000_ts_data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev, "Failed to allocate driver data\n");
		ret = -ENOMEM;
		goto exit;
	}

	data->client = client;
	data->platform_data = client->dev.platform_data;
	data->irq = client->irq;
	atomic_set(&data->irq_disable, 0);

	i2c_set_clientdata(client, data);

	if (data->platform_data->set_pin)
		data->platform_data->set_pin();

	ret = mcs6000_ts_input_init(data);
	if (ret)
		goto exit_free;

	mcs6000_ts_phys_init(data);

	return 0;

exit_free:
	kfree(data);
	i2c_set_clientdata(client, NULL);
exit:
	return ret;
}

static int __devexit mcs6000_ts_remove(struct i2c_client *client)
{
	struct mcs6000_ts_data *data = i2c_get_clientdata(client);

	free_irq(data->irq, data);
	cancel_work_sync(&data->ts_event_work);

	/*
	 * If work indeed has been cancelled, disable_irq() will have been left
	 * unbalanced from mcs6000_ts_interrupt().
	 */
	while (atomic_dec_return(&data->irq_disable) >= 0)
		enable_irq(data->irq);

	input_unregister_device(data->input_dev);
	kfree(data);

	i2c_set_clientdata(client, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int mcs6000_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	/* Touch sleep mode */
	i2c_smbus_write_byte_data(client, MCS6000_TS_OP_MODE, OP_MODE_SLEEP);

	return 0;
}

static int mcs6000_ts_resume(struct i2c_client *client)
{
	struct mcs6000_ts_data *data;

	data = i2c_get_clientdata(client);
	mcs6000_ts_phys_init(data);

	return 0;
}
#endif

static const struct i2c_device_id mcs6000_ts_id[] = {
	{ "mcs6000_ts", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mcs6000_ts_id);

static struct i2c_driver mcs6000_ts_driver = {
	.driver = {
		.name = "mcs6000_ts",
	},
	.probe		= mcs6000_ts_probe,
	.remove		= __devexit_p(mcs6000_ts_remove),
#ifdef CONFIG_PM
	.suspend	= mcs6000_ts_suspend,
	.resume		= mcs6000_ts_resume,
#endif
	.id_table	= mcs6000_ts_id,
};

static int __init mcs6000_ts_init(void)
{
	return i2c_add_driver(&mcs6000_ts_driver);
}

static void __exit mcs6000_ts_exit(void)
{
	i2c_del_driver(&mcs6000_ts_driver);
}

module_init(mcs6000_ts_init);
module_exit(mcs6000_ts_exit);

/* Module information */
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_DESCRIPTION("Touch screen driver for MELFAS MCS-6000 controller");
MODULE_LICENSE("GPL");
