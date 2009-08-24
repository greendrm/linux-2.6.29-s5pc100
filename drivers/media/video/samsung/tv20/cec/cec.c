//-------------------------------------------------------------------
// Copyright SAMSUNG Electronics Co., Ltd
// All right reserved.
//
// This software is the confidential and proprietary information
// of Samsung Electronics, Inc. ("Confidential Information").  You
// shall not disclose such Confidential Information and shall use
// it only in accordance with the terms of the license agreement
// you entered into with Samsung Electronics.
//-------------------------------------------------------------------
/**
 * @file  cec.c
 * @brief This file contains an implementation of CEC device driver.
 *
 * @author  Digital IP Development Team (mrkim@samsung.com) \n
 *          SystemLSI, Samsung Electronics
 * @version 1.0
 *
 * @remarks 09-30-2008 1.0 Initial version
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/poll.h>

#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>

#include "../hdmi/regs-hdmi.h"

#include "cec.h"
#include "regs-cec.h"


#define CEC_DEBUG 0

#if CEC_DEBUG
#define DPRINTK(args...)    printk(args)
#else
#define DPRINTK(args...)
#endif


#define VERSION   "1.0" /* Driver version number */
#define CEC_MINOR 242 /* Major 10, Minor 242, /dev/cec */

#define CEC_MESSAGE_BROADCAST_MASK    0x0F
#define CEC_MESSAGE_BROADCAST         0x0F

#define CEC_FILTER_THRESHOLD          0x15

/**
 * @enum cec_state
 * Defines all possible states of CEC software state machine
 */
enum cec_state {
    STATE_RX,
    STATE_TX,
    STATE_DONE,
    STATE_ERROR
};

/**
 * @struct cec_rx_struct
 * Holds CEC Rx state and data
 */
struct cec_rx_struct {
    spinlock_t lock;
    wait_queue_head_t waitq;
    atomic_t state;
    u8 *buffer;
    unsigned int size;
};

/**
 * @struct cec_tx_struct
 * Holds CEC Tx state and data
 */
struct cec_tx_struct {
    wait_queue_head_t waitq;
    atomic_t state;
};

static struct cec_rx_struct cec_rx_struct;
static struct cec_tx_struct cec_tx_struct;

static int cec_open(struct inode *inode, struct file *file);
static int cec_release(struct inode *inode, struct file *file);
static ssize_t cec_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
static ssize_t cec_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos);
static int cec_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static unsigned int cec_poll(struct file *file, poll_table *wait);

static irqreturn_t cec_irq_handler(int irq, void *dev_id);

inline static void cec_set_divider(void);
inline static void cec_enable_interrupts(void);
inline static void cec_disable_interrupts(void);
inline static void cec_enable_rx(void);
inline static void cec_mask_rx_interrupts(void);
inline static void cec_unmask_rx_interrupts(void);
inline static void cec_mask_tx_interrupts(void);
inline static void cec_unmask_tx_interrupts(void);
inline static void cec_set_rx_state(enum cec_state state);
inline static void cec_set_tx_state(enum cec_state state);

static const struct file_operations cec_fops =
{
    .owner   = THIS_MODULE,
    .open    = cec_open,
    .release = cec_release,
    .read    = cec_read,
    .write   = cec_write,
    .ioctl   = cec_ioctl,
    .poll    = cec_poll,
};

static struct miscdevice cec_misc_device =
{
    .minor = CEC_MINOR,
    .name  = "CEC",
    .fops  = &cec_fops,
};


int cec_open(struct inode *inode, struct file *file)
{
    DPRINTK(KERN_INFO "%s\n", __FUNCTION__);

    writeb(CEC_RX_CTRL_RESET, CEC_RX_CTRL); // reset CEC Rx
    writeb(CEC_TX_CTRL_RESET, CEC_TX_CTRL); // reset CEC Tx

    cec_set_divider();

    writeb(CEC_FILTER_THRESHOLD, CEC_RX_FILTER_TH); // setup filter

    cec_enable_interrupts();

    cec_unmask_tx_interrupts();

    cec_set_rx_state(STATE_RX);
    cec_unmask_rx_interrupts();
    cec_enable_rx();

    return 0;
}

int cec_release(struct inode *inode, struct file *file)
{
    DPRINTK(KERN_INFO "%s\n", __FUNCTION__);

    cec_mask_tx_interrupts();
    cec_mask_rx_interrupts();
    cec_disable_interrupts();

    return 0;
}

ssize_t cec_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
    ssize_t retval;

    DPRINTK(KERN_INFO "%s\n", __FUNCTION__);

    if (wait_event_interruptible(cec_rx_struct.waitq, atomic_read(&cec_rx_struct.state) == STATE_DONE)) {
        return -ERESTARTSYS;
    }

    spin_lock_irq(&cec_rx_struct.lock);

    if (cec_rx_struct.size > count) {
        spin_unlock_irq(&cec_rx_struct.lock);
        return -1;
    }

    if (copy_to_user(buffer, cec_rx_struct.buffer, cec_rx_struct.size)) {
        spin_unlock_irq(&cec_rx_struct.lock);
        printk(KERN_ERR "CEC: copy_to_user() failed!\n");
        return -EFAULT;
    }

    retval = cec_rx_struct.size;
    cec_set_rx_state(STATE_RX);
    spin_unlock_irq(&cec_rx_struct.lock);

    return retval;
}

ssize_t cec_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    char *data;
    unsigned char reg;
    int i = 0;

    DPRINTK(KERN_INFO "%s\n", __FUNCTION__);

    /* check data size */
    if (count > CEC_TX_BUFF_SIZE || count == 0)
        return -1;

    data = kmalloc(count, GFP_KERNEL);
    if (!data)
    {
        printk(KERN_ERR "CEC: kmalloc() failed!\n");
        return -1;
    }

    if (copy_from_user(data, buffer, count))
    {
        printk(KERN_ERR "CEC: copy_from_user() failed!\n");
        kfree(data);
        return -EFAULT;
    }

    /* copy packet to hardware buffer */
    while (i < count) {
        writeb(data[i], CEC_TX_BUFF0 + (i*4));
        i++;
    }

    /* set number of bytes to transfer */
    writeb(count, CEC_TX_BYTES);

    cec_set_tx_state(STATE_TX);

    /* start transfer */
    reg = readb(CEC_TX_CTRL);
    reg |= CEC_TX_CTRL_START;

    /* if message is broadcast message - set corresponding bit */
    if ((data[0] & CEC_MESSAGE_BROADCAST_MASK) == CEC_MESSAGE_BROADCAST)
        reg |= CEC_TX_CTRL_BCAST;
    else
        reg &= ~CEC_TX_CTRL_BCAST;

    /* set number of retransmissions */
    reg |= 0x50;

    writeb(reg, CEC_TX_CTRL);

    kfree(data);

    /* wait for interrupt */
    if (wait_event_interruptible(cec_tx_struct.waitq, atomic_read(&cec_tx_struct.state) != STATE_TX)) {
        return -ERESTARTSYS;
    }

    if (atomic_read(&cec_tx_struct.state) == STATE_ERROR) {
        return -1;
    }

    return count;
}

int cec_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int laddr;

    DPRINTK(KERN_INFO "%s\n", __FUNCTION__);

    switch (cmd) {
        case CEC_IOC_SETLADDR:
            DPRINTK(KERN_INFO "CEC: ioctl(CEC_IOC_SETLADDR)\n");
            if (get_user(laddr, (unsigned int __user *) arg))
                return -EFAULT;
            DPRINTK(KERN_INFO "CEC: logical address = 0x%02x\n", laddr);
            writeb(laddr & 0x0F, CEC_LOGIC_ADDR);
            break;
        default:
            return -EINVAL;
    }

    return 0;
}

unsigned int cec_poll(struct file *file, poll_table *wait)
{
//    DPRINTK(KERN_INFO "%s\n", __FUNCTION__);

    poll_wait(file, &cec_rx_struct.waitq, wait);

    if (atomic_read(&cec_rx_struct.state) == STATE_DONE)
        return POLLIN | POLLRDNORM;

    return 0;
}

/**
 * @brief CEC interrupt handler
 *
 * Handles interrupt requests from CEC hardware. \n
 * Action depends on current state of CEC hardware.
 */
irqreturn_t cec_irq_handler(int irq, void *dev_id)
{
    u8 flag;
    u32 status;

    /* read flag register */
    flag = readb(HDMI_SS_INTC_FLAG);

    /* is this our interrupt? */
    if (!(flag & (1<<HDMI_IRQ_CEC))) {
        return IRQ_NONE;
    }

    DPRINTK(KERN_INFO "%s\n", __FUNCTION__);

    status = readb(CEC_STATUS_0);
    status |= readb(CEC_STATUS_1) << 8;
    status |= readb(CEC_STATUS_2) << 16;
    status |= readb(CEC_STATUS_3) << 24;

    DPRINTK(KERN_INFO "CEC: status = 0x%x!\n", status);

    if (status & CEC_STATUS_TX_DONE) {
        if (status & CEC_STATUS_TX_ERROR) {
            DPRINTK(KERN_INFO "CEC: CEC_STATUS_TX_ERROR!\n");
            cec_set_tx_state(STATE_ERROR);
        } else {
            DPRINTK(KERN_INFO "CEC: CEC_STATUS_TX_DONE!\n");
            cec_set_tx_state(STATE_DONE);
        }
        /* clear interrupt pending bit */
        writeb(CEC_IRQ_TX_DONE | CEC_IRQ_TX_ERROR, CEC_IRQ_CLEAR);
        wake_up_interruptible(&cec_tx_struct.waitq);
    }

    if (status & CEC_STATUS_RX_DONE) {
        if (status & CEC_STATUS_RX_ERROR) {
            DPRINTK(KERN_INFO "CEC: CEC_STATUS_RX_ERROR!\n");
            writeb(CEC_RX_CTRL_RESET, CEC_RX_CTRL); // reset CEC Rx
        } else {
            unsigned int size, i = 0;

            DPRINTK(KERN_INFO "CEC: CEC_STATUS_RX_DONE!\n");

            /* copy data from internal buffer */
            size = status >> 24;

            spin_lock(&cec_rx_struct.lock);

            while (i < size) {
                cec_rx_struct.buffer[i] = readb(CEC_RX_BUFF0 + (i*4));
                i++;
            }
            cec_rx_struct.size = size;
            cec_set_rx_state(STATE_DONE);

            spin_unlock(&cec_rx_struct.lock);

            cec_enable_rx();
        }
        /* clear interrupt pending bit */
        writeb(CEC_IRQ_RX_DONE | CEC_IRQ_RX_ERROR, CEC_IRQ_CLEAR);
        wake_up_interruptible(&cec_rx_struct.waitq);
    }

    return IRQ_HANDLED;
}

static int __init cec_init(void)
{
    u8 *buffer;

    DPRINTK(KERN_INFO "%s\n", __FUNCTION__);

    if (!machine_is_hdmidp())
        return -ENODEV;

    printk(KERN_INFO "CEC Driver ver. %s (built %s %s)\n", VERSION, __DATE__, __TIME__);

    if (misc_register(&cec_misc_device))
    {
        printk(KERN_WARNING "CEC: Couldn't register device 10, %d.\n", CEC_MINOR);
        return -EBUSY;
    }

    cec_disable_interrupts();

    if (request_irq(IRQ_HDMI, cec_irq_handler, IRQF_SHARED, "cec", cec_irq_handler))
    {
        printk(KERN_WARNING "CEC: IRQ %d is not free.\n", IRQ_HDMI);
        misc_deregister(&cec_misc_device);
        return -EIO;
    }

    init_waitqueue_head(&cec_rx_struct.waitq);
    spin_lock_init(&cec_rx_struct.lock);
    init_waitqueue_head(&cec_tx_struct.waitq);

    buffer = kmalloc(CEC_TX_BUFF_SIZE, GFP_KERNEL);
    if (!buffer)
    {
        printk(KERN_ERR "CEC: kmalloc() failed!\n");
        misc_deregister(&cec_misc_device);
        return -EIO;
    }

    cec_rx_struct.buffer = buffer;
    cec_rx_struct.size   = 0;

    return 0;
}

