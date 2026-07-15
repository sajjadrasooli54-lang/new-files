/*
 * Descriptors.c
 * USB Descriptors for Logitech PRO X 2 (VID=0x046D, PID=0xC09B)
 * Captured from real device on 2026-07-11 via Wireshark/USBPcap.
 * 
 * Replaces Sino Wealth (0x258A/0x0036) with actual PRO X 2 descriptors.
 * Contains:
 *   - Device Descriptor (VID=0x046D, PID=0xC09B, bcdDevice=0x3205)
 *   - Configuration Descriptor (3 interfaces: Mouse, Keyboard, Vendor)
 *   - HID Report Descriptors:
 *       Interface 0 (Mouse):  83 bytes (14-byte input reports)
 *       Interface 1 (Keyboard): 125 bytes (9-byte input reports, 2-byte output)
 *       Interface 2 (Vendor):  54 bytes (Report IDs 0x10 and 0x11)
 *   - String Descriptors: "Logitech", "PRO X 2", "99FAAB88", "MPM32.05_B0029"
 */

#include "Descriptors.h"

/* ── Device Descriptor ───────────────────────────────────────────────────── */
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
    .Header                 = { .Size = sizeof(USB_Descriptor_Device_t),
                                .Type = DTYPE_Device },
    .USBSpecification       = VERSION_BCD(2, 0, 0),   /* USB 2.0 bcdUSB=0x0200 */
    .Class                  = USB_CSCP_NoDeviceClass,
    .SubClass               = USB_CSCP_NoDeviceSubclass,
    .Protocol               = USB_CSCP_NoDeviceProtocol,
    .Endpoint0Size          = 64,                     /* bMaxPacketSize0 = 0x40 */
    .VendorID               = 0x046D,                 /* Logitech Inc. */
    .ProductID              = 0xC09B,                 /* PRO X 2 */
    .ReleaseNumber          = VERSION_BCD(3, 2, 5),   /* bcdDevice = 0x3205 */
    .ManufacturerStrIndex   = 0x01,
    .ProductStrIndex        = 0x02,
    .SerialNumStrIndex      = 0x03,
    .NumberOfConfigurations = 1,
};

/* ── Mouse HID Report Descriptor (83 bytes, captured from PRO X 2) ──────── */
const uint8_t PROGMEM MouseReportDescriptor[MOUSE_REPORT_DESC_SIZE] = {
    0x05, 0x01,        /* Usage Page (Generic Desktop) */
    0x09, 0x02,        /* Usage (Mouse) */
    0xA1, 0x01,        /* Collection (Application) */
      0x09, 0x01,      /*   Usage (Pointer) */
      0xA1, 0x00,      /*   Collection (Physical) */
        0x95, 0x10,    /*     Report Count (16) */
        0x75, 0x01,    /*     Report Size (1) */
        0x15, 0x00,    /*     Logical Minimum (0) */
        0x25, 0x01,    /*     Logical Maximum (1) */
        0x05, 0x09,    /*     Usage Page (Button) */
        0x19, 0x01,    /*     Usage Minimum (1) */
        0x29, 0x10,    /*     Usage Maximum (16) */
        0x81, 0x02,    /*     Input (Data, Var, Abs) – 16 buttons */
        0x95, 0x02,    /*     Report Count (2) */
        0x75, 0x10,    /*     Report Size (16) */
        0x16, 0x01, 0x80, /* Logical Minimum (-32767) */
        0x26, 0xFF, 0x7F, /* Logical Maximum (32767) */
        0x05, 0x01,    /*     Usage Page (Generic Desktop) */
        0x09, 0x30,    /*     Usage (X) */
        0x09, 0x31,    /*     Usage (Y) */
        0x81, 0x06,    /*     Input (Data, Var, Rel) – X, Y 16-bit */
        0x95, 0x01,    /*     Report Count (1) */
        0x75, 0x08,    /*     Report Size (8) */
        0x15, 0x81,    /*     Logical Minimum (-127) */
        0x25, 0x7F,    /*     Logical Maximum (127) */
        0x09, 0x38,    /*     Usage (Wheel) */
        0x81, 0x06,    /*     Input (Data, Var, Rel) */
        0x95, 0x01,    /*     Report Count (1) */
        0x05, 0x0C,    /*     Usage Page (Consumer) */
        0x0A, 0x38, 0x02, /* Usage (AC Pan) */
        0x81, 0x06,    /*     Input (Data, Var, Rel) */
      0xC0,            /*   End Collection (Physical) */
      0x06, 0x00, 0xFF,/*   Usage Page (Vendor 0xFF00) */
      0x09, 0xF1,      /*   Usage (0xF1) */
      0x75, 0x08,      /*   Report Size (8) */
      0x95, 0x05,      /*   Report Count (5) */
      0x15, 0x00,      /*   Logical Minimum (0) */
      0x26, 0xFF, 0x00,/*   Logical Maximum (255) */
      0x81, 0x00,      /*   Input (Data, Array) – 5 vendor bytes */
    0xC0,              /* End Collection (Application) */
};

