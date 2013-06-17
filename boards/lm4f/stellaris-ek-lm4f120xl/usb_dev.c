/*
 * This file is part of the vultureprog project.
 *
 * Copyright (C) 2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stellaris.h"
#include <blackbox.h>

#include <libopencm3/usb/usbd.h>
#include <libopencm3/lm4f/nvic.h>
#include <libopencm3/lm4f/rcc.h>
#include <libopencm3/lm4f/gpio.h>
#include <libopencm3/lm4f/usb.h>
#include <stdio.h>

/**
 * @file usb_dev.c Hardware-specific USB peripheral functionality
 *
 * This file contains the bits to connect QiProg's USB device functionality to
 * the USB peripheral on the LM4F.
 */

typedef void (**usb_complete_cb) (usbd_device * usbd_dev,
				  struct usb_setup_data * req);

usbd_device *qiprog_dev;
u8 usbd_control_buffer[128];
extern usbd_driver lm4f_usb_driver;

/* =============================================================================
 * = USB descriptors
 * ---------------------------------------------------------------------------*/

static const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0110,
	.bDeviceClass = USB_CLASS_VENDOR,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = VULTUREPROG_USB_VID,
	.idProduct = VULTUREPROG_USB_PID,
	.bcdDevice = 0x0110,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor qiprog_endp[] = {{
	/*
	 * EP 1 OUT - Erase and write flash chip
	 */
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x01,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	/*
	 * EP 1 IN - Read flash chip
	 */
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x81,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}};

static const struct usb_interface_descriptor qiprog_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = 0xff,
	.bInterfaceSubClass = 0xff,
	.bInterfaceProtocol = 0xff,
	.iInterface = 0,

	.endpoint = qiprog_endp,

	.extra = NULL,
	.extralen = 0,
}};

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = qiprog_iface,
}};

static const struct usb_config_descriptor config_descr = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,
	.interface = ifaces,
};

static const char *usb_strings[] = {
	"Alexandru Gagniuc",
	"VultureProg",
	"none",
	"DEMO",
};

/* =============================================================================
 * = USB "glue"
 * ---------------------------------------------------------------------------*/

static void ep1_out_rx_cb(usbd_device * usbd_dev, u8 ep)
{
	(void)ep;
	(void)usbd_dev;

	/* TODO: Connect to QiProg logic */

	print_spew("EP 1 OUT: received some data\n");
}

static void ep1_in_tx_cb(usbd_device * usbd_dev, u8 ep)
{
	(void)ep;
	(void)usbd_dev;

	/* TODO: Connect to QiProg logic */

	print_spew("EP 1 IN: packet transmitted\n");
}

/*
 * Link control requests to QiProg logic
 */
static int qiprog_control_request(usbd_device * usbd_dev,
				  struct usb_setup_data *req, u8 ** buf,
				  u16 * len, usb_complete_cb complete)
{
	(void)usbd_dev;
	(void)buf;
	(void)len;
	(void)complete;

	/* TODO: Connect to QiProg logic */

	print_spew("Received a control request:\n");
	print_spew("bmRequestType: 0x%.2x\n", req->bmRequestType);
	print_spew("bRequest:      0x%.2x\n", req->bRequest);
	print_spew("wValue:        0x%.4x\n", req->wValue);
	print_spew("wIndex:        0x%.4x\n", req->wIndex);
	print_spew("wLength:       0x%.4x\n", req->wLength);

	/* NAK the request. FIXME: The ACK/NAK should be decided by QiProg */
	return 0;
}

/*
 * Initialize the USB configuration
 *
 * Called after the host issues a SetConfiguration request.
 */
static void set_config(usbd_device * usbd_dev, u16 wValue)
{
	(void)wValue;
	print_info("Configuring endpoints.\n\r");
	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64,
		      ep1_out_rx_cb);
	usbd_ep_setup(usbd_dev, 0x81, USB_ENDPOINT_ATTR_BULK, 64, ep1_in_tx_cb);

	usbd_register_control_callback(usbd_dev,
				       USB_REQ_TYPE_CLASS |
				       USB_REQ_TYPE_INTERFACE,
				       USB_REQ_TYPE_TYPE |
				       USB_REQ_TYPE_RECIPIENT,
				       qiprog_control_request);

	print_info("Done.\n\r");
}

/*
 * Configure D+/D- pins to their USB function
 */
static void usb_pins_setup(void)
{
	/* USB pins are connected to port D */
	periph_clock_enable(RCC_GPIOD);
	/* Mux USB pins to their analog function */
	gpio_mode_setup(GPIOD, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4 | GPIO5);
}

/*
 * Enable USB to run interrupt-driven
 */
static void usb_ints_setup(void)
{
	u8 usbints;
	/* Gimme some interrupts */
	usbints = USB_INT_RESET | USB_INT_DISCON | USB_INT_RESUME |
	    USB_INT_SUSPEND;
	usb_enable_interrupts(usbints, 0xff, 0xff);
	nvic_enable_irq(NVIC_USB0_IRQ);
}

void stellaris_usb_init(void)
{
	usb_pins_setup();

	qiprog_dev = usbd_init(&lm4f_usb_driver, &dev_descr, &config_descr,
			       usb_strings, 4,
			       usbd_control_buffer,
			       sizeof(usbd_control_buffer));

	usbd_register_set_config_callback(qiprog_dev, set_config);

	usb_ints_setup();
}

/*
 * USB runs entirely from interrupts
 */
void usb0_isr(void)
{
	usbd_poll(qiprog_dev);
}
