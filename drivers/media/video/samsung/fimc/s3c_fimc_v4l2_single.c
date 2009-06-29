/* linux/drivers/media/video/samsung/s3c_fimc_v4l2.c
 *
 * V4L2 interface support file for Samsung Camera Interface (FIMC) driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/videodev2.h>
#include <media/v4l2-ioctl.h>

#include "s3c_fimc_single.h"

static struct v4l2_input s3c_fimc_input_types[] = {
	{
		.index		= 0,
		.name		= "External Camera Input",
		.type		= V4L2_INPUT_TYPE_CAMERA,
		.audioset	= 1,
		.tuner		= 0,
		.std		= V4L2_STD_PAL_BG | V4L2_STD_NTSC_M,
		.status		= 0,
	}, 
	{
		.index		= 1,
		.name		= "Memory Input",
		.type		= V4L2_INPUT_TYPE_MEMORY,
		.audioset	= 2,
		.tuner		= 0,
		.std		= V4L2_STD_PAL_BG | V4L2_STD_NTSC_M,
		.status		= 0,
	}
};

static struct v4l2_output s3c_fimc_output_types[] = {
	{
		.index		= 0,
		.name		= "Memory Output",
		.type		= V4L2_OUTPUT_TYPE_MEMORY,
		.audioset	= 0,
		.modulator	= 0, 
		.std		= 0,
	}, 
	{
		.index		= 1,
		.name		= "LCD FIFO Output",
		.type		= V4L2_OUTPUT_TYPE_LCDFIFO,
		.audioset	= 0,
		.modulator	= 0,
		.std		= 0,
	} 
};

const static struct v4l2_fmtdesc s3c_fimc_capture_formats[] = {
	{
		.index		= 0,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "4:2:0, planar, Y-Cb-Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV420,
	},
	{
		.index		= 1,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "4:2:2, planar, Y-Cb-Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV422P,

	},	
	{
		.index		= 2,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "4:2:2, packed, YCBYCR",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
	},
	{
		.index		= 3,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "4:2:2, packed, CBYCRY",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
	}
};

const static struct v4l2_fmtdesc s3c_fimc_overlay_formats[] = {
	{
		.index		= 0,
		.type		= V4L2_BUF_TYPE_VIDEO_OVERLAY,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "16 bpp RGB, le",
		.pixelformat	= V4L2_PIX_FMT_RGB565,		
	},
	{
		.index		= 1,
		.type		= V4L2_BUF_TYPE_VIDEO_OVERLAY,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "24 bpp RGB, le",
		.pixelformat	= V4L2_PIX_FMT_RGB24,		
	},
};

#define S3C_FIMC_MAX_INPUT_TYPES	ARRAY_SIZE(s3c_fimc_input_types)
#define S3C_FIMC_MAX_OUTPUT_TYPES	ARRAY_SIZE(s3c_fimc_output_types)
#define S3C_FIMC_MAX_CAPTURE_FORMATS	ARRAY_SIZE(s3c_fimc_capture_formats)
#define S3C_FIMC_MAX_OVERLAY_FORMATS	ARRAY_SIZE(s3c_fimc_overlay_formats)

static int s3c_fimc_v4l2_querycap(struct file *filp, void *fh,
					struct v4l2_capability *cap)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;

	strcpy(cap->driver, "Samsung FIMC Driver");
	strlcpy(cap->card, ctrl->vd->name, sizeof(cap->card));
	sprintf(cap->bus_info, "FIMC AHB-bus");

	cap->version = 0;
	cap->capabilities = (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT | \
				V4L2_CAP_STREAMING);

	return 0;
}

static int s3c_fimc_v4l2_g_fbuf(struct file *filp, void *fh,
					struct v4l2_framebuffer *fb)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;

	*fb = ctrl->v4l2.frmbuf;

	fb->base = ctrl->v4l2.frmbuf.base;
	fb->capability = V4L2_FBUF_CAP_LIST_CLIPPING;

	fb->fmt.pixelformat  = ctrl->v4l2.frmbuf.fmt.pixelformat;
	fb->fmt.width = ctrl->v4l2.frmbuf.fmt.width;
	fb->fmt.height = ctrl->v4l2.frmbuf.fmt.height;
	fb->fmt.bytesperline = ctrl->v4l2.frmbuf.fmt.bytesperline;

	return 0;
}

static int s3c_fimc_v4l2_s_fbuf(struct file *filp, void *fh,
					struct v4l2_framebuffer *fb)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;
	struct v4l2_framebuffer *frmbuf = &(ctrl->v4l2.frmbuf);
	int i, bpp;

	for (i = 0; i < S3C_FIMC_MAX_OVERLAY_FORMATS; i++) {
		if (s3c_fimc_overlay_formats[i].pixelformat == fb->fmt.pixelformat)
			break;
	}

	if (i == S3C_FIMC_MAX_OVERLAY_FORMATS)
		return -EINVAL;

	bpp = s3c_fimc_set_output_frame(ctrl, &fb->fmt);

	frmbuf->base  = fb->base;
	frmbuf->flags = fb->flags;
	frmbuf->capability = fb->capability;
	frmbuf->fmt.width = fb->fmt.width;
	frmbuf->fmt.height = fb->fmt.height;
	frmbuf->fmt.field = fb->fmt.field;
	frmbuf->fmt.pixelformat = fb->fmt.pixelformat;
	frmbuf->fmt.bytesperline = fb->fmt.width * bpp / 8;
	frmbuf->fmt.sizeimage = fb->fmt.width * frmbuf->fmt.bytesperline;

	return 0;
}

static int s3c_fimc_v4l2_enum_fmt_vid_cap(struct file *filp, void *fh,
					struct v4l2_fmtdesc *f)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;
	int index = f->index;

	if (index >= S3C_FIMC_MAX_CAPTURE_FORMATS)
		return -EINVAL;

	memset(f, 0, sizeof(*f));
	memcpy(f, ctrl->v4l2.fmtdesc + index, sizeof(*f));

	return 0;
}

static int s3c_fimc_v4l2_g_fmt_vid_cap(struct file *filp, void *fh,
					struct v4l2_format *f)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;
	int size = sizeof(struct v4l2_pix_format);

	memset(&f->fmt.pix, 0, size);
	memcpy(&f->fmt.pix, &(ctrl->v4l2.frmbuf.fmt), size);

	return 0;
}

static int s3c_fimc_v4l2_s_fmt_vid_cap(struct file *filp, void *fh,
					struct v4l2_format *f)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;
	struct s3c_fimc_out_frame *frame = &ctrl->out_frame;

	ctrl->v4l2.frmbuf.fmt = f->fmt.pix;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (frame->hq && f->fmt.pix.pixelformat == V4L2_PIX_FMT_JPEG) {
		frame->jpeg_enc = 1;
		frame->jpeg_thumb = 1;
	}

	if (frame->hq && f->fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG)
		frame->jpeg_enc = 1;

	ctrl->out_type = PATH_OUT_DMA;
	s3c_fimc_set_output_frame(ctrl, &f->fmt.pix);

	return 0;
}

static int s3c_fimc_v4l2_s_fmt_vid_out(struct file *filp, void *fh,
					struct v4l2_format *f)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;
	struct v4l2_format out_fmt;

	ctrl->v4l2.frmbuf.fmt = f->fmt.pix;

	if (f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	ctrl->in_type = PATH_IN_DMA;
	s3c_fimc_set_input_frame(ctrl, &f->fmt.pix);

	out_fmt.fmt.pix.width = f->fmt.pix.width;
	out_fmt.fmt.pix.height = f->fmt.pix.height;
	out_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
	out_fmt.fmt.pix.field = V4L2_FIELD_NONE;

	ctrl->out_type = PATH_OUT_LCDFIFO;
	s3c_fimc_set_output_frame(ctrl, &out_fmt.fmt.pix);

	return 0;
}

static int s3c_fimc_v4l2_try_fmt_vid_cap(struct file *filp, void *fh,
					  struct v4l2_format *f)
{
	return 0;
}

static int s3c_fimc_v4l2_try_fmt_vid_out(struct file *filp, void *fh,
					  struct v4l2_format *f)
{
	return 0;
}

static int s3c_fimc_v4l2_try_fmt_overlay(struct file *filp, void *fh,
					  struct v4l2_format *f)
{
	return 0;
}

static int s3c_fimc_v4l2_overlay(struct file *filp, void *fh, unsigned int i)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;

	if (i) {
		if (ctrl->in_type != PATH_IN_DMA)
			s3c_fimc_init_camera(ctrl);

		FSET_PREVIEW(ctrl);
		s3c_fimc_start_dma(ctrl);
	} else {
		s3c_fimc_stop_dma(ctrl);

		if (ctrl->out_type != PATH_OUT_LCDFIFO) {
			s3c_fimc_free_output_memory(&ctrl->out_frame);
			s3c_fimc_set_output_address(ctrl);
		}
	}
	
	return 0;
}

static int s3c_fimc_v4l2_g_ctrl(struct file *filp, void *fh,
					struct v4l2_control *c)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;
	struct s3c_fimc_out_frame *frame = &ctrl->out_frame;

	switch (c->id) {
	case V4L2_CID_OUTPUT_ADDR:
		c->value = frame->addr[c->value].phys_y;
		break;

	case V4L2_CID_JPEG_SIZE:
		c->value = frame->jpeg_size;
		break;

	case V4L2_CID_THUMBNAIL_SIZE:
		c->value = frame->thumb_size;
		break;

	default:
		err("invalid control id: %d\n", c->id);
		return -EINVAL;
	}
	
	return 0;
}

static int s3c_fimc_v4l2_s_ctrl(struct file *filp, void *fh,
					struct v4l2_control *c)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;
	struct s3c_fimc_out_frame *frame = &ctrl->out_frame;
	struct s3c_fimc_window_offset *offset = &ctrl->in_cam->offset;

	switch (c->id) {
	case V4L2_CID_EFFECT_ORIGINAL:
		frame->effect.type = EFFECT_ORIGINAL;
		s3c_fimc_change_effect(ctrl);
		break;

	case V4L2_CID_EFFECT_NEGATIVE:
		frame->effect.type = EFFECT_NEGATIVE;
		s3c_fimc_change_effect(ctrl);
		break;

	case V4L2_CID_EFFECT_EMBOSSING:
		frame->effect.type = EFFECT_EMBOSSING;
		s3c_fimc_change_effect(ctrl);
		break;

	case V4L2_CID_EFFECT_ARTFREEZE:
		frame->effect.type = EFFECT_ARTFREEZE;
		s3c_fimc_change_effect(ctrl);
		break;

	case V4L2_CID_EFFECT_SILHOUETTE:
		frame->effect.type = EFFECT_SILHOUETTE;
		s3c_fimc_change_effect(ctrl);
		break;

	case V4L2_CID_EFFECT_ARBITRARY:
		frame->effect.type = EFFECT_ARBITRARY;
		frame->effect.pat_cb = PAT_CB(c->value);
		frame->effect.pat_cr = PAT_CR(c->value);
		s3c_fimc_change_effect(ctrl);
		break;

	case V4L2_CID_ROTATE_ORIGINAL:
		frame->flip = FLIP_ORIGINAL;
		ctrl->rot90 = 0;
		s3c_fimc_change_rotate(ctrl);
		break;

	case V4L2_CID_HFLIP:
		frame->flip = FLIP_X_AXIS;
		ctrl->rot90 = 0;
		s3c_fimc_change_rotate(ctrl);
		break;

	case V4L2_CID_VFLIP:
		frame->flip = FLIP_Y_AXIS;
		ctrl->rot90 = 0;
		s3c_fimc_change_rotate(ctrl);
		break;

	case V4L2_CID_ROTATE_180:
		frame->flip = FLIP_XY_AXIS;
		ctrl->rot90 = 0;
		s3c_fimc_change_rotate(ctrl);
		break;

	case V4L2_CID_ROTATE_90:
		frame->flip = FLIP_ORIGINAL;
		ctrl->rot90 = 1;
		s3c_fimc_change_rotate(ctrl);
		break;

	case V4L2_CID_ROTATE_270:
		frame->flip = FLIP_XY_AXIS;
		ctrl->rot90 = 1;
		s3c_fimc_change_rotate(ctrl);
		break;

	case V4L2_CID_ROTATE_90_HFLIP:
		frame->flip = FLIP_X_AXIS;
		ctrl->rot90 = 1;
		s3c_fimc_change_rotate(ctrl);
		break;

	case V4L2_CID_ROTATE_90_VFLIP:
		frame->flip = FLIP_Y_AXIS;
		ctrl->rot90 = 1;
		s3c_fimc_change_rotate(ctrl);
		break;

	case V4L2_CID_ZOOM_IN:
		if (s3c_fimc_check_zoom(ctrl, c->id) == 0) {
			offset->h1 += S3C_FIMC_ZOOM_PIXELS;
			offset->h2 += S3C_FIMC_ZOOM_PIXELS;
			offset->v1 += S3C_FIMC_ZOOM_PIXELS;
			offset->v2 += S3C_FIMC_ZOOM_PIXELS;
			s3c_fimc_restart_dma(ctrl);
		}

		break;

	case V4L2_CID_ZOOM_OUT:
		if (s3c_fimc_check_zoom(ctrl, c->id) == 0) {
			offset->h1 -= S3C_FIMC_ZOOM_PIXELS;
			offset->h2 -= S3C_FIMC_ZOOM_PIXELS;
			offset->v1 -= S3C_FIMC_ZOOM_PIXELS;
			offset->v2 -= S3C_FIMC_ZOOM_PIXELS;
			s3c_fimc_restart_dma(ctrl);
		}

		break;

	case V4L2_CID_AUTO_WHITE_BALANCE:
		s3c_fimc_i2c_command(ctrl, I2C_CAM_WB, c->value);
		break;

	case V4L2_CID_ACTIVE_CAMERA:
		s3c_fimc_set_active_camera(ctrl, c->value);
		s3c_fimc_i2c_command(ctrl, I2C_CAM_WB, WB_AUTO);
		break;

	case V4L2_CID_TEST_PATTERN:
		s3c_fimc_set_active_camera(ctrl, S3C_FIMC_TPID);
		s3c_fimc_set_test_pattern(ctrl, c->value);
		break;

	case V4L2_CID_NR_FRAMES:
		s3c_fimc_set_nr_frames(ctrl, c->value);
		break;

	case V4L2_CID_INPUT_ADDR:
		s3c_fimc_alloc_input_memory(&ctrl->in_frame, \
						(dma_addr_t) c->value);
		s3c_fimc_set_input_address(ctrl);
		break;

	case V4L2_CID_INPUT_ADDR_Y:
	case V4L2_CID_INPUT_ADDR_RGB:
		s3c_fimc_alloc_y_memory(&ctrl->in_frame, \
						(dma_addr_t) c->value);
		s3c_fimc_set_input_address(ctrl);
		break;

	case V4L2_CID_INPUT_ADDR_CB:	/* fall through */
	case V4L2_CID_INPUT_ADDR_CBCR:
		s3c_fimc_alloc_cb_memory(&ctrl->in_frame, \
						(dma_addr_t) c->value);
		s3c_fimc_set_input_address(ctrl);
		break;

	case V4L2_CID_INPUT_ADDR_CR:
		s3c_fimc_alloc_cr_memory(&ctrl->in_frame, \
						(dma_addr_t) c->value);
		s3c_fimc_set_input_address(ctrl);
		break;

	case V4L2_CID_RESET:
		ctrl->rot90 = 0;
		ctrl->in_frame.flip = FLIP_ORIGINAL;
		ctrl->out_frame.flip = FLIP_ORIGINAL;
		ctrl->out_frame.effect.type = EFFECT_ORIGINAL;
		ctrl->scaler.bypass = 0;
		s3c_fimc_reset(ctrl);
		break;

	case V4L2_CID_JPEG_INPUT:	/* fall through */
	case V4L2_CID_SCALER_BYPASS:
		ctrl->scaler.bypass = 1;
		break;

	case V4L2_CID_JPEG_QUALITY:
		frame->jpeg_quality = c->value - 1;
		break;

	default:
		err("invalid control id: %d\n", c->id);
		return -EINVAL;
	}

	return 0;
}

static int s3c_fimc_v4l2_streamon(struct file *filp, void *fh,
					enum v4l2_buf_type i)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;
	
	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE && \
		i != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	if (ctrl->in_type != PATH_IN_DMA)
		s3c_fimc_init_camera(ctrl);

	ctrl->out_frame.skip_frames = 0;
	FSET_CAPTURE(ctrl);
	FSET_IRQ_X(ctrl);
	s3c_fimc_start_dma(ctrl);

	return 0;
}

static int s3c_fimc_v4l2_streamoff(struct file *filp, void *fh,
					enum v4l2_buf_type i)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;
	struct s3c_fimc_out_frame *frame = &ctrl->out_frame;
	
	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE && \
		i != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	FSET_STOP(ctrl);
	FCLR_USAGE(ctrl);
	FCLR_IRQ(ctrl);

	s3cfb_direct_ioctl(1, S3CFB_WIN_OFF, 0);

	s3c_fimc_stop_dma(ctrl);
	s3c_fimc_free_output_memory(&ctrl->out_frame);
	s3c_fimc_set_output_address(ctrl);
	s3c_fimc_free_input_memory(&ctrl->in_frame);
	s3c_fimc_set_input_address(ctrl);

	frame->hq = 0;
	frame->jpeg_enc = 0;
	frame->jpeg_thumb = 0;
	frame->jpeg_size = 0;
	frame->thumb_size = 0;

	return 0;
}

static int s3c_fimc_v4l2_g_input(struct file *filp, void *fh,
					unsigned int *i)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;

	*i = ctrl->v4l2.input->index;

	return 0;
}

static int s3c_fimc_v4l2_s_input(struct file *filp, void *fh,
					unsigned int i)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;

	if (i >= S3C_FIMC_MAX_INPUT_TYPES)
		return -EINVAL;

	ctrl->v4l2.input = &s3c_fimc_input_types[i];

	if (s3c_fimc_input_types[i].type == V4L2_INPUT_TYPE_CAMERA)
		ctrl->in_type = PATH_IN_ITU_CAMERA;
	else
		ctrl->in_type = PATH_IN_DMA;

	return 0;
}

static int s3c_fimc_v4l2_g_output(struct file *filp, void *fh,
					unsigned int *i)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;

	*i = ctrl->v4l2.output->index;

	return 0;
}

static int s3c_fimc_v4l2_s_output(struct file *filp, void *fh,
					unsigned int i)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;

	if (i >= S3C_FIMC_MAX_OUTPUT_TYPES)
		return -EINVAL;

	ctrl->v4l2.output = &s3c_fimc_output_types[i];

	if (s3c_fimc_output_types[i].type == V4L2_OUTPUT_TYPE_MEMORY)
		ctrl->out_type = PATH_OUT_DMA;
	else
		ctrl->out_type = PATH_OUT_LCDFIFO;

	return 0;
}