/* ── Keyboard / Consumer / System HID Report Descriptor (125 bytes) ────── */
const uint8_t PROGMEM KeyboardReportDescriptor[KBD_REPORT_DESC_SIZE] = {
    /* --- Report ID 1: Standard Keyboard (modifiers + 6 keys) --- */
    0x05, 0x01,        /* Usage Page (Generic Desktop) */
    0x09, 0x06,        /* Usage (Keyboard) */
    0xA1, 0x01,        /* Collection (Application) */
      0x85, 0x01,      /*   Report ID (1) */
      0x95, 0x08,      /*   Report Count (8) */
      0x75, 0x01,      /*   Report Size (1) */
      0x15, 0x00,      /*   Logical Minimum (0) */
      0x25, 0x01,      /*   Logical Maximum (1) */
      0x05, 0x07,      /*   Usage Page (Keyboard) */
      0x19, 0xE0,      /*   Usage Minimum (Left Ctrl) */
      0x29, 0xE7,      /*   Usage Maximum (Right GUI) */
      0x81, 0x02,      /*   Input (Data, Var, Abs) – modifier byte */
      0x81, 0x03,      /*   Input (Const, Var) – padding */
      0x95, 0x05,      /*   Report Count (5) */
      0x05, 0x08,      /*   Usage Page (LEDs) */
      0x19, 0x01,      /*   Usage Minimum (NumLock) */
      0x29, 0x05,      /*   Usage Maximum (Kana) */
      0x91, 0x02,      /*   Output (Data, Var, Abs) – LED report */
      0x95, 0x01,      /*   Report Count (1) */
      0x75, 0x03,      /*   Report Size (3) */
      0x91, 0x03,      /*   Output (Const) – padding */
      0x95, 0x06,      /*   Report Count (6) */
      0x75, 0x08,      /*   Report Size (8) */
      0x15, 0x00,      /*   Logical Minimum (0) */
      0x26, 0xFF, 0x00,/*   Logical Maximum (255) */
      0x05, 0x07,      /*   Usage Page (Keyboard) */
      0x19, 0x00,      /*   Usage Minimum (0) */
      0x2A, 0xFF, 0x00,/*   Usage Maximum (255) */
      0x81, 0x00,      /*   Input (Data, Array) – 6 keycodes */
    0xC0,              /* End Collection */

    /* --- Report ID 3: Consumer Control --- */
    0x05, 0x0C,        /* Usage Page (Consumer) */
    0x09, 0x01,        /* Usage (Consumer Control) */
    0xA1, 0x01,        /* Collection (Application) */
      0x85, 0x03,      /*   Report ID (3) */
      0x95, 0x02,      /*   Report Count (2) */
      0x75, 0x10,      /*   Report Size (16) */
      0x15, 0x01,      /*   Logical Minimum (1) */
      0x26, 0xFF, 0x02,/*   Logical Maximum (767) */
      0x19, 0x01,      /*   Usage Minimum (1) */
      0x2A, 0xFF, 0x02,/*   Usage Maximum (767) */
      0x81, 0x00,      /*   Input (Data, Array) – 16-bit usages */
    0xC0,              /* End Collection */

    /* --- Report ID 4: System Control --- */
    0x05, 0x01,        /* Usage Page (Generic Desktop) */
    0x09, 0x80,        /* Usage (System Control) */
    0xA1, 0x01,        /* Collection (Application) */
      0x85, 0x04,      /*   Report ID (4) */
      0x95, 0x01,      /*   Report Count (1) */
      0x75, 0x02,      /*   Report Size (2) */
      0x15, 0x01,      /*   Logical Minimum (1) */
      0x25, 0x03,      /*   Logical Maximum (3) */
      0x09, 0x82,      /*   Usage (System Sleep) */
      0x09, 0x81,      /*   Usage (System Power Down) */
      0x09, 0x83,      /*   Usage (System Wake Up) */
      0x81, 0x00,      /*   Input (Data, Array) – 2-bit system control */
      0x75, 0x01,      /*   Report Size (1) */
      0x15, 0x00,      /*   Logical Minimum (0) */
      0x25, 0x01,      /*   Logical Maximum (1) */
      0x09, 0x9B,      /*   Usage (System Do Not Disturb) */
      0x81, 0x06,      /*   Input (Data, Var, Rel) */
      0x75, 0x05,      /*   Report Size (5) */
      0x81, 0x03,      /*   Input (Const) – padding */
    0xC0,              /* End Collection */
};

