/*
 * LUFAConfig.h
 * LUFA compile-time configuration – enables USB device mode,
 * uses flash descriptors, and sets fixed control endpoint size.
 */

#pragma once

#define USE_STATIC_OPTIONS   (USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)
#define USB_DEVICE_ONLY
#define FIXED_CONTROL_ENDPOINT_SIZE    8
#define FIXED_NUM_CONFIGURATIONS       1
#define DEVICE_STATE_AS_GPIOR          0
#define ORDERED_EP_CONFIG
/* All descriptors are in PROGMEM (flash) — removes the DescriptorMemorySpace
   parameter from CALLBACK_USB_GetDescriptor so the signature matches ours. */
#define USE_FLASH_DESCRIPTORS