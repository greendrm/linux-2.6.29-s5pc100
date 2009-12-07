/* module header files */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <mach/map.h>

#include <plat/regs-g2d.h>	// arch/arm/plat-s3c/include/plat/regs-g2d.h. need confirm file.
#include "fimg2d2x.h"


//#define USE_G2D_DEBUG

#ifdef USE_G2D_DEBUG
#define DPRINTK(fmt, args...) printk(KERN_INFO "%s : " fmt, __FUNCTION__, ## args)
#else
#define DPRINTK(fmt, args...)
#endif	


static struct clk *g2d_clock;
static struct clk *h_clk;

static int g2d_irq_num = NO_IRQ;
static struct resource *g2d_mem;
static void __iomem *g2d_base;
static wait_queue_head_t waitq_g2d;

static u16 g2d_poll_flag = 0;

static struct mutex *g2d_mutex;

void g2d_check_fifo(int empty_fifo)
{
	#if 0
	u32 val;
	val = __raw_readl(g2d_base + G2D_FIFO_STAT_REG);

	DPRINTK("g2d_check_fifo > val = 0x%x\n", val);

	while( G2D_FIFO_USED(val) > (FIFO_NUM - empty_fifo));
	#endif
}

static int g2d_init_regs(G2D_PARAMS* params)
{
	u32 bpp_mode;
	u32 tmp_reg;

	g2d_check_fifo(25);
	
	/* bpp */
	switch(params->bpp) {
	case ARGB8:
		bpp_mode = G2D_COLOR_MODE_REG_C0_15BPP;
		break;
	case RGB16:
		bpp_mode = G2D_COLOR_RGB_565;
		break;
	case RGB18:
		bpp_mode = G2D_COLOR_MODE_REG_C2_18BPP;
		break;
	case RGB24:
		bpp_mode = G2D_COLOR_XRGB_8888;
		break;
	default:
		bpp_mode = G2D_COLOR_MODE_REG_C3_24BPP;
		break;
	}


	/* Set register for source image */
	__raw_writel(params->src_base_addr, g2d_base + G2D_SRC_BASE_ADDR);
	__raw_writel(params->src_full_width, g2d_base + G2D_HORI_RES_REG);
	__raw_writel(params->src_full_height, g2d_base + G2D_VERT_RES_REG);
	__raw_writel((G2D_FULL_V(params->src_full_height) |
		G2D_FULL_H(params->src_full_width)),
		g2d_base + G2D_SRC_RES_REG);
	__raw_writel(bpp_mode, g2d_base + G2D_SRC_COLOR_MODE);

	/* Set register for destination image */
	__raw_writel(params->dst_base_addr, g2d_base + G2D_DST_BASE_ADDR);
	__raw_writel(params->dst_full_width, g2d_base + G2D_SC_HORI_REG);
	__raw_writel(params->dst_full_height, g2d_base + G2D_SC_VERT_REG);
	__raw_writel((G2D_FULL_V(params->dst_full_height) |
		G2D_FULL_H(params->dst_full_width)), 
		g2d_base + G2D_SC_RES_REG);
	__raw_writel(bpp_mode, g2d_base + G2D_DST_COLOR_MODE);
	

	/* Set register for clipping window */
	__raw_writel(params->cw_x1, g2d_base + G2D_CW_LT_X_REG);
	__raw_writel(params->cw_y1, g2d_base + G2D_CW_LT_Y_REG);
	__raw_writel(params->cw_x2, g2d_base + G2D_CW_RB_X_REG);
	__raw_writel(params->cw_y2, g2d_base + G2D_CW_RB_Y_REG);

	/* Set register for color */
	__raw_writel(params->color_val[G2D_WHITE], g2d_base + G2D_FG_COLOR_REG);
	__raw_writel(params->color_val[G2D_BLACK], g2d_base + G2D_BG_COLOR_REG);
	__raw_writel(params->color_val[G2D_BLUE], g2d_base + G2D_BS_COLOR_REG);

	/* Set register for ROP & Alpha */
	if(params->alpha_mode == TRUE) {
		if(params->alpha_val > ALPHA_VALUE_MAX) {
			DPRINTK("%d : g2d driver error : exceed aplha value range 0 ~ 255\n", __LINE__);
			return -ENOENT;
		}

		__raw_writel(G2D_ROP_REG_OS_FG_COLOR | 
			G2D_ROP_REG_ABM_REGISTER |
			G2D_ROP_REG_T_OPAQUE_MODE |
			G2D_ROP_SRC_ONLY,
			g2d_base + G2D_ROP_REG);
		
		/* Alpha */
		__raw_writel(G2D_ALPHA(params->alpha_val), g2d_base + G2D_ALPHA_REG);
			
	} else {
		__raw_writel(G2D_ROP_REG_OS_FG_COLOR |
			G2D_ROP_REG_ABM_NO_BLENDING |
			G2D_ROP_REG_T_OPAQUE_MODE |
			G2D_ROP_SRC_ONLY,
			g2d_base + G2D_ROP_REG);
		
		/* Alpha */
		__raw_writel(G2D_ALPHA(0x00), g2d_base + G2D_ALPHA_REG);
	}

	/* Set register for color key */
	if(params->color_key_mode == TRUE) {
		tmp_reg = __raw_readl(g2d_base + G2D_ROP_REG);
		tmp_reg |= G2D_ROP_REG_T_TRANSP_MODE;
		__raw_writel(tmp_reg, g2d_base + G2D_ROP_REG);
		__raw_writel(params->color_key_val, g2d_base + G2D_BG_COLOR_REG);
	}

	/* Set register for rotation */
	__raw_writel(G2D_ROTATE_REG_R0_0, g2d_base + G2D_ROT_OC_X_REG);
	__raw_writel(G2D_ROTATE_REG_R0_0, g2d_base + G2D_ROT_OC_Y_REG);
	__raw_writel(G2D_ROTATE_REG_R0_0, g2d_base + G2D_ROTATE_REG);

	return 0;
}