/* ── Vendor Interface HID Report Descriptor (54 bytes) ──────────────────── */
const uint8_t PROGMEM VendorReportDescriptor[VENDOR_REPORT_DESC_SIZE] = {
    /* --- Report ID 16 (0x10): 7-byte Input/Output --- */
    0x06, 0x00, 0xFF,  /* Usage Page (Vendor 0xFF00) */
    0x09, 0x01,        /* Usage (0x01) */
    0xA1, 0x01,        /* Collection (Application) */
      0x85, 0x10,      /*   Report ID (16) */
      0x95, 0x06,      /*   Report Count (6) */
      0x75, 0x08,      /*   Report Size (8) */
      0x15, 0x00,      /*   Logical Minimum (0) */
      0x26, 0xFF, 0x00,/*   Logical Maximum (255) */
      0x09, 0x01,      /*   Usage (0x01) */
      0x81, 0x00,      /*   Input (Data, Array) – 6 bytes IN */
      0x09, 0x01,      /*   Usage (0x01) */
      0x91, 0x00,      /*   Output (Data, Array) – 6 bytes OUT (injection channel) */
    0xC0,              /* End Collection */

    /* --- Report ID 17 (0x11): 20-byte Input/Output (DPI, RGB, etc.) --- */
    0x06, 0x00, 0xFF,  /* Usage Page (Vendor 0xFF00) */
    0x09, 0x02,        /* Usage (0x02) */
    0xA1, 0x01,        /* Collection (Application) */
      0x85, 0x11,      /*   Report ID (17) */
      0x95, 0x13,      /*   Report Count (19) */
      0x75, 0x08,      /*   Report Size (8) */
      0x15, 0x00,      /*   Logical Minimum (0) */
      0x26, 0xFF, 0x00,/*   Logical Maximum (255) */
      0x09, 0x02,      /*   Usage (0x02) */
      0x81, 0x00,      /*   Input (Data, Array) – 19 bytes IN */
      0x09, 0x02,      /*   Usage (0x02) */
      0x91, 0x00,      /*   Output (Data, Array) – 19 bytes OUT */
    0xC0,              /* End Collection */
};