static int s3c_fimc_v4l2_enum_input(struct file *filp, void *fh,
					struct v4l2_input *i)
{
	if (i->index >= S3C_FIMC_MAX_INPUT_TYPES)
		return -EINVAL;

	memcpy(i, &s3c_fimc_input_types[i->index], sizeof(struct v4l2_input));

	return 0;
}

static int s3c_fimc_v4l2_enum_output(struct file *filp, void *fh,
					struct v4l2_output *o)
{
	if ((o->index) >= S3C_FIMC_MAX_OUTPUT_TYPES)
		return -EINVAL;

	memcpy(o, &s3c_fimc_output_types[o->index], sizeof(struct v4l2_output));

	return 0;
}

extern int s3c_fimc_alloc_input_memory_volans
	(struct s3c_fimc_control *ctrl, struct s3c_fimc_in_frame *info);

static int s3c_fimc_v4l2_reqbufs(struct file *filp, void *fh,
					struct v4l2_requestbuffers *b)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;

	if (b->memory != V4L2_MEMORY_MMAP) {
		err("V4L2_MEMORY_MMAP is only supported\n");
		return -EINVAL;
	}

	if (b->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		b->count = 4;
		if (ctrl->in_frame.addr.virt_y == NULL) {
			if (s3c_fimc_alloc_input_memory_volans(ctrl, \
				&ctrl->in_frame))
				err("cannot allocate input memory\n");
		}
	} else if (b->type == V4L2_BUF_TYPE_VIDEO_CAPTURE){
		if (b->count > 4)
			b->count = 4;
		else if (b->count == 3)
			b->count = 4;
		else if (b->count < 1)
			b->count = 1;

		ctrl->out_frame.nr_frames = b->count;
		if (ctrl->out_frame.addr[0].virt_y != NULL)
			s3c_fimc_free_output_memory(&ctrl->out_frame);

		if (s3c_fimc_alloc_output_memory(ctrl, &ctrl->out_frame))
			err("cannot allocate output memory\n");
	} else
		return -EINVAL;

	return 0;
}

