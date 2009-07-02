#ifndef __LINUX_MCS6000_TS_H
#define __LINUX_MCS6000_TS_H

/* platform data for the MELFAS MCS6000 touchscreen driver */
struct mcs6000_ts_platform_data {
	void (*set_pin)(void);
	int x_size;
	int y_size;
};

#endif /* __LINUX_MCS6000_TS_H */
