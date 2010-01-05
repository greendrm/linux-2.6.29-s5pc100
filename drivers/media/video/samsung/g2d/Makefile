#################################################
# Makefile for FIMG-2D
# 2009 (C) Samsung Electronics
# Author : Jinhee Hyeon <jh0722.hyen@samsung.com>
#################################################

obj-$(CONFIG_VIDEO_G2D) += fimg2d3x.o

EXTRA_CFLAGS += -Idrivers/media/video

ifeq ($(CONFIG_VIDEO_G2D_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

