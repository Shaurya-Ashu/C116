#include "usb_cdc.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <string.h>


static const struct usb_device_descriptor dev_desc = {
    .bLength            = USB_DT_DEVICE_SIZE,
    .bDescriptorType    = USB_DT_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = USB_CLASS_CDC,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0x0483,
    .idProduct          = 0x5740,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor comm_ep[] = {{
    .bLength          = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x83,
    .bmAttributes     = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize   = 16,
    .bInterval        = 255,
}};

static const struct usb_endpoint_descriptor data_ep[] = {
    {
        .bLength          = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType  = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x01,
        .bmAttributes     = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize   = 64,
        .bInterval        = 1,
    },
    {
        .bLength          = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType  = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x82,
        .bmAttributes     = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize   = 64,
        .bInterval        = 1,
    },
};

static const struct {
    struct usb_cdc_header_descriptor         header;
    struct usb_cdc_call_management_descriptor call_mgmt;
    struct usb_cdc_acm_descriptor             acm;
    struct usb_cdc_union_descriptor           cdc_union;
} __attribute__((packed)) cdc_functional = {
    .header = {
        .bFunctionLength    = sizeof(struct usb_cdc_header_descriptor),
        .bDescriptorType    = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
        .bcdCDC             = 0x0110,
    },
    .call_mgmt = {
        .bFunctionLength    = sizeof(struct usb_cdc_call_management_descriptor),
        .bDescriptorType    = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
        .bmCapabilities     = 0,
        .bDataInterface     = 1,
    },
    .acm = {
        .bFunctionLength    = sizeof(struct usb_cdc_acm_descriptor),
        .bDescriptorType    = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_ACM,
        .bmCapabilities     = 0,
    },
    .cdc_union = {
        .bFunctionLength        = sizeof(struct usb_cdc_union_descriptor),
        .bDescriptorType        = CS_INTERFACE,
        .bDescriptorSubtype     = USB_CDC_TYPE_UNION,
        .bControlInterface      = 0,
        .bSubordinateInterface0 = 1,
    },
};

static const struct usb_interface_descriptor comm_iface[] = {{
    .bLength            = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 1,
    .bInterfaceClass    = USB_CLASS_CDC,
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
    .iInterface         = 0,
    .endpoint           = comm_ep,
    .extra              = &cdc_functional,
    .extralen           = sizeof(cdc_functional),
}};

static const struct usb_interface_descriptor data_iface[] = {{
    .bLength            = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 1,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 2,
    .bInterfaceClass    = USB_CLASS_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = 0,
    .endpoint           = data_ep,
}};

static const struct usb_interface ifaces[] = {
    { .num_altsetting = 1, .altsetting = comm_iface },
    { .num_altsetting = 1, .altsetting = data_iface },
};

static const struct usb_config_descriptor config_desc = {
    .bLength             = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType     = USB_DT_CONFIGURATION,
    .wTotalLength        = 0,
    .bNumInterfaces      = 2,
    .bConfigurationValue = 1,
    .iConfiguration      = 0,
    .bmAttributes        = 0x80,
    .bMaxPower           = 0x32,   
    .interface           = ifaces,
};

static const char *usb_strings[] = {
    "Harsh / Tiny MCU Project",
    "Tiny MCU v1.0",
    "00000001",
};

static uint8_t  usbd_control_buf[128];
static usbd_device *_usbd_dev = NULL;
static int      _connected    = 0;
static enum usbd_request_return_codes
cdc_control_request(usbd_device *dev,
                    struct usb_setup_data *req,
                    uint8_t **buf, uint16_t *len,
                    usbd_control_complete_callback *complete)
{
    (void)dev; (void)buf; (void)len; (void)complete;

    switch (req->bRequest) {
    case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
        _connected = req->wValue & 1;  
        return USBD_REQ_HANDLED;
    case USB_CDC_REQ_SET_LINE_CODING:
        return USBD_REQ_HANDLED;
    }
    return USBD_REQ_NOTSUPP;
}


static void cdc_rx_cb(usbd_device *dev, uint8_t ep)
{
    (void)ep;
    char buf[64];
    uint16_t len = usbd_ep_read_packet(dev, 0x01, buf, 64);
    if (len) usbd_ep_write_packet(dev, 0x82, buf, len);
}


static void cdc_set_config(usbd_device *dev, uint16_t wValue)
{
    (void)wValue;
    usbd_ep_setup(dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdc_rx_cb);
    usbd_ep_setup(dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    usbd_ep_setup(dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);
    usbd_register_control_callback(
        dev,
        USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE  | USB_REQ_TYPE_RECIPIENT,
        cdc_control_request);
}


usbd_device *usb_cdc_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
    gpio_set_af(GPIOA, GPIO_AF2, GPIO11 | GPIO12);

    rcc_periph_clock_enable(RCC_USB);

    _usbd_dev = usbd_init(&st_usbfs_v2_usb_driver, &dev_desc, &config_desc,
                          usb_strings, 3,
                          usbd_control_buf, sizeof(usbd_control_buf));

    usbd_register_set_config_callback(_usbd_dev, cdc_set_config);
    return _usbd_dev;
}

void usb_cdc_send(const char *str)
{
    if (!_usbd_dev || !_connected) return;
    uint16_t len = (uint16_t)strlen(str);
    while (len > 0) {
        uint16_t chunk = len > 64 ? 64 : len;
        usbd_ep_write_packet(_usbd_dev, 0x82, str, chunk);
        str += chunk;
        len -= chunk;
    }
}

void usb_cdc_send_buf(const uint8_t *buf, uint16_t len)
{
    if (!_usbd_dev || !_connected) return;
    while (len > 0) {
        uint16_t chunk = len > 64 ? 64 : len;
        usbd_ep_write_packet(_usbd_dev, 0x82, buf, chunk);
        buf += chunk;
        len -= chunk;
    }
}

int usb_cdc_connected(void) { return _connected; }