void g2d_bitblt_1(u16 src_x1, u16 src_y1, u16 src_x2, u16 src_y2,
		u16 dst_x1, u16 dst_y1, u16 dst_x2, u16 dst_y2)
{
	u32 cmd_reg_val;

	g2d_check_fifo(25);
	
	__raw_writel(src_x1, g2d_base + G2D_COORD0_X_REG);
	__raw_writel(src_y1, g2d_base + G2D_COORD0_Y_REG);
	__raw_writel(src_x2, g2d_base + G2D_COORD1_X_REG);
	__raw_writel(src_y2, g2d_base + G2D_COORD1_Y_REG);

	__raw_writel(dst_x1, g2d_base + G2D_COORD2_X_REG);
	__raw_writel(dst_y1, g2d_base + G2D_COORD2_Y_REG);
	__raw_writel(dst_x2, g2d_base + G2D_COORD3_X_REG);
	__raw_writel(dst_y2, g2d_base + G2D_COORD3_Y_REG);

	cmd_reg_val = __raw_readl(g2d_base + G2D_CMD1_REG);	/* bitblt command register */
	cmd_reg_val = ~(G2D_CMD1_REG_S | G2D_CMD1_REG_N);
	cmd_reg_val |= G2D_CMD1_REG_N;
	__raw_writel(cmd_reg_val, g2d_base + G2D_CMD1_REG);
	
}
static void g2d_rotate_start(G2D_PARAMS* params, G2D_ROT rot_degree)
{
	u16 org_x = 0, org_y = 0;
	u32 rot_reg_val = 0;
	u32 src_x1, src_y1, src_x2, src_y2, dst_x1, dst_y1, dst_x2, dst_y2;

	src_x1 = params->src_start_x;
	src_y1 = params->src_start_y;
	src_x2 = params->src_start_x + params->src_work_width;
	src_y2 = params->src_start_y + params->src_work_height;
	dst_x1 = params->dst_start_x;
	dst_y1 = params->dst_start_y;
	dst_x2 = params->dst_start_x + params->dst_work_width;
	dst_y2 = params->dst_start_y + params->dst_work_height;

	g2d_check_fifo(25);
	
	__raw_writel(G2D_INTEN_REG_CCF, g2d_base + G2D_INTEN_REG);

	g2d_get_rotation_origin(src_x1, src_y1, src_x2, src_y2,
				dst_x1, dst_y1, rot_degree, &org_x, &org_y);
	if(rot_degree != (ROT_0 || ROT_X_FLIP || ROT_Y_FLIP)) {
		org_x += 1;
		org_y += 1;
	}

	__raw_writel(org_x, g2d_base + G2D_ROT_OC_X_REG);
	__raw_writel(org_y, g2d_base + G2D_ROT_OC_Y_REG);

	switch(rot_degree) {
	case ROT_0:
		rot_reg_val = G2D_ROTATE_REG_R0_0;
		break;
	case ROT_90:
		rot_reg_val = G2D_ROTATE_REG_R1_90;
		break;
	case ROT_180:
		rot_reg_val = G2D_ROTATE_REG_R2_180;
		break;
	case ROT_270:
		rot_reg_val = G2D_ROTATE_REG_R3_270;
		break;
	case ROT_X_FLIP:
		rot_reg_val = G2D_ROTATE_REG_FX;
		break;
	case ROT_Y_FLIP:
		rot_reg_val = G2D_ROTATE_REG_FY;
		break;
	default:
		DPRINTK("%d : Wrong rotation degree \n", __LINE__);
		break;
	}
	__raw_writel(rot_reg_val, g2d_base + G2D_ROTATE_REG);


	switch(rot_degree) {
	case ROT_0:
	case ROT_X_FLIP:
	case ROT_Y_FLIP:
		g2d_bitblt_1(src_x1, src_y1, src_x2, src_y2, 
			dst_x1, dst_y1, dst_x2, dst_y2);
		break;
	case ROT_90:
		g2d_bitblt_1(src_x1, src_y1, src_x2, src_y2, 
			src_x1, src_y1+1, src_x2, src_y2+1);
		break;
	case ROT_180:
		g2d_bitblt_1(src_x1, src_y1, src_x2, src_y2, 
			src_x1+1, src_y1+1, src_x2+1, src_y2+1);
		break;
	case ROT_270:
		g2d_bitblt_1(src_x1, src_y1, src_x2, src_y2, 
			src_x1+1, src_y1, src_x2+1, src_y2);
		break;
	default:
		break;
	}

}
static void g2d_get_rotation_origin(u16 src_x1, u16 src_y1,
					u16 src_x2, u16 src_y2,
					u16 dst_x1, u16 dst_y1,
					G2D_ROT rot_degree,
					u16 *org_x, u16 *org_y)
{
	g2d_check_fifo(25);
	
	switch(rot_degree) {
	case ROT_90:
		*org_x = ((dst_x1 - dst_y1 + src_x1 + src_y2)>>1);
		*org_y = ((dst_x1 + dst_y1 - src_x1 + src_y2)>>1);
		break;
	case ROT_180:
	case ROT_X_FLIP:
	case ROT_Y_FLIP:
		*org_x = ((dst_x1 + src_x2)>>1);
		*org_y = ((dst_y1 + src_y2)>>1);
		break;
	case ROT_270:
		*org_x = ((dst_x1 + dst_y1 + src_x2 - src_y1)>>1);
		*org_y = ((dst_y1 - dst_x1 + src_x2 + src_y1)>>1);
		break;
	case ROT_0:
	default:
		break;
	}
}
static void g2d_rotator_start(G2D_PARAMS *params, G2D_ROT rot_degree)
{
	g2d_init_regs(params);
	g2d_rotate_start(params, rot_degree);
}

