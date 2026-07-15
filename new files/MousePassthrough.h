/*
 * MousePassthrough.h
 * Header for the main firmware logic – declares buffers and event handlers.
 */

#pragma once

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <stdbool.h>
#include <string.h>

#include <LUFA/Drivers/USB/USB.h>
#include "Descriptors.h"
#include "max3421e.h"

/* Latest reports received from the downstream mouse */
extern uint8_t g_mouse_report[MOUSE_REPORT_SIZE];   /* 14 bytes */
extern uint8_t g_kbd_report[KBD_REPORT_SIZE];       /* 9 bytes */
extern volatile bool g_mouse_pending;
extern volatile bool g_kbd_pending;

/* Feature report cache for Report ID 5 (5 bytes) */
extern uint8_t g_feat5_buf[6];
extern bool    g_feat5_valid;

/* LUFA USB event handlers */
void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);

/* Application entry point */
int main(void);