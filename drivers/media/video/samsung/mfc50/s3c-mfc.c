/*
 * drivers/media/video/samsung/mfc50/s3c-mfc.c
 *
 * C file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Jaeryul Oh, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <plat/media.h>
#include <plat/clock.h>
#include <plat/mfc.h>
#include <plat/map.h>
#include <plat/regs-clock.h>

#include "s3c_mfc_interface.h"
#include "s3c_mfc_common.h"
#include "s3c_mfc_logmsg.h"
#include "s3c_mfc_opr.h"
#include "s3c_mfc_intr.h"
#include "s3c_mfc_memory.h"
#include "s3c_mfc_buffer_manager.h"

struct s3c_mfc_ctrl s3c_mfc;
struct s3c_mfc_ctrl *ctrl = &s3c_mfc;
struct s3c_mfc_sys  s3c_mfc_sclk;
struct s3c_mfc_sys  *mfc_sys = &s3c_mfc_sclk;
unsigned int align_margin;

static int s3c_mfc_openhandle_count = 0;
static struct resource *s3c_mfc_mem;
void __iomem *s3c_mfc_sfr_virt_base;

unsigned int s3c_mfc_int_type = 0;
unsigned int s3c_mfc_err_type;

static struct mutex s3c_mfc_mutex;

unsigned int s3c_mfc_phys_buf, s3c_mfc_phys_dpb_luma_buf;
unsigned int s3c_mfc_buf_size, s3c_mfc_dpb_luma_buf_size;
volatile unsigned char *s3c_mfc_virt_buf, *s3c_mfc_virt_dpb_luma_buf;

extern s3c_mfc_alloc_mem_t *s3c_mfc_alloc_mem_head[MFC_MAX_PORT_NUM];
extern s3c_mfc_alloc_mem_t *s3c_mfc_alloc_mem_tail[MFC_MAX_PORT_NUM];

DECLARE_WAIT_QUEUE_HEAD(s3c_mfc_wait_queue);

#ifdef DETECT_FRMAE_DROP
extern	unsigned long long mfc_mid;
#endif

static int s3c_mfc_open(struct inode *inode, struct file *file)
{
	s3c_mfc_inst_ctx *mfc_ctx;
	int ret;
	//struct s3c_mfc_ctrl   *ctrl = &s3c_mfc;

	mutex_lock(&s3c_mfc_mutex);

	s3c_mfc_openhandle_count++;
	if (s3c_mfc_openhandle_count == 1) {

		clk_enable(ctrl->clock);
		clk_enable(mfc_sys->clock);

		/* MFC Hardware Initialization */
		if (s3c_mfc_init_hw() < 0){
			ret = -ENODEV;
			goto out_open;
		}
	}

	mfc_ctx = (s3c_mfc_inst_ctx *) kmalloc(sizeof(s3c_mfc_inst_ctx), GFP_KERNEL);
	if (mfc_ctx == NULL) {
		mfc_err("MFC_RET_MEM_ALLOC_FAIL\n");
		ret = -ENOMEM;
		goto out_open;
	}

	memset(mfc_ctx, 0, sizeof(s3c_mfc_inst_ctx));

	/* get the inst no allocating some part of memory among reserved memory */
	mfc_ctx->mem_inst_no = s3c_mfc_get_mem_inst_no(MEMORY);
	if (mfc_ctx->mem_inst_no < 0) {
		kfree(mfc_ctx);
		mfc_err("MFC_RET_INST_NUM_EXCEEDED_FAIL\n");
		ret = -EPERM;
		goto out_open;
	}

	if (s3c_mfc_set_state(mfc_ctx, MFCINST_STATE_OPENED) < 0) {
		mfc_err("MFC_RET_STATE_INVALID\n");
		kfree(mfc_ctx);
		ret = -ENODEV;
		goto out_open;
	}

	/* Decoder only */
	mfc_ctx->extraDPB = MFC_MAX_EXTRA_DPB;
	mfc_ctx->displayDelay = 9999;
	//mfc_ctx->sliceEnable = 0;
	mfc_ctx->FrameType = MFC_RET_FRAME_NOT_SET;

	file->private_data = (s3c_mfc_inst_ctx *) mfc_ctx;

	ret = 0;