static int s3c_fimc_v4l2_querybuf(struct file *filp, void *fh,
					struct v4l2_buffer *b)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;

	if (b->memory != V4L2_MEMORY_MMAP)
		return -EINVAL;

	if (b->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		b->length = ctrl->out_frame.buf_size;
		b->m.offset = b->index * PAGE_SIZE;
	} else if (b->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		b->length = ctrl->in_frame.buf_size;
		b->m.offset = 0;
	} else
		return -EINVAL;

	/*
	 * NOTE: we use the m.offset as an index for multiple frames out.
	 * Because all frames are not contiguous, we cannot use it as
	 * original purpose.
	 * The index value used to find out which frame user wants to mmap.
	 */

	return 0;
}

static int s3c_fimc_v4l2_qbuf(struct file *filp, void *fh,
				struct v4l2_buffer *b)
{
	return 0;
}

static int s3c_fimc_v4l2_dqbuf(struct file *filp, void *fh,
				struct v4l2_buffer *b)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;
	struct s3c_fimc_out_frame *frame = &ctrl->out_frame;
	
	ctrl->out_frame.cfn = s3c_fimc_get_frame_count(ctrl);
	b->index = (frame->cfn + 2) % frame->nr_frames;

	if (frame->jpeg_enc) {
		s3c_fimc_stop_dma(ctrl);
		s3c_fimc_encode_jpeg(frame, b->index);
	}

	return 0;
}