/* ── Configuration Descriptor ───────────────────────────────────────────── */
const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
    .Config = {
        .Header = { .Size = sizeof(USB_Descriptor_Configuration_Header_t),
                    .Type = DTYPE_Configuration },
        .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
        .TotalInterfaces        = 3,
        .ConfigurationNumber    = 1,
        .ConfigurationStrIndex  = 0x04,
        .ConfigAttributes       = (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED | USB_CONFIG_ATTR_REMOTEWAKEUP),
        .MaxPowerConsumption    = USB_CONFIG_POWER_MA(500),
    },

    /* Interface 0 – HID Mouse (Boot, protocol=Mouse) */
    .HID_MouseInterface = {
        .Header = { .Size = sizeof(USB_Descriptor_Interface_t),
                    .Type = DTYPE_Interface },
        .InterfaceNumber   = 0,
        .AlternateSetting  = 0,
        .TotalEndpoints    = 1,
        .Class             = HID_CSCP_HIDClass,
        .SubClass          = HID_CSCP_BootSubclass,
        .Protocol          = HID_CSCP_MouseBootProtocol,
        .InterfaceStrIndex = NO_DESCRIPTOR,
    },
    .HID_MouseHID = {
        .Header = { .Size = sizeof(USB_HID_Descriptor_HID_t),
                    .Type = HID_DTYPE_HID },
        .HIDSpec               = VERSION_BCD(1, 1, 1),
        .CountryCode           = 0x00,
        .TotalReportDescriptors = 1,
        .HIDReportType         = HID_DTYPE_Report,
        .HIDReportLength       = MOUSE_REPORT_DESC_SIZE,
    },
    .HID_MouseEP = {
        .Header = { .Size = sizeof(USB_Descriptor_Endpoint_t),
                    .Type = DTYPE_Endpoint },
        .EndpointAddress   = MOUSE_IN_EPADDR,
        .Attributes        = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize      = MOUSE_EPSIZE,
        .PollingIntervalMS = 1,
    },

    /* Interface 1 – HID Keyboard+Consumer+System (Boot, protocol=Keyboard) */
    .HID_KbdInterface = {
        .Header = { .Size = sizeof(USB_Descriptor_Interface_t),
                    .Type = DTYPE_Interface },
        .InterfaceNumber   = 1,
        .AlternateSetting  = 0,
        .TotalEndpoints    = 1,
        .Class             = HID_CSCP_HIDClass,
        .SubClass          = HID_CSCP_BootSubclass,
        .Protocol          = HID_CSCP_KeyboardBootProtocol,
        .InterfaceStrIndex = NO_DESCRIPTOR,
    },
    .HID_KbdHID = {
        .Header = { .Size = sizeof(USB_HID_Descriptor_HID_t),
                    .Type = HID_DTYPE_HID },
        .HIDSpec               = VERSION_BCD(1, 1, 1),
        .CountryCode           = 0x00,
        .TotalReportDescriptors = 1,
        .HIDReportType         = HID_DTYPE_Report,
        .HIDReportLength       = KBD_REPORT_DESC_SIZE,
    },
    .HID_KbdEP = {
        .Header = { .Size = sizeof(USB_Descriptor_Endpoint_t),
                    .Type = DTYPE_Endpoint },
        .EndpointAddress   = KBD_IN_EPADDR,
        .Attributes        = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize      = KBD_EPSIZE,
        .PollingIntervalMS = 1,
    },

    /* Interface 2 – Vendor (0xFF00) with Report IDs 0x10 and 0x11 */
    .HID_VendorInterface = {
        .Header = { .Size = sizeof(USB_Descriptor_Interface_t),
                    .Type = DTYPE_Interface },
        .InterfaceNumber   = 2,
        .AlternateSetting  = 0,
        .TotalEndpoints    = 1,
        .Class             = HID_CSCP_HIDClass,
        .SubClass          = 0x00,
        .Protocol          = 0x00,
        .InterfaceStrIndex = NO_DESCRIPTOR,
    },
    .HID_VendorHID = {
        .Header = { .Size = sizeof(USB_HID_Descriptor_HID_t),
                    .Type = HID_DTYPE_HID },
        .HIDSpec               = VERSION_BCD(1, 1, 1),
        .CountryCode           = 0x00,
        .TotalReportDescriptors = 1,
        .HIDReportType         = HID_DTYPE_Report,
        .HIDReportLength       = VENDOR_REPORT_DESC_SIZE,
    },
    .HID_VendorEP = {
        .Header = { .Size = sizeof(USB_Descriptor_Endpoint_t),
                    .Type = DTYPE_Endpoint },
        .EndpointAddress   = VENDOR_IN_EPADDR,
        .Attributes        = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize      = VENDOR_EPSIZE,
        .PollingIntervalMS = 1,
    },
};

