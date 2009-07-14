obj-$(CONFIG_VIDEO_FIMC2)	+= fimc_core.o fimc_v4l2.o fimc_cfg.o fimc_capture.o fimc_output.o fimc_overlay.o
obj-$(CONFIG_VIDEO_FIMC_MIPI)	+= csis.o
obj-$(CONFIG_ARCH_S5PC1XX)	+= fimc4x_regs.o

EXTRA_CFLAGS += -Idrivers/media/video

ifeq ($(CONFIG_VIDEO_FIMC_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