static int s3c_fimc_v4l2_cropcap(struct file *filp, void *fh,
					struct v4l2_cropcap *a)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE && \
		a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	/* crop limitations */
	ctrl->v4l2.crop_bounds.left = 0;
	ctrl->v4l2.crop_bounds.top = 0;
	ctrl->v4l2.crop_bounds.width = 400;
	ctrl->v4l2.crop_bounds.height = 240;

	/* crop default values */
	ctrl->v4l2.crop_defrect.left = 0;
	ctrl->v4l2.crop_defrect.top = 0;

	ctrl->v4l2.crop_defrect.width = 176;
	ctrl->v4l2.crop_defrect.height = 144;

	a->bounds = ctrl->v4l2.crop_bounds;
	a->defrect = ctrl->v4l2.crop_defrect;

	return 0;
}

static int s3c_fimc_v4l2_g_crop(struct file *filp, void *fh,
				struct v4l2_crop *a)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE && \
		a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	a->c = ctrl->v4l2.crop_current;

	return 0;
}

static int s3c_fimc_v4l2_s_crop(struct file *filp, void *fh,
				struct v4l2_crop *a)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE && \
		a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	if (a->c.height < 0)
		return -EINVAL;

	if (a->c.width < 0)
		return -EINVAL;

	if ((a->c.left + a->c.width > 400) || \
		(a->c.top + a->c.height > 240))
		return -EINVAL;

	ctrl->v4l2.crop_current = a->c;

	return 0;
}