static void __exit cec_exit(void)
{
    DPRINTK(KERN_INFO "%s\n", __FUNCTION__);

    free_irq(IRQ_HDMI, cec_irq_handler);
    misc_deregister(&cec_misc_device);

    kfree(cec_rx_struct.buffer);
}

/**
 * Set CEC divider value.
 */
void cec_set_divider(void)
{
    /**
     * (CEC_DIVISOR) * (clock cycle time) = 0.05ms \n
     * for 27MHz clock divisor is 0x0545
     */
    writeb(0x05, CEC_DIVISOR_1);
    writeb(0x45, CEC_DIVISOR_0);
}

/**
 * Enable CEC interrupts
 */
void cec_enable_interrupts(void)
{
    unsigned char reg;
    reg = readb(HDMI_SS_INTC_CON);
    writeb(reg | (1<<HDMI_IRQ_CEC) | (1<<HDMI_IRQ_GLOBAL), HDMI_SS_INTC_CON);
}

/**
 * Disable CEC interrupts
 */
void cec_disable_interrupts(void)
{
    unsigned char reg;
    reg = readb(HDMI_SS_INTC_CON);
    writeb(reg & ~(1<<HDMI_IRQ_CEC), HDMI_SS_INTC_CON);
}

/**
 * Enable CEC Rx engine
 */
void cec_enable_rx(void)
{
    unsigned char reg;
    reg = readb(CEC_RX_CTRL);
    reg |= CEC_RX_CTRL_ENABLE;
    writeb(reg, CEC_RX_CTRL);
}

/**
 * Mask CEC Rx interrupts
 */
void cec_mask_rx_interrupts(void)
{
    unsigned char reg;
    reg = readb(CEC_IRQ_MASK);
    reg |= CEC_IRQ_RX_DONE;
    reg |= CEC_IRQ_RX_ERROR;
    writeb(reg, CEC_IRQ_MASK);
}

/**
 * Unmask CEC Rx interrupts
 */
void cec_unmask_rx_interrupts(void)
{
    unsigned char reg;
    reg = readb(CEC_IRQ_MASK);
    reg &= ~CEC_IRQ_RX_DONE;
    reg &= ~CEC_IRQ_RX_ERROR;
    writeb(reg, CEC_IRQ_MASK);
}

/**
 * Mask CEC Tx interrupts
 */
void cec_mask_tx_interrupts(void)
{
    unsigned char reg;
    reg = readb(CEC_IRQ_MASK);
    reg |= CEC_IRQ_TX_DONE;
    reg |= CEC_IRQ_TX_ERROR;
    writeb(reg, CEC_IRQ_MASK);
}

/**
 * Unmask CEC Tx interrupts
 */
void cec_unmask_tx_interrupts(void)
{
    unsigned char reg;
    reg = readb(CEC_IRQ_MASK);
    reg &= ~CEC_IRQ_TX_DONE;
    reg &= ~CEC_IRQ_TX_ERROR;
    writeb(reg, CEC_IRQ_MASK);
}

/**
 * Change CEC Tx state to state
 * @param state [in] new CEC Tx state.
 */
void cec_set_tx_state(enum cec_state state)
{
    atomic_set(&cec_tx_struct.state, state);
}

/**
 * Change CEC Rx state to @c state.
 * @param state [in] new CEC Rx state.
 */
void cec_set_rx_state(enum cec_state state)
{
    atomic_set(&cec_rx_struct.state, state);
}


MODULE_AUTHOR("Samsung HSI");

module_init(cec_init);
module_exit(cec_exit);