out_open:
#if	defined(CONFIG_VIDEO_MFC_FRAME_CLK_GATING)
	s5pc11x_clk_ip0_ctrl(ctrl->clock, 0);
#endif
	mutex_unlock(&s3c_mfc_mutex);
	return ret;
}

static int s3c_mfc_release(struct inode *inode, struct file *file)
{
	s3c_mfc_inst_ctx *mfc_ctx;
	s3c_mfc_alloc_mem_t *node, *tmp_node;
	int port_no = 0;
	//struct s3c_mfc_ctrl   *ctrl = &s3c_mfc;
	int ret;

	mfc_debug("MFC Release..\n");
	mutex_lock(&s3c_mfc_mutex);

#if	defined(CONFIG_VIDEO_MFC_FRAME_CLK_GATING)
	s5pc11x_clk_ip0_ctrl(ctrl->clock, 1);
#endif

	mfc_ctx = (s3c_mfc_inst_ctx *) file->private_data;
	if (mfc_ctx == NULL) {
		mfc_err("MFC_RET_INVALID_PARAM_FAIL\n");
		ret = -EIO;
		goto out_release;
	}
	for (port_no = 0; port_no < MFC_MAX_PORT_NUM; port_no++) {
		for (node = s3c_mfc_alloc_mem_head[port_no];
				node != s3c_mfc_alloc_mem_tail[port_no];
				node = node->next) {
			if (node->inst_no == mfc_ctx->mem_inst_no) {
				tmp_node = node;
				node = node->prev;
				mfc_ctx->port_no = port_no;
				s3c_mfc_release_alloc_mem(mfc_ctx, tmp_node);
			}
		}
	}
	s3c_mfc_merge_frag(mfc_ctx->mem_inst_no);

	s3c_mfc_return_mem_inst_no(mfc_ctx->mem_inst_no);

	if ((mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) ||
			(mfc_ctx->MfcState >= MFCINST_STATE_ENC_INITIALIZE))
		s3c_mfc_return_inst_no(mfc_ctx->InstNo, mfc_ctx->MfcCodecType);

	s3c_mfc_openhandle_count--;
	kfree(mfc_ctx);

	ret = 0;
	/* In EVT1, it should be tested */
#if 1
	if (s3c_mfc_openhandle_count == 0) {
		clk_disable(mfc_sys->clock);
		clk_disable(ctrl->clock);
	}
#endif

out_release:
#if	defined(CONFIG_VIDEO_MFC_FRAME_CLK_GATING)
	s5pc11x_clk_ip0_ctrl(ctrl->clock, 0);
#endif
	mutex_unlock(&s3c_mfc_mutex);
	return ret;
}

