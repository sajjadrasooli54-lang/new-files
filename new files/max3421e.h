/*
 * Descriptors.h
 * Header for USB descriptors – defines endpoint addresses, report sizes,
 * and descriptor lengths for Logitech PRO X 2 (VID=0x046D, PID=0xC09B).
 */

#pragma once

#include <avr/pgmspace.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/USB/Class/Common/HIDClassCommon.h>

/* ── Endpoint addresses ──────────────────────────────────────────────────── */
#define MOUSE_IN_EPADDR   (ENDPOINT_DIR_IN | 1)   /* EP1 IN  = 0x81 (Interface 0) */
#define KBD_IN_EPADDR     (ENDPOINT_DIR_IN | 2)   /* EP2 IN  = 0x82 (Interface 1) */
#define VENDOR_IN_EPADDR  (ENDPOINT_DIR_IN | 3)   /* EP3 IN  = 0x83 (Interface 2) */

/* ── Endpoint sizes (from device descriptor wMaxPacketSize = 0x0040) ───── */
#define MOUSE_EPSIZE      64
#define KBD_EPSIZE        64
#define VENDOR_EPSIZE     64

/* ── HID report sizes (from HID capabilities) ──────────────────────────── */
#define MOUSE_REPORT_SIZE      14   /* InputReportByteLength = 0x0E */
#define KBD_REPORT_SIZE        9    /* InputReportByteLength = 0x09 */
#define INJECT_REPORT_SIZE     7    /* Report ID 0x10 = ID(1) + payload(6) */

/* ── HID report descriptor sizes ────────────────────────────────────────── */
#define MOUSE_REPORT_DESC_SIZE    83
#define KBD_REPORT_DESC_SIZE      125
#define VENDOR_REPORT_DESC_SIZE   54

/* ── Full configuration descriptor layout ────────────────────────────────── */
typedef struct {
    USB_Descriptor_Configuration_Header_t Config;

    /* Interface 0: Mouse */
    USB_Descriptor_Interface_t            HID_MouseInterface;
    USB_HID_Descriptor_HID_t             HID_MouseHID;
    USB_Descriptor_Endpoint_t            HID_MouseEP;

    /* Interface 1: Keyboard + Consumer + System */
    USB_Descriptor_Interface_t            HID_KbdInterface;
    USB_HID_Descriptor_HID_t             HID_KbdHID;
    USB_Descriptor_Endpoint_t            HID_KbdEP;

    /* Interface 2: Vendor (0xFF00) */
    USB_Descriptor_Interface_t            HID_VendorInterface;
    USB_HID_Descriptor_HID_t             HID_VendorHID;
    USB_Descriptor_Endpoint_t            HID_VendorEP;
} USB_Descriptor_Configuration_t;

/* ── Exported descriptors (stored in flash) ─────────────────────────────── */
extern const USB_Descriptor_Device_t        PROGMEM DeviceDescriptor;
extern const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor;
extern const USB_Descriptor_String_t        PROGMEM LanguageString;
extern const USB_Descriptor_String_t        PROGMEM ManufacturerString;
extern const USB_Descriptor_String_t        PROGMEM ProductString;
extern const USB_Descriptor_String_t        PROGMEM SerialString;
extern const USB_Descriptor_String_t        PROGMEM ConfigString;

/* ── HID report descriptors ─────────────────────────────────────────────── */
extern const uint8_t PROGMEM MouseReportDescriptor[];
extern const uint8_t PROGMEM KeyboardReportDescriptor[];
extern const uint8_t PROGMEM VendorReportDescriptor[];

/* ── LUFA descriptor callback ───────────────────────────────────────────── */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                     const uint16_t wIndex,
                                     const void** const DescriptorAddress);