irqreturn_t g2d_irq(int irq, void *dev_id)
{
 	if(__raw_readl(g2d_base + G2D_INTC_PEND_REG) & G2D_PEND_REG_INTP_CMD_FIN) {
		__raw_writel(G2D_PEND_REG_INTP_CMD_FIN, g2d_base + G2D_INTC_PEND_REG);
		wake_up_interruptible(&waitq_g2d);
		DPRINTK("\nafter wake_up_interruptible\n");
		g2d_poll_flag = 1;	
	}
	
	return IRQ_HANDLED;
}

int g2d_open(struct inode *inode, struct file *file)
{
	G2D_PARAMS *params;
	params = (G2D_PARAMS *)kmalloc(sizeof(G2D_PARAMS), GFP_KERNEL);
	if(params == NULL) {
		DPRINTK("\n%d : Instance memory allocation is failed\n", __LINE__);
		return -1;
	}

	memset(params, 0, sizeof(G2D_PARAMS));

	file->private_data = (G2D_PARAMS *)params;
 	
	return 0;
}
int g2d_release(struct inode *inode, struct file *file)
{
	G2D_PARAMS *params;
	params = (G2D_PARAMS *)file->private_data;
	if(params == NULL) {
		DPRINTK("%d : Can't release g2d \n", __LINE__);
		return -1;
	}

	kfree(params);
	
	return 0;
}
int g2d_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long pageFrameNo = 0;
	unsigned long size;

	size = vma->vm_end - vma->vm_start;

	pageFrameNo = __phys_to_pfn(g2d_mem->start);

	if(size > G2D_SFR_SIZE) {
		DPRINTK("%d : The size of G2D_SFR_SIZE mapping is too big!\n", __LINE__);
		return -EINVAL;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		DPRINTK("%d : Writable G2D_SFR_SIZE mapping must be shared \n", __LINE__);
		return -EINVAL;
	}

	/* remap kernel memory to userspace */
	if(remap_pfn_range(vma, vma->vm_start, pageFrameNo, size, vma->vm_page_prot))
		return -EINVAL;
	
	return 0;
}
static int g2d_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	G2D_PARAMS *params;
	G2D_ROT rot_degree;

	params = (G2D_PARAMS *)file->private_data;
	if(copy_from_user(params, (G2D_PARAMS*) arg, sizeof(G2D_PARAMS)))
		return -EFAULT;

	//mutex_lock(g2d_mutex);

	switch(cmd) {
	case G2D_ROTATOR_0:
		rot_degree = ROT_0;
		g2d_rotator_start(params, rot_degree);
		break;
	case G2D_ROTATOR_90:
		rot_degree = ROT_90;
		g2d_rotator_start(params, rot_degree);
		break;
	case G2D_ROTATOR_180:
		rot_degree = ROT_180;
		g2d_rotator_start(params, rot_degree);
		break;
	case G2D_ROTATOR_270:
		rot_degree = ROT_270;
		g2d_rotator_start(params, rot_degree);
		break;
	case G2D_ROTATOR_X_FLIP:
		rot_degree = ROT_X_FLIP;
		g2d_rotator_start(params, rot_degree);
		break;
	case G2D_ROTATOR_Y_FLIP:
		rot_degree = ROT_Y_FLIP;
		g2d_rotator_start(params, rot_degree);
		break;
	default:
		mutex_unlock(g2d_mutex);
		return -EINVAL;
	}
	

	if(!(file->f_flags & O_NONBLOCK)) {
		if(interruptible_sleep_on_timeout(&waitq_g2d, G2D_TIMEOUT) == 0) {
			DPRINTK("%d : Waiting for interrupt is timeout\n", __LINE__);
		}
	} else {
		return -EAGAIN;
	}

	//mutex_unlock(g2d_mutex);
	
	return 0;
}
static unsigned int g2d_poll(struct file *file, struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &waitq_g2d, wait);
	if(g2d_poll_flag == 1) {
		mask = POLLOUT|POLLWRNORM;
		g2d_poll_flag = 0;
	}
	
	return mask;
}
struct file_operations g2d_fops = {
	.owner		= THIS_MODULE,
	.open		= g2d_open,
	.release	= g2d_release,
	.mmap		= g2d_mmap,
	.ioctl		= g2d_ioctl,
	.poll		= g2d_poll,
};