static int s3c_mfc_ioctl(struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{
	int ret, ex_ret;
	s3c_mfc_frame_buf_arg_t frame_buf_size;
	s3c_mfc_inst_ctx *mfc_ctx = NULL;
	s3c_mfc_common_args in_param;
	s3c_mfc_alloc_mem_t *node;
	int port_no = 0;
	int matched_u_addr = 0;
	//unsigned char *start;
	//int size;

	mutex_lock(&s3c_mfc_mutex);

#if	defined(CONFIG_VIDEO_MFC_FRAME_CLK_GATING)
	s5pc11x_clk_ip0_ctrl(ctrl->clock, 1);
#endif

	ret =
		copy_from_user(&in_param, (s3c_mfc_common_args *) arg,
				sizeof(s3c_mfc_common_args));
	if (ret < 0) {
		mfc_err("Inparm copy error\n");
		ret = -EIO;
		in_param.ret_code = MFC_RET_INVALID_PARAM_FAIL;
		goto out_ioctl;
	}

	mfc_ctx = (s3c_mfc_inst_ctx *) file->private_data;
	mutex_unlock(&s3c_mfc_mutex);

	switch (cmd) {
		case IOCTL_MFC_ENC_INIT:

			mutex_lock(&s3c_mfc_mutex);
			mfc_debug("IOCTL_MFC_ENC_INIT\n");
			if (s3c_mfc_set_state(mfc_ctx, MFCINST_STATE_ENC_INITIALIZE) <
					0) {
				mfc_err("MFC_RET_STATE_INVALID\n");
				in_param.ret_code = MFC_RET_STATE_INVALID;
				ret = -EINVAL;
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			/* MFC encode init */
			in_param.ret_code =
				s3c_mfc_init_encode(mfc_ctx, &(in_param.args));
			if (in_param.ret_code < 0) {
				ret = in_param.ret_code;
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			/* Allocate MFC buffer(stream, refYC, MV) */
			in_param.ret_code =
				s3c_mfc_allocate_stream_ref_buf(mfc_ctx, &(in_param.args));
			if (in_param.ret_code < 0) {
				ret = in_param.ret_code;
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			/* cache clean */

			/* Set Ref YC0~3 & MV */
			in_param.ret_code =
				s3c_mfc_set_enc_ref_buffer(mfc_ctx, &(in_param.args));
			if (in_param.ret_code < 0) {
				ret = in_param.ret_code;
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			in_param.ret_code =
				s3c_mfc_encode_header(mfc_ctx, &(in_param.args));
			if (in_param.ret_code < 0) {
				ret = in_param.ret_code;
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			mutex_unlock(&s3c_mfc_mutex);
			break;

		case IOCTL_MFC_ENC_EXE:

			mutex_lock(&s3c_mfc_mutex);
			if (s3c_mfc_set_state(mfc_ctx, MFCINST_STATE_ENC_EXE) < 0) {
				mfc_err("MFC_RET_STATE_INVALID\n");
				in_param.ret_code = MFC_RET_STATE_INVALID;
				ret = -EINVAL;
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			in_param.ret_code =
				s3c_mfc_exe_encode(mfc_ctx, &(in_param.args));
			//mfc_debug("InParm->ret_code : %d\n", in_param.ret_code);
			ret = in_param.ret_code;
			mutex_unlock(&s3c_mfc_mutex);
			break;

		case IOCTL_MFC_DEC_INIT:

			mutex_lock(&s3c_mfc_mutex);
			mfc_debug("IOCTL_MFC_DEC_INIT\n");
			if (s3c_mfc_set_state(mfc_ctx, MFCINST_STATE_DEC_INITIALIZE) <
					0) {
				mfc_err("MFC_RET_STATE_INVALID\n");
				in_param.ret_code = MFC_RET_STATE_INVALID;
				ret = -EINVAL;
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			/* MFC decode init */
			in_param.ret_code =
				s3c_mfc_init_decode(mfc_ctx, &(in_param.args));
			if (in_param.ret_code < 0) {
				ret = in_param.ret_code;
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			if (in_param.args.dec_init.out_dpb_cnt <= 0) {
				mfc_err("MFC out_dpb_cnt error\n");
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			/* Get frame buf size */
			frame_buf_size =
				s3c_mfc_get_frame_buf_size(mfc_ctx, &(in_param.args));

			/* Allocate MFC buffer(Y, C, MV) */
			in_param.ret_code =
				s3c_mfc_allocate_frame_buf(mfc_ctx, &(in_param.args),
						frame_buf_size);
			if (in_param.ret_code < 0) {
				ret = in_param.ret_code;
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			/* Set DPB buffer */
			in_param.ret_code =
				s3c_mfc_set_dec_frame_buffer(mfc_ctx, &(in_param.args));
			//s3c_mfc_set_dec_frame_buffer(mfc_ctx, dec_arg->in_frm_buf, dec_arg->in_frm_size);
			if (in_param.ret_code < 0) {
				ret = in_param.ret_code;
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			mutex_unlock(&s3c_mfc_mutex);
			break;

		case IOCTL_MFC_DEC_EXE:

			mutex_lock(&s3c_mfc_mutex);
			mfc_debug("IOCTL_MFC_DEC_EXE\n");
			if (s3c_mfc_set_state(mfc_ctx, MFCINST_STATE_DEC_EXE) < 0) {
				mfc_err("MFC_RET_STATE_INVALID\n");
				in_param.ret_code = MFC_RET_STATE_INVALID;
				ret = -EINVAL;
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			in_param.ret_code =
				s3c_mfc_exe_decode(mfc_ctx, &(in_param.args));
			ret = in_param.ret_code;
			mutex_unlock(&s3c_mfc_mutex);
			break;

		case IOCTL_MFC_GET_CONFIG:

			mutex_lock(&s3c_mfc_mutex);
			if (mfc_ctx->MfcState < MFCINST_STATE_DEC_INITIALIZE) {
				mfc_err("MFC_RET_STATE_INVALID\n");
				in_param.ret_code = MFC_RET_STATE_INVALID;
				ret = -EINVAL;
				mutex_unlock(&s3c_mfc_mutex);
				break;
			}

			in_param.ret_code =
				s3c_mfc_get_config(mfc_ctx, &(in_param.args));
			ret = in_param.ret_code;
			mutex_unlock(&s3c_mfc_mutex);
			break;

		case IOCTL_MFC_SET_CONFIG:

			mutex_lock(&s3c_mfc_mutex);
			in_param.ret_code =
				s3c_mfc_set_config(mfc_ctx, &(in_param.args));
			ret = in_param.ret_code;
			mutex_unlock(&s3c_mfc_mutex);
			break;

		case IOCTL_MFC_GET_IN_BUF:

			if (mfc_ctx->MfcState < MFCINST_STATE_OPENED) {
				mfc_err("MFC_RET_STATE_INVALID\n");
				in_param.ret_code = MFC_RET_STATE_INVALID;
				ret = -EINVAL;
				break;
			}
			/* allocate stream buf for decoder & current YC buf for encoder */
			if (in_param.args.mem_alloc.dec_enc_type == ENCODER)
				mfc_ctx->port_no = 1;
			else
				mfc_ctx->port_no = 0;

			in_param.args.mem_alloc.buff_size =
				Align(in_param.args.mem_alloc.buff_size, 2 * BUF_L_UNIT);
			in_param.ret_code =
				s3c_mfc_get_virt_addr(mfc_ctx, &(in_param.args));

			ret = in_param.ret_code;

			break;

		case IOCTL_MFC_FREE_BUF:

			if (mfc_ctx->MfcState < MFCINST_STATE_OPENED) {
				mfc_err("MFC_RET_STATE_INVALID\n");
				in_param.ret_code = MFC_RET_STATE_INVALID;
				ret = -EINVAL;
				break;
			}

			for (port_no = 0; port_no < MFC_MAX_PORT_NUM; port_no++) {
				for (node = s3c_mfc_alloc_mem_head[port_no];
						node != s3c_mfc_alloc_mem_tail[port_no];
						node = node->next) {
					if (node->u_addr ==
							(unsigned char *)in_param.args.mem_free.
							u_addr) {
						matched_u_addr = 1;
						break;
					}
				}
				if (matched_u_addr)
					break;
			}
			if (node == s3c_mfc_alloc_mem_tail[port_no]) {
				mfc_err("invalid virtual address(0x%x)\r\n",
						in_param.args.mem_free.u_addr);
				ret = MFC_RET_MEM_INVALID_ADDR_FAIL;
			}
			mfc_ctx->port_no = port_no;

			in_param.ret_code = s3c_mfc_release_alloc_mem(mfc_ctx, node);
			ret = in_param.ret_code;

			break;

		case IOCTL_MFC_GET_PHYS_ADDR:

			if (mfc_ctx->MfcState < MFCINST_STATE_OPENED) {
				mfc_err("MFC_RET_STATE_INVALID\n");
				in_param.ret_code = MFC_RET_STATE_INVALID;
				ret = -EINVAL;

				break;
			}

			in_param.ret_code =
				s3c_mfc_get_phys_addr(mfc_ctx, &(in_param.args));
			ret = in_param.ret_code;

			break;

		case IOCTL_MFC_GET_MMAP_SIZE:

			if (mfc_ctx->MfcState < MFCINST_STATE_OPENED) {
				mfc_err("MFC_RET_STATE_INVALID\n");
				in_param.ret_code = MFC_RET_STATE_INVALID;
				ret = -EINVAL;

				break;
			}

			in_param.ret_code = MFC_RET_OK;
			ret = s3c_mfc_get_data_buf_phys_size()
				+ s3c_mfc_get_dpb_luma_buf_phys_size();

			break;

		default:

			mfc_err
				("Requested ioctl command is not defined. (ioctl cmd=0x%08x)\n",
				 cmd);
			in_param.ret_code = MFC_RET_INVALID_PARAM_FAIL;
			ret = -EINVAL;
	}

out_ioctl:

#if	defined(CONFIG_VIDEO_MFC_FRAME_CLK_GATING)
	s5pc11x_clk_ip0_ctrl(ctrl->clock, 0);
#endif

	ex_ret =
		copy_to_user((s3c_mfc_common_args *) arg, &in_param,
				sizeof(s3c_mfc_common_args));
	if (ex_ret < 0) {
		mfc_err("Outparm copy to user error\n");
		ret = -EIO;
	}

	mfc_debug("---------------IOCTL return = %d ---------------\n", ret);

	return ret;
}

static int s3c_mfc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long vir_size = vma->vm_end - vma->vm_start;
	unsigned long phy_size;
	unsigned long offset;
	unsigned long pfn;
	unsigned long remap_offset, remap_size;

	mfc_debug("vm_start= 0x%08lx, vm_end= 0x%08lx, size= %ld(%ldMB)\n",
			vma->vm_start, vma->vm_end, vir_size, (vir_size >> 20));

	offset = s3c_mfc_get_data_buf_phys_addr() - s3c_mfc_phys_buf;

	phy_size =
		(unsigned long)(s3c_mfc_buf_size + s3c_mfc_dpb_luma_buf_size -
				offset);

	/*
	 * if memory size required from appl. mmap() is bigger than max data memory
	 * size allocated in the driver
	 */
	if (vir_size > phy_size) {
		mfc_err("virtual requested mem(%ld) is bigger than physical mem(%ld)\n",
				vir_size, phy_size);
		return -EINVAL;
	}

	remap_offset = 0;
	remap_size = min((unsigned long)s3c_mfc_get_data_buf_phys_size(), vir_size);

	vma->vm_flags |= VM_RESERVED | VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	/*
	 * Port 0 mapping for stream buf & frame buf (chroma + MV)
	 */
	pfn = __phys_to_pfn(s3c_mfc_get_data_buf_phys_addr());
	if (remap_pfn_range(vma, vma->vm_start + remap_offset, pfn,
				remap_size, vma->vm_page_prot)) {

		mfc_err("mfc remap port 0 error\n");

		return -EAGAIN;
	}

	remap_offset = remap_size;
	remap_size = min((unsigned long)s3c_mfc_get_dpb_luma_buf_phys_size(), vir_size - remap_offset);

	vma->vm_flags |= VM_RESERVED | VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	/*
	 * Port 1 mapping for frame buf (luma)
	 */
	pfn = __phys_to_pfn(s3c_mfc_get_dpb_luma_buf_phys_addr());
	if (remap_pfn_range(vma, vma->vm_start + remap_offset, pfn,
				remap_size, vma->vm_page_prot)) {
		mfc_err("mfc remap port 1 error\n");
		return -EAGAIN;
	}
	mfc_debug("virtual requested mem = %ld, physical reserved data mem = %ld\n",
			vir_size, phy_size);

	if ((remap_offset + remap_size) < phy_size)
		mfc_warn("The MFC reserved memory dose not mmap fully [%ld: %ld]\n",
				phy_size, (remap_offset + remap_size));

	return 0;

}

static struct file_operations s3c_mfc_fops = {
	.owner = THIS_MODULE,
	.open = s3c_mfc_open,
	.release = s3c_mfc_release,
	.ioctl = s3c_mfc_ioctl,
	.mmap = s3c_mfc_mmap
};

static struct miscdevice s3c_mfc_miscdev = {
	.minor = 252,
	.name = "s3c-mfc",
	.fops = &s3c_mfc_fops,
};

static irqreturn_t s3c_mfc_irq(int irq, void *dev_id)
{
	unsigned int int_reason;
	unsigned int error_reason;

	int_reason =
		readl(s3c_mfc_sfr_virt_base + S3C_FIMV_RISC2HOST_CMD) & 0x1FFFF;
	mfc_debug("Interrupt !! : %d\n", int_reason);

	error_reason =
		readl(s3c_mfc_sfr_virt_base + S3C_FIMV_RISC2HOST_ARG2) & 0xFFFF;

	if ((error_reason >= MFC_ERR_START_NO) &&
			(error_reason < MFC_WARN_START_NO))
		printk("MFC fw error code : %d\n", error_reason);
	else if (error_reason >= MFC_WARN_START_NO)
		printk("MFC fw warning code : %d\n", error_reason);

	if ((int_reason >= R2H_CMD_EMPTY) && (int_reason <= R2H_CMD_ERR_RET)) {
		s3c_mfc_clear_int();
		s3c_mfc_int_type = int_reason;
		s3c_mfc_err_type = error_reason;
		wake_up_interruptible(&s3c_mfc_wait_queue);
	}
	s3c_mfc_clear_ch_id(int_reason);

#ifdef DETECT_FRMAE_DROP
	mfc_mid = sched_clock();
#endif

	return IRQ_HANDLED;
}

static int s3c_mfc_probe(struct platform_device *pdev)
{
	struct s3c_mfc_platdata *pdata = pdev->dev.platform_data;
	struct resource *res;
	size_t size;
	int ret,err;
	struct clk *sys_clk = mfc_sys->clock;
	struct clk *parent = NULL;
	//struct s3c_mfc_ctrl   *ctrl = &s3c_mfc;

	/* mfc clock enable should be here */
	sprintf(ctrl->clk_name, "%s", S3C_MFC_CLK_NAME);
	sprintf(mfc_sys->clk_name, "%s", S3C_MFC_SCLK_NAME);

	ctrl->clock = clk_get(&pdev->dev, ctrl->clk_name);
	if (IS_ERR(ctrl->clock)) {
		printk(KERN_ERR "failed to get mfc clock source\n");
		return EPERM;
	}

	sys_clk = clk_get(&pdev->dev, mfc_sys->clk_name);
	if (IS_ERR(sys_clk)) {
		printk(KERN_ERR "failed to get mfc sys clock source\n");
		return EPERM;
	}

	if(sys_clk->set_parent){
		parent = clk_get(&pdev->dev, "dout_a2m");
		if (IS_ERR(parent)) {
			dev_err(&pdev->dev, "failed to get parent of interface clock\n");
			return EPERM;
		}
		sys_clk->parent = parent;

		err = sys_clk->set_parent(sys_clk, parent);
		if (err) {
			dev_err(&pdev->dev, "failed to set parent of interface clock\n");
			return EPERM;
		}
	}

	if(sys_clk->set_rate){
		sys_clk->set_rate(sys_clk, 200000000);	
		dev_info(&pdev->dev, "set interface clock rate to 200000000\n");
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get memory region resource\n");
		ret = -ENOENT;
		goto probe_out;
	}

	/* 60K is required for mfc register (0x0 ~ 0xe008) */
	size = (res->end - res->start) + 1;
	s3c_mfc_mem = request_mem_region(res->start, size, pdev->name);
	if (s3c_mfc_mem == NULL) {
		dev_err(&pdev->dev, "failed to get memory region\n");
		ret = -ENOENT;
		goto probe_out;
	}

	s3c_mfc_sfr_virt_base =
		ioremap(s3c_mfc_mem->start,
				s3c_mfc_mem->end - s3c_mfc_mem->start + 1);
	if (s3c_mfc_sfr_virt_base == NULL) {
		dev_err(&pdev->dev, "failed to ioremap address region\n");
		ret = -ENOENT;
		goto probe_out;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get irq resource\n");
		ret = -ENOENT;
		goto probe_out;
	}

	ret =
		request_irq(res->start, s3c_mfc_irq, IRQF_DISABLED, pdev->name,
				pdev);
	if (ret != 0) {
		dev_err(&pdev->dev, "failed to install irq (%d)\n", ret);
		goto probe_out;
	}

	mutex_init(&s3c_mfc_mutex);

	/*
	 * buffer memory secure
	 */
	s3c_mfc_phys_buf = pdata->buf_phy_base[0];
	s3c_mfc_phys_buf = Align(s3c_mfc_phys_buf, 128 * BUF_L_UNIT);
	s3c_mfc_virt_buf = phys_to_virt(s3c_mfc_phys_buf);
	if (s3c_mfc_virt_buf == NULL) {
		mfc_err("fail to mapping port0 buffer\n");
		ret = -EPERM;
		goto probe_out;
	}
	align_margin = s3c_mfc_phys_buf - pdata->buf_phy_base[0];
	s3c_mfc_buf_size = pdata->buf_size[0] - align_margin;

	s3c_mfc_phys_dpb_luma_buf = pdata->buf_phy_base[1];
	s3c_mfc_phys_dpb_luma_buf =
		Align(s3c_mfc_phys_dpb_luma_buf, 128 * BUF_L_UNIT);
	s3c_mfc_virt_dpb_luma_buf = phys_to_virt(s3c_mfc_phys_dpb_luma_buf);
	if (s3c_mfc_virt_dpb_luma_buf == NULL) {
		mfc_err("fail to mapping port1 buffer\n");
		ret = -EPERM;
		goto probe_out;
	}
	s3c_mfc_dpb_luma_buf_size = pdata->buf_size[1];

	printk
		("s3c_mfc_phys_buf = 0x%08x, s3c_mfc_phys_dpb_luma_buf = 0x%08x <<\n",
		 s3c_mfc_phys_buf, s3c_mfc_phys_dpb_luma_buf);
	printk
		("s3c_mfc_virt_buf = 0x%08x, s3c_mfc_virt_dpb_luma_buf = 0x%08x <<\n",
		 (unsigned int)s3c_mfc_virt_buf,
		 (unsigned int)s3c_mfc_virt_dpb_luma_buf);

	/*
	 * MFC FW downloading
	 */
	if (s3c_mfc_load_firmware() < 0) {
		mfc_err("MFC_RET_FW_INIT_FAIL\n");
		ret = -EPERM;
		goto probe_out;
	}

	s3c_mfc_init_mem_inst_no();

	s3c_mfc_init_buffer_manager();

	ret = misc_register(&s3c_mfc_miscdev);

	return 0;

probe_out:
	dev_err(&pdev->dev, "not found (%d). \n", ret);
	return ret;
}

static int s3c_mfc_remove(struct platform_device *pdev)
{
	//struct s3c_mfc_ctrl   *ctrl = &s3c_mfc;
	clk_disable(ctrl->clock);
	clk_disable(mfc_sys->clock);

	iounmap(s3c_mfc_sfr_virt_base);
	iounmap(s3c_mfc_virt_buf);

	/* remove memory region */
	if (s3c_mfc_mem != NULL) {
		release_resource(s3c_mfc_mem);
		kfree(s3c_mfc_mem);
		s3c_mfc_mem = NULL;
	}

	free_irq(IRQ_MFC, pdev);

	mutex_destroy(&s3c_mfc_mutex);

	misc_deregister(&s3c_mfc_miscdev);

	return 0;
}

static int s3c_mfc_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret;

	mfc_debug("s3c_mfc_suspend\n");

	mutex_lock(&s3c_mfc_mutex);

	if (!s3c_mfc_is_running()) {
		mutex_unlock(&s3c_mfc_mutex);
		return 0;
	}

#if	defined(CONFIG_VIDEO_MFC_FRAME_CLK_GATING)
	s5pc11x_clk_ip0_ctrl(ctrl->clock, 1);
#endif
	ret = s3c_mfc_set_sleep();
	if (ret != MFC_RET_OK){
		mutex_unlock(&s3c_mfc_mutex);
#if	defined(CONFIG_VIDEO_MFC_FRAME_CLK_GATING)
		s5pc11x_clk_ip0_ctrl(ctrl->clock, 0);
#endif
		return ret;
	}

	//clk_disable(ctrl->clock);

#if	defined(CONFIG_VIDEO_MFC_FRAME_CLK_GATING)
	s5pc11x_clk_ip0_ctrl(ctrl->clock, 0);
#endif
	mutex_unlock(&s3c_mfc_mutex);

	return 0;
}

static int s3c_mfc_resume(struct platform_device *pdev)
{
	int ret;

	mfc_debug("s3c_mfc_resume\n");

	mutex_lock(&s3c_mfc_mutex);

	//clk_enable(ctrl->clock);

	if (!s3c_mfc_is_running()) {
		mutex_unlock(&s3c_mfc_mutex);
		return 0;
	}
#if	defined(CONFIG_VIDEO_MFC_FRAME_CLK_GATING)
	s5pc11x_clk_ip0_ctrl(ctrl->clock, 1);
#endif

	ret = s3c_mfc_set_wakeup();
	if (ret != MFC_RET_OK){
		mutex_unlock(&s3c_mfc_mutex);
#if	defined(CONFIG_VIDEO_MFC_FRAME_CLK_GATING)
		s5pc11x_clk_ip0_ctrl(ctrl->clock, 0);
#endif
		return ret;
	}

#if	defined(CONFIG_VIDEO_MFC_FRAME_CLK_GATING)
	s5pc11x_clk_ip0_ctrl(ctrl->clock, 0);
#endif

	mutex_unlock(&s3c_mfc_mutex);

	return 0;
}

static struct platform_driver s3c_mfc_driver = {
	.probe = s3c_mfc_probe,
	.remove = s3c_mfc_remove,
	.shutdown = NULL,
	.suspend = s3c_mfc_suspend,
	.resume = s3c_mfc_resume,

	.driver = {
		.owner = THIS_MODULE,
		.name = "s3c-mfc",
	},
};

static char banner[] __initdata
= KERN_INFO "S3C MFC (Multi Function Codec - FIMV5.0)"
"Device Driver, (c) 2009 Samsung Electronics\n";

static int __init s3c_mfc_init(void)
{
	mfc_info("%s", banner);

	if (platform_driver_register(&s3c_mfc_driver) != 0) {
		printk(KERN_ERR "platform device registration failed.. \n");
		return -1;
	}

	return 0;
}

static void __exit s3c_mfc_exit(void)
{
	platform_driver_unregister(&s3c_mfc_driver);
	mfc_info
		("S3C MFC (Multi Function Codec - FIMV5.0) Device Driver exit.\n");
}

module_init(s3c_mfc_init);
module_exit(s3c_mfc_exit);

MODULE_AUTHOR("Jaeryul, Oh");
MODULE_DESCRIPTION("S3C MFC (Multi Function Codec - FIMV5.0) Device Driver");
MODULE_LICENSE("GPL");
