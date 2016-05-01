/*
 * Example driver for Linux
 * Copyright (C) 2014, Manuel Traut <manut@mecka.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/interrupt.h>

#define DRV_NAME "hswgt-pci"

struct hswgt_mem {
	u8 status;
	u8 mem[1024*1024-1];
} __attribute__((packed));

struct hswgt_dev {
	struct pci_dev *pdev;
	struct hswgt_mem *hswgtmem;
};

static const struct pci_device_id hswgt_id_table[] = {
	{ 0x1af4, 0x1110, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, 0 },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, hswgt_id_table);

static irqreturn_t hswgt_intr(int irq, void *dev_id)
{
	struct hswgt_dev *hswgt = dev_id;

	u8 status = hswgt->hswgtmem->status;

	pr_err("%s: %d\n", __func__, status);

	return IRQ_HANDLED; /* IRQ_NONE */
}

static int hswgt_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int err = 0;
	struct hswgt_dev *hswgt;

	pr_err("%s\n", __func__);

	hswgt = kmalloc(sizeof(struct hswgt_dev), GFP_KERNEL);
	if (!hswgt) {
		pr_err("cannot allocate memory for hswgt device, aborting\n");
		return -ENOMEM;
	}

	hswgt->pdev = pdev;
	pci_set_drvdata(pdev, hswgt);

	if ((err = pci_enable_device(pdev))) {
		pr_err("Cannot enable PCI device, aborting\n");
		goto err_out_free_dev;
	}

	if (!(pci_resource_flags(pdev, 0) & IORESOURCE_MEM)) {
		pr_err("Cannot find proper PCI device base address, aborting\n");
		err = -ENODEV;
		goto err_out_disable_pdev;
	}

	if ((err = pci_request_regions(pdev, DRV_NAME))) {
		pr_err("Cannot obtain PCI resources, aborting\n");
		goto err_out_disable_pdev;
	}

	pci_set_master(pdev);

	if ((err = pci_set_dma_mask(pdev, DMA_BIT_MASK(32)))) {
		pr_err("no usable dma configuration, aborting\n");
		goto err_out_free_res;
	}

	hswgt->hswgtmem = pci_iomap(pdev, 2, sizeof(struct hswgt_mem));
	if (!hswgt->hswgtmem) {
		pr_err("Cannot map device registers, aborting\n");
		err = -ENOMEM;
		goto err_out_free_res;
	}

	if ((err = request_irq(pdev->irq, hswgt_intr, IRQF_SHARED, DRV_NAME, hswgt))) {
		pr_err("cannot request irq, aborting\n");
		goto err_no_irq;
	}

	pr_info("%s rdy: %p\n", __func__, hswgt);

	return err;

err_no_irq:
	pci_iounmap(pdev, hswgt->hswgtmem);
err_out_free_res:
	pci_release_regions(pdev);
err_out_disable_pdev:
	pci_disable_device(pdev);
err_out_free_dev:
	kfree(hswgt);

	return err;
}

static void hswgt_remove(struct pci_dev *pdev)
{
	struct hswgt_dev *hswgt = pci_get_drvdata(pdev);

	free_irq(pdev->irq, hswgt);
	pci_iounmap(pdev, hswgt->hswgtmem);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
	kfree(hswgt);
}

static struct pci_driver hswgt_driver = {
	.name =         DRV_NAME,
	.id_table =     hswgt_id_table,
	.probe =        hswgt_probe,
	.remove =       hswgt_remove,
};

static int __init hswgt_init(void)
{
	pr_err("%s\n", __func__);
	return pci_register_driver(&hswgt_driver);;
}

static void __exit hswgt_exit(void)
{
	pr_err("%s\n", __func__);
	pci_unregister_driver(&hswgt_driver);
}

module_init(hswgt_init);
module_exit(hswgt_exit);
