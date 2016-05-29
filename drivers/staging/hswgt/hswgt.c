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
#include <linux/leds.h>

#define DRV_NAME "hswgt-pci"
#define NUM_LEDS 3

struct hswgt_mem {
	u8 led[NUM_LEDS];
	u8 mem[1024*1024-NUM_LEDS];
} __attribute__((packed));

struct hswgt_dev {
	struct pci_dev *pdev;
	struct hswgt_mem *hswgtmem;
	struct led_classdev led_heartbeat;
	struct led_classdev led_net;
	struct led_classdev led_disk;
};

static const struct pci_device_id hswgt_id_table[] = {
	{ 0x1af4, 0x1110, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, 0 },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, hswgt_id_table);

static void hswgt_led_heartbeat_set(struct led_classdev *led_cdev,
			   enum led_brightness brightness)
{
	struct hswgt_dev *hswgt;
	hswgt = container_of(led_cdev, struct hswgt_dev, led_heartbeat);
	hswgt->hswgtmem->led[0] = brightness;
}

static enum led_brightness hswgt_led_heartbeat_get(struct led_classdev *led_cdev)
{
	struct hswgt_dev *hswgt;
	hswgt = container_of(led_cdev, struct hswgt_dev, led_heartbeat);
	return hswgt->hswgtmem->led[0];
}


static void hswgt_led_net_set(struct led_classdev *led_cdev,
			   enum led_brightness brightness)
{
	struct hswgt_dev *hswgt = container_of(led_cdev, struct hswgt_dev,
					       led_net);
	hswgt->hswgtmem->led[1] = brightness;
}

static void hswgt_led_disk_set(struct led_classdev *led_cdev,
			   enum led_brightness brightness)
{
	struct hswgt_dev *hswgt = container_of(led_cdev, struct hswgt_dev,
					       led_disk);
	hswgt->hswgtmem->led[2] = brightness;
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

	hswgt->led_heartbeat.name = "heartbeat";
	hswgt->led_heartbeat.brightness = LED_OFF;
	hswgt->led_heartbeat.max_brightness = 1;
	hswgt->led_heartbeat.brightness_set = hswgt_led_heartbeat_set;
	hswgt->led_heartbeat.brightness_get = hswgt_led_heartbeat_get;
	devm_led_classdev_register(&pdev->dev, &hswgt->led_heartbeat);

	hswgt->led_net.name = "net";
	hswgt->led_net.brightness = LED_OFF;
	hswgt->led_net.max_brightness = 1;
	hswgt->led_net.brightness_set = hswgt_led_net_set;
	devm_led_classdev_register(&pdev->dev, &hswgt->led_net);

	hswgt->led_disk.name = "disk";
	hswgt->led_disk.brightness = LED_OFF;
	hswgt->led_disk.max_brightness = 1;
	hswgt->led_disk.brightness_set = hswgt_led_disk_set;
	devm_led_classdev_register(&pdev->dev, &hswgt->led_disk);

	hswgt->hswgtmem->led[0] = 1;

	pr_info("%s rdy: %p\n", __func__, hswgt);

	return err;

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