static int s3c_fimc_v4l2_s_parm(struct file *filp, void *fh,
				struct v4l2_streamparm *a)
{
	struct s3c_fimc_control *ctrl = (struct s3c_fimc_control *) fh;
	struct s3c_fimc_out_frame *frame = &ctrl->out_frame;

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (a->parm.capture.capturemode == V4L2_MODE_HIGHQUALITY)
		frame->hq = 1;

	return 0;
}

const struct v4l2_ioctl_ops s3c_fimc_v4l2_ops = {
	.vidioc_querycap		= s3c_fimc_v4l2_querycap,
	.vidioc_g_fbuf			= s3c_fimc_v4l2_g_fbuf,
	.vidioc_s_fbuf			= s3c_fimc_v4l2_s_fbuf,
	.vidioc_enum_fmt_vid_cap	= s3c_fimc_v4l2_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= s3c_fimc_v4l2_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= s3c_fimc_v4l2_s_fmt_vid_cap,
	.vidioc_s_fmt_vid_out		= s3c_fimc_v4l2_s_fmt_vid_out,
	.vidioc_try_fmt_vid_cap		= s3c_fimc_v4l2_try_fmt_vid_cap,
	.vidioc_try_fmt_vid_out		= s3c_fimc_v4l2_try_fmt_vid_out,
	.vidioc_try_fmt_vid_overlay	= s3c_fimc_v4l2_try_fmt_overlay,
	.vidioc_overlay			= s3c_fimc_v4l2_overlay,
	.vidioc_g_ctrl			= s3c_fimc_v4l2_g_ctrl,
	.vidioc_s_ctrl			= s3c_fimc_v4l2_s_ctrl,
	.vidioc_streamon		= s3c_fimc_v4l2_streamon,
	.vidioc_streamoff		= s3c_fimc_v4l2_streamoff,
	.vidioc_g_input			= s3c_fimc_v4l2_g_input,
	.vidioc_s_input			= s3c_fimc_v4l2_s_input,
	.vidioc_g_output		= s3c_fimc_v4l2_g_output,
	.vidioc_s_output		= s3c_fimc_v4l2_s_output,
	.vidioc_enum_input		= s3c_fimc_v4l2_enum_input,
	.vidioc_enum_output		= s3c_fimc_v4l2_enum_output,
	.vidioc_reqbufs			= s3c_fimc_v4l2_reqbufs,
	.vidioc_querybuf		= s3c_fimc_v4l2_querybuf,
	.vidioc_qbuf			= s3c_fimc_v4l2_qbuf,
	.vidioc_dqbuf			= s3c_fimc_v4l2_dqbuf,
	.vidioc_cropcap			= s3c_fimc_v4l2_cropcap,
	.vidioc_g_crop			= s3c_fimc_v4l2_g_crop,
	.vidioc_s_crop			= s3c_fimc_v4l2_s_crop,
	.vidioc_s_parm			= s3c_fimc_v4l2_s_parm,
};
