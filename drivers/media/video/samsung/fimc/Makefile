obj-$(CONFIG_VIDEO_FIMC)	+= fimc_core.o fimc_v4l2.o fimc_cfg.o
obj-$(CONFIG_VIDEO_FIMC_MIPI)	+= csis.o
obj-$(CONFIG_ARCH_S3C64XX)	+= fimc3x_regs.o
obj-$(CONFIG_ARCH_S5PC1XX)	+= fimc4x_regs.o

EXTRA_CFLAGS += -Idrivers/media/video

ifeq ($(CONFIG_VIDEO_FIMC_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif
