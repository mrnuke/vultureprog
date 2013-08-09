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

#include <qiprog_usb_dev.h>

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
uint8_t usbd_control_buffer[128];
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
	.idVendor = USB_VID_OPENMOKO,
	.idProduct = USB_PID_OPENMOKO_VULTUREPROG,
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

static uint16_t send_packet(void *data, uint16_t len)
{
	/* Only send the packet if we receive an IN token */
	if (USB_TXCSRL(1) & USB_TXCSRL_UNDRN)
		return usbd_ep_write_packet(qiprog_dev, 0x81, data, len);
	else
		return 0;
}

static uint16_t read_packet(void *data, uint16_t len)
{
	return usbd_ep_read_packet(qiprog_dev, 0x01, data, len);
}

static uint8_t qiprog_buf[256];

/*
 * Link control requests to QiProg logic
 */
static int qiprog_control_request(usbd_device * usbd_dev,
				  struct usb_setup_data *req, uint8_t ** buf,
				  uint16_t * len, usb_complete_cb complete)
{
	qiprog_err ret;

	(void)usbd_dev;
	(void)complete;

	/*
	 * Don't be overly verbose in a control transaction.
	 *
	 * If we need to stall the endpoint, but don't do it fast enough, the
	 * host may not see an error. It could be due to a bug in our USB driver
	 * or just a fact of USB in general.
	 *
	 * Just be shy for now.
	 */
	print_spew("bRequest: 0x%.2x\n", req->bRequest);

	ret = qiprog_handle_control_request(req->bRequest, req->wValue,
					    req->wIndex, req->wLength,
					    buf, len);

	if (ret != QIPROG_SUCCESS) {
		print_err("Request was not handled, code %i\n", ret);
	}
	/* ACK or STALL based on whether QiProg handles the request */
	return (ret == QIPROG_SUCCESS);
}

extern struct qiprog_device stellaris_lpc_dev;
/*
 * Initialize the USB configuration
 *
 * Called after the host issues a SetConfiguration request.
 */
static void set_config(usbd_device * usbd_dev, uint16_t wValue)
{
	(void)wValue;
	print_info("Configuring endpoints.\n\r");
	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, 0x81, USB_ENDPOINT_ATTR_BULK, 64, NULL);

	usbd_register_control_callback(usbd_dev,
				       USB_REQ_TYPE_VENDOR,
				       USB_REQ_TYPE_TYPE |
				       USB_REQ_TYPE_RECIPIENT,
				       qiprog_control_request);

	qiprog_change_device(&stellaris_lpc_dev);

	qiprog_usb_dev_init(send_packet, read_packet, 64, 64, qiprog_buf);

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
	uint8_t usbints;
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
