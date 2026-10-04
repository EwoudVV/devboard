#include <stddef.h>
#include <string.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include "usb_serial.h"

#define TX_CAPACITY 512u
#define RX_CAPACITY 256u
static usbd_device *device;
static bool configured;
static bool dtr;
static uint8_t control_buffer[128];
static char tx[TX_CAPACITY], rx[RX_CAPACITY];
static unsigned tx_head, tx_tail, rx_head, rx_tail;
static uint32_t dropped;
static struct usb_cdc_line_coding line_coding = {
    .dwDTERate = 115200, .bCharFormat = 0, .bParityType = 0, .bDataBits = 8,
};

static const struct usb_device_descriptor device_descriptor = {
    .bLength = USB_DT_DEVICE_SIZE, .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200, .bDeviceClass = USB_CLASS_CDC,
    .bDeviceSubClass = 0, .bDeviceProtocol = 0, .bMaxPacketSize0 = 64,
    .idVendor = 0x0483, .idProduct = 0x5740, .bcdDevice = 0x0100,
    .iManufacturer = 1, .iProduct = 2, .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor notification_endpoint[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE, .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x83, .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize = 16, .bInterval = 255,
}};

static const struct usb_endpoint_descriptor data_endpoints[] = {
    {.bLength = USB_DT_ENDPOINT_SIZE, .bDescriptorType = USB_DT_ENDPOINT,
     .bEndpointAddress = 0x01, .bmAttributes = USB_ENDPOINT_ATTR_BULK,
     .wMaxPacketSize = 64, .bInterval = 0},
    {.bLength = USB_DT_ENDPOINT_SIZE, .bDescriptorType = USB_DT_ENDPOINT,
     .bEndpointAddress = 0x82, .bmAttributes = USB_ENDPOINT_ATTR_BULK,
     .wMaxPacketSize = 64, .bInterval = 0},
};

static const struct {
    struct usb_cdc_header_descriptor header;
    struct usb_cdc_call_management_descriptor call;
    struct usb_cdc_acm_descriptor acm;
    struct usb_cdc_union_descriptor interfaces;
} __attribute__((packed)) functions = {
    .header = {.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
               .bDescriptorType = CS_INTERFACE, .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
               .bcdCDC = 0x0110},
    .call = {.bFunctionLength = sizeof(struct usb_cdc_call_management_descriptor),
             .bDescriptorType = CS_INTERFACE, .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
             .bmCapabilities = 0, .bDataInterface = 1},
    .acm = {.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
            .bDescriptorType = CS_INTERFACE, .bDescriptorSubtype = USB_CDC_TYPE_ACM,
            .bmCapabilities = 2},
    .interfaces = {.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
                   .bDescriptorType = CS_INTERFACE, .bDescriptorSubtype = USB_CDC_TYPE_UNION,
                   .bControlInterface = 0, .bSubordinateInterface0 = 1},
};

static const struct usb_interface_descriptor comm_interface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE, .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0, .bAlternateSetting = 0, .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_CDC, .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = USB_CDC_PROTOCOL_AT, .iInterface = 0,
    .endpoint = notification_endpoint, .extra = &functions,
    .extralen = sizeof(functions),
}};

static const struct usb_interface_descriptor data_interface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE, .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 1, .bAlternateSetting = 0, .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_DATA, .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0, .iInterface = 0, .endpoint = data_endpoints,
}};

static const struct usb_interface interfaces[] = {
    {.num_altsetting = 1, .altsetting = comm_interface},
    {.num_altsetting = 1, .altsetting = data_interface},
};
static const struct usb_config_descriptor configuration = {
    .bLength = USB_DT_CONFIGURATION_SIZE, .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0, .bNumInterfaces = 2, .bConfigurationValue = 1,
    .iConfiguration = 0, .bmAttributes = 0x80, .bMaxPower = 50,
    .interface = interfaces,
};
static const char *strings[] = {"EVV", "Devboard USB serial", "DEVBOARD-001"};

static void reset(void)
{
    configured = false; dtr = false;
    tx_head = tx_tail = rx_head = rx_tail = 0;
}

static enum usbd_request_return_codes control(usbd_device *dev,
        struct usb_setup_data *request, uint8_t **buf, uint16_t *length,
        void (**complete)(usbd_device *, struct usb_setup_data *))
{
    (void)dev; (void)complete;
    if (request->wIndex != 0) return USBD_REQ_NOTSUPP;
    switch (request->bRequest) {
    case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
        if (request->wLength != 0) return USBD_REQ_NOTSUPP;
        dtr = (request->wValue & 1u) != 0;
        if (!dtr) tx_head = tx_tail = rx_head = rx_tail = 0;
        return USBD_REQ_HANDLED;
    case USB_CDC_REQ_SET_LINE_CODING:
        if (*length != sizeof line_coding) return USBD_REQ_NOTSUPP;
        memcpy(&line_coding, *buf, sizeof line_coding);
        return USBD_REQ_HANDLED;
    case USB_CDC_REQ_GET_LINE_CODING:
        *buf = (uint8_t *)&line_coding;
        *length = sizeof line_coding;
        return USBD_REQ_HANDLED;
    default:
        return USBD_REQ_NOTSUPP;
    }
}

static void receive(usbd_device *dev, uint8_t endpoint)
{
    char packet[64];
    uint16_t count = usbd_ep_read_packet(dev, endpoint, packet, sizeof packet);
    if (!dtr) return;
    unsigned free = (rx_tail + RX_CAPACITY - rx_head - 1u) % RX_CAPACITY;
    if (count > free) { dropped++; return; }
    for (unsigned i = 0; i < count; i++) {
        rx[rx_head] = packet[i]; rx_head = (rx_head + 1u) % RX_CAPACITY;
    }
}

static void configure(usbd_device *dev, uint16_t value)
{
    reset();
    configured = value == 1;
    if (!configured) return;
    usbd_ep_setup(dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, receive);
    usbd_ep_setup(dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    usbd_ep_setup(dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);
    usbd_register_control_callback(dev, USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
            USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT, control);
}

void usb_serial_init(void)
{
    device = usbd_init(&otgfs_usb_driver, &device_descriptor, &configuration,
            strings, 3, control_buffer, sizeof control_buffer);
    usbd_register_set_config_callback(device, configure);
    usbd_register_reset_callback(device, reset);
}

bool usb_serial_open(void) { return configured && dtr; }
uint32_t usb_serial_dropped(void) { return dropped; }

bool usb_serial_write(const char *text)
{
    if (!usb_serial_open()) return false;
    size_t length = strlen(text);
    unsigned free = (tx_tail + TX_CAPACITY - tx_head - 1u) % TX_CAPACITY;
    if (length > free) { dropped++; return false; }
    for (size_t i = 0; i < length; i++) {
        tx[tx_head] = text[i]; tx_head = (tx_head + 1u) % TX_CAPACITY;
    }
    return true;
}

bool usb_serial_getc(char *ch)
{
    if (rx_head == rx_tail) return false;
    *ch = rx[rx_tail]; rx_tail = (rx_tail + 1u) % RX_CAPACITY;
    return true;
}

void usb_serial_poll(void)
{
    usbd_poll(device);
    if (!usb_serial_open() || tx_head == tx_tail) return;
    char packet[63];
    unsigned count = 0, cursor = tx_tail;
    while (cursor != tx_head && count < sizeof packet) {
        packet[count++] = tx[cursor]; cursor = (cursor + 1u) % TX_CAPACITY;
    }
    if (usbd_ep_write_packet(device, 0x82, packet, count) == count) tx_tail = cursor;
}
