/*
 * OHCI HCD (Host Controller Driver) for USB.
 *
 * Bus Glue for SAMSUNG S5PC110
 *
 * Based on "ohci-au1xxx.c" by Matt Porter <mporter@kernel.crashing.org>
 *
 * Modified for AMD Alchemy Au1200 EHC
 *  by K.Boge <karsten.boge@amd.com>
 *
 * Modified for SAMSUNG s5pc110 EHCI
 *  by Jin-goo Han <jg1.han@samsung.com>
 *
 * Modified for SAMSUNG s5pc110 OHCI
 *  by Jin-goo Han <jg1.han@samsung.com>
 *
 * This file is licenced under the GPL.
 */

#include <linux/clk.h>
#include <linux/platform_device.h>

static struct clk *usb_clk;

extern int usb_disabled(void);

extern void usb_host_clk_en(void);

static void s5pc110_start_ohc(void);
static void s5pc110_stop_ohc(void);
static int ohci_hcd_s5pc110_drv_probe(struct platform_device *pdev);
static int ohci_hcd_s5pc110_drv_remove(struct platform_device *pdev);

#ifdef CONFIG_PM
static int ohci_hcd_s5pc110_drv_suspend(struct platform_device *pdev)
static int ohci_hcd_s5pc110_drv_resume(struct platform_device *pdev)
#else
#define ohci_hcd_s5pc110_drv_suspend NULL
#define ohci_hcd_s5pc110_drv_resume NULL
#endif


static void s5pc110_start_ohc(void)
{
	clk_enable(usb_clk);
	usb_host_clk_en();
}

static void s5pc110_stop_ohc(void)
{
	clk_disable(usb_clk);
	clk_put(usb_clk);
}

static int __devinit ohci_s5pc110_start(struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	int ret;

	ohci_dbg(ohci, "ohci_s5pc110_start, ohci:%p", ohci);

	if ((ret = ohci_init(ohci)) < 0)
		return ret;

	if ((ret = ohci_run(ohci)) < 0) {
		err ("can't start %s", hcd->self.bus_name);
		ohci_stop(hcd);
		return ret;
	}

	return 0;
}

static const struct hc_driver ohci_s5pc110_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "s5pc110 OHCI",
	.hcd_priv_size		= sizeof(struct ohci_hcd),

	.irq			= ohci_irq,
	.flags			= HCD_MEMORY|HCD_USB11,

	.start			= ohci_s5pc110_start,
	.stop			= ohci_stop,
	.shutdown		= ohci_shutdown,

	.get_frame_number	= ohci_get_frame,

	.urb_enqueue		= ohci_urb_enqueue,
	.urb_dequeue		= ohci_urb_dequeue,
	.endpoint_disable	= ohci_endpoint_disable,

	.hub_status_data	= ohci_hub_status_data,
	.hub_control		= ohci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend		= ohci_bus_suspend,
	.bus_resume		= ohci_bus_resume,
#endif
	.start_port_reset	= ohci_start_port_reset,
};

static int ohci_hcd_s5pc110_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd  *hcd = NULL;
	int retval = 0;

	if (usb_disabled()) {
		return -ENODEV;
	}

	if (pdev->resource[1].flags != IORESOURCE_IRQ) {
		dev_err(&pdev->dev,"resource[1] is not IORESOURCE_IRQ.\n");
		return -ENODEV;
	}

	hcd = usb_create_hcd(&ohci_s5pc110_hc_driver, &pdev->dev, "s5pc110");
	if (!hcd) {
		dev_err(&pdev->dev,"usb_create_hcd failed!\n");
		return -ENODEV;
	}

	hcd->rsrc_start = pdev->resource[0].start;
	hcd->rsrc_len = pdev->resource[0].end - pdev->resource[0].start + 1;

	if(!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		dev_err(&pdev->dev,"request_mem_region failed!\n");
		retval = -EBUSY;
		goto err1;
	}

	usb_clk = clk_get(&pdev->dev, "usb-host");
	if (IS_ERR(usb_clk)) {
		dev_err(&pdev->dev, "cannot get usb-host clock\n");
		retval = -ENODEV;
		goto err2;
	}

	s5pc110_start_ohc(); 

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&pdev->dev,"ioremap failed!\n");
		retval = -ENOMEM;
		goto err2;
	}

	ohci_hcd_init(hcd_to_ohci(hcd));

	retval = usb_add_hcd(hcd, pdev->resource[1].start,
				IRQF_DISABLED | IRQF_SHARED);

	if (retval == 0) {
		platform_set_drvdata(pdev, hcd);
		return retval;
	}

	s5pc110_stop_ohc();
	iounmap(hcd->regs);
err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err1:
	usb_put_hcd(hcd);
	return retval;
}

static int ohci_hcd_s5pc110_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	s5pc110_stop_ohc();
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver  ohci_hcd_s5pc110_driver = {
	.probe		= ohci_hcd_s5pc110_drv_probe,
	.remove		= ohci_hcd_s5pc110_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.suspend	= ohci_hcd_s5pc110_drv_suspend,
	.resume		= ohci_hcd_s5pc110_drv_resume,
	.driver = {
		.name = "s5pc110-ohci",
		.owner = THIS_MODULE,
	}
};

MODULE_ALIAS("platform:s5pc110-ohci");
