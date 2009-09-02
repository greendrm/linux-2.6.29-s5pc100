/*
 * EHCI HCD (Host Controller Driver) for USB.
 *
 * Bus Glue for SAMSUNG S5PC110
 *
 * Based on "ohci-au1xxx.c" by Matt Porter <mporter@kernel.crashing.org>
 *
 * Modified for AMD Alchemy Au1200 EHC
 *  by K.Boge <karsten.boge@amd.com>
 *
 * Modified for SAMSUNG S5pc110 EHCI
 *  by Jin-goo Han <jg1.han@samsung.com>
 *
 * This file is licenced under the GPL.
 */

#include <linux/clk.h>
#include <linux/platform_device.h>

static struct clk *usb_clk;

extern int usb_disabled(void);

extern void usb_host_clk_en(void);

static void s5pc110_start_ehc(void);
static void s5pc110_stop_ehc(void);
static int ehci_hcd_s5pc110_drv_probe(struct platform_device *pdev);
static int ehci_hcd_s5pc110_drv_remove(struct platform_device *pdev);

#ifdef CONFIG_PM
static int ehci_hcd_s5pc110_drv_suspend(struct platform_device *pdev)
static int ehci_hcd_s5pc110_drv_resume(struct platform_device *pdev)
#else
#define ehci_hcd_s5pc110_drv_suspend NULL
#define ehci_hcd_s5pc110_drv_resume NULL
#endif

static void s5pc110_start_ehc(void)
{
	clk_enable(usb_clk);
	usb_host_clk_en();
}

static void s5pc110_stop_ehc(void)
{
	clk_disable(usb_clk);
	clk_put(usb_clk);
}

static const struct hc_driver ehci_s5pc110_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "s5pc110 EHCI",
	.hcd_priv_size		= sizeof(struct ehci_hcd),

	.irq			= ehci_irq,
	.flags			= HCD_MEMORY|HCD_USB2,

	.reset			= ehci_init,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	.get_frame_number	= ehci_get_frame,

	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,

	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,
};

static int ehci_hcd_s5pc110_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd  *hcd = NULL;
	struct ehci_hcd *ehci = NULL;
	int retval = 0;

	if (usb_disabled()) {
		return -ENODEV;
	}

	if (pdev->resource[1].flags != IORESOURCE_IRQ) {
		dev_err(&pdev->dev,"resource[1] is not IORESOURCE_IRQ.\n");
		return -ENODEV;
	}

	hcd = usb_create_hcd(&ehci_s5pc110_hc_driver, &pdev->dev, "s5pc110");
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

	s5pc110_start_ehc(); 

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&pdev->dev,"ioremap failed!\n");
		retval = -ENOMEM;
		goto err2;
	}

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(readl(&ehci->caps->hc_capbase));
	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	writel(0x00600040, hcd->regs + 0x94);

	retval = usb_add_hcd(hcd, pdev->resource[1].start,
				IRQF_DISABLED | IRQF_SHARED);

	if (retval == 0) {
		platform_set_drvdata(pdev, hcd);
		return retval;
	}

	s5pc110_stop_ehc();
	iounmap(hcd->regs);
err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err1:
	usb_put_hcd(hcd);
	return retval;
}

static int ehci_hcd_s5pc110_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	s5pc110_stop_ehc();
	platform_set_drvdata(pdev, NULL);
	
	return 0;
}

static struct platform_driver  ehci_hcd_s5pc110_driver = {
	.probe		= ehci_hcd_s5pc110_drv_probe,
	.remove		= ehci_hcd_s5pc110_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.suspend	= ehci_hcd_s5pc110_drv_suspend,
	.resume		= ehci_hcd_s5pc110_drv_resume,
	.driver = {
		.name = "s5pc110-ehci",
		.owner = THIS_MODULE,
	}
};

MODULE_ALIAS("platform:s5pc110-ehci");