static struct miscdevice g2d_dev = {
	.minor		= G2D_MINOR, 
	.name		= "g2d",
	.fops		= &g2d_fops,
};


int g2d_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;

	/* find the IRQs */
	g2d_irq_num = platform_get_irq(pdev, 0);
	DPRINTK("%d : g2d_irq_num %d\n", __LINE__, g2d_irq_num);
	
	if(g2d_irq_num <= 0) {
		DPRINTK("%d : failed to get irq resource\n", __LINE__);
		return -ENOENT;
	}
	/* request_irq(irq_number, irq handler, irq_flags, dev_name, dev_id) */
	ret = request_irq(g2d_irq_num, g2d_irq, IRQF_DISABLED, pdev->name, NULL);
	if(ret) {
		DPRINTK("%d : request_irq(g2d) failed\n", __LINE__);
		return ret;
	}

	
	/* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL) {
		DPRINTK("%d : failed to get memory region resource\n", __LINE__);
		return -ENOENT;		//ENOENT : No such file or directory
	}
	
	g2d_mem = request_mem_region(res->start, ((res->end)-(res->start))+1, pdev->name);
	if(g2d_mem == NULL) {
		DPRINTK("%d : failed to reserve memory region\n", __LINE__);
		return -ENOENT;
	}

	
	g2d_base = ioremap(g2d_mem->start, g2d_mem->end - res->start + 1);
	if(g2d_base == NULL) {
		DPRINTK("%d : failed ioremap\n", __LINE__);
		return -ENOENT;
	}
	
	/* clock */
	g2d_clock = clk_get(&pdev->dev, "g2d");
	if(g2d_clock == NULL) {
		DPRINTK("%d : failed to find g2d clock source\n", __LINE__);
		return -ENOENT;
	}

	clk_enable(g2d_clock);

	h_clk = clk_get(&pdev->dev, "hclk");
	if(h_clk == NULL) {
		DPRINTK("%d : failed to find h_clk clock source\n", __LINE__);
		return -ENOENT;
	}

	init_waitqueue_head(&waitq_g2d);

	ret = misc_register(&g2d_dev);
	if(ret) {
		DPRINTK("%d : cannot register miscdev on minor=%d (%d)\n", __LINE__, G2D_MINOR, ret);
		return ret;
	}

	/* mutex */
	g2d_mutex = (struct mutex *)kmalloc(sizeof(struct mutex), GFP_KERNEL);
	if(g2d_mutex == NULL) {
		return -1;
	}

	mutex_init(g2d_mutex);
	
	return 0;
}
static int g2d_remove(struct platform_device *pdev)
{

	free_irq(g2d_irq_num, NULL);

	if(g2d_mem != NULL) {
		DPRINTK("%d g2d Driver, releasing resource\n", __LINE__);
		iounmap(g2d_base);
		release_resource(g2d_mem);
		kfree(g2d_mem);
	}

	misc_deregister(&g2d_dev);

	return 0;
}
static int g2d_suspend(struct platform_device *pdev, pm_message_t state)
{
	clk_disable(g2d_clock);
	
	return 0;
}
static int g2d_resume(struct platform_device *pdev)
{
	clk_enable(g2d_clock);
	
	return 0;
}
static struct platform_driver g2d_driver = {
	.probe		= g2d_probe,
	.remove		= g2d_remove,
	.suspend	= g2d_suspend,
	.resume		= g2d_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "g2d",
	},
};

int __init g2d_init(void)
{
	if(platform_driver_register(&g2d_driver) != 0 ){
		DPRINTK("%d : platform device register failed \n", __LINE__);
		return -1;
	}

	return 0;
}
void __exit g2d_exit(void)
{
	platform_driver_unregister(&g2d_driver);
}


module_init(g2d_init);
module_exit(g2d_exit);

MODULE_AUTHOR("Jonghun Han <jonghun.han@samsung.com>");
MODULE_DESCRIPTION("S5PC1XX FIMG-2D Device Driver");
MODULE_LICENSE("GPL");