/* ── String Descriptors ─────────────────────────────────────────────────── */
const USB_Descriptor_String_t PROGMEM LanguageString =
    USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);

const USB_Descriptor_String_t PROGMEM ManufacturerString =
    USB_STRING_DESCRIPTOR(L"Logitech");

const USB_Descriptor_String_t PROGMEM ProductString =
    USB_STRING_DESCRIPTOR(L"PRO X 2");

const USB_Descriptor_String_t PROGMEM SerialString =
    USB_STRING_DESCRIPTOR(L"99FAAB88");

const USB_Descriptor_String_t PROGMEM ConfigString =
    USB_STRING_DESCRIPTOR(L"MPM32.05_B0029");

/* ── LUFA descriptor callback ───────────────────────────────────────────── */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                     const uint16_t wIndex,
                                     const void** const DescriptorAddress)
{
    const uint8_t DescType  = (wValue >> 8);
    const uint8_t DescIndex = (wValue & 0xFF);

    switch (DescType) {
        case DTYPE_Device:
            *DescriptorAddress = &DeviceDescriptor;
            return sizeof(USB_Descriptor_Device_t);

        case DTYPE_Configuration:
            *DescriptorAddress = &ConfigurationDescriptor;
            return sizeof(USB_Descriptor_Configuration_t);

        case DTYPE_String:
            switch (DescIndex) {
                case 0x00:
                    *DescriptorAddress = &LanguageString;
                    return pgm_read_byte(&LanguageString.Header.Size);
                case 0x01:
                    *DescriptorAddress = &ManufacturerString;
                    return pgm_read_byte(&ManufacturerString.Header.Size);
                case 0x02:
                    *DescriptorAddress = &ProductString;
                    return pgm_read_byte(&ProductString.Header.Size);
                case 0x03:
                    *DescriptorAddress = &SerialString;
                    return pgm_read_byte(&SerialString.Header.Size);
                case 0x04:
                    *DescriptorAddress = &ConfigString;
                    return pgm_read_byte(&ConfigString.Header.Size);
                default:
                    return NO_DESCRIPTOR;
            }

        case HID_DTYPE_HID:
            switch (wIndex) {
                case 0:
                    *DescriptorAddress = &ConfigurationDescriptor.HID_MouseHID;
                    return sizeof(USB_HID_Descriptor_HID_t);
                case 1:
                    *DescriptorAddress = &ConfigurationDescriptor.HID_KbdHID;
                    return sizeof(USB_HID_Descriptor_HID_t);
                case 2:
                    *DescriptorAddress = &ConfigurationDescriptor.HID_VendorHID;
                    return sizeof(USB_HID_Descriptor_HID_t);
                default:
                    return NO_DESCRIPTOR;
            }

        case HID_DTYPE_Report:
            switch (wIndex) {
                case 0:
                    *DescriptorAddress = MouseReportDescriptor;
                    return MOUSE_REPORT_DESC_SIZE;
                case 1:
                    *DescriptorAddress = KeyboardReportDescriptor;
                    return KBD_REPORT_DESC_SIZE;
                case 2:
                    *DescriptorAddress = VendorReportDescriptor;
                    return VENDOR_REPORT_DESC_SIZE;
                default:
                    return NO_DESCRIPTOR;
            }

        default:
            return NO_DESCRIPTOR;
    }
}