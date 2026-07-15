#include "MousePassthrough.h"

/* ── Debug LED on pin 13 (PC7) ──────────────────────────────────────────── */

#define LED_DDR   DDRC
#define LED_PORT  PORTC
#define LED_BIT   PC7
#define LED_ON()  (LED_PORT |= (1 << LED_BIT))
#define LED_OFF() (LED_PORT &= ~(1 << LED_BIT))

/* ── Global report buffers ──────────────────────────────────────────────── */

uint8_t g_mouse_report[MOUSE_REPORT_SIZE];
uint8_t g_kbd_report[KBD_REPORT_SIZE];
volatile bool g_mouse_pending = false;
volatile bool g_kbd_pending   = false;

uint8_t g_feat5_buf[6];
bool    g_feat5_valid = false;

static uint8_t idle_rate[3] = {0, 0, 0}; /* one per interface */

/* ── Feature-report caches (pre-fetched from the real mouse) ────────────── */

static uint8_t feat4_cache[520];
static uint8_t feat5_cache[6];
static bool features_fetched = false;

/* ── Deferred SET_REPORT forwarding ─────────────────────────────────────── */

static bool    set4_pending = false;
static uint16_t set4_len    = 0;
static uint8_t  set4_iface  = 0;

static bool    set5_pending = false;
static uint16_t set5_len    = 0;
static uint8_t  set5_iface  = 0;

/* ── Injection from usermode app (Report ID 0x10 on Interface 2) ───────── */

static volatile bool    inject_pending = false;
static volatile int16_t inject_dx      = 0;
static volatile int16_t inject_dy      = 0;

/* ── Cooldown control (from SET_REPORT 0x10 with command 0x02) ─────────── */

static volatile uint16_t cooldown_ms = 50;   /* default 50ms */

/* Last button/wheel/pan state from the real mouse so injected reports
   preserve the real mouse's button state. */
static uint8_t last_buttons[2]  = {0x00, 0x00}; /* buttons 1-8, 9-16 */
static uint8_t last_wheel       = 0x00;
static uint8_t last_pan         = 0x00;
static uint8_t last_vendor[5]   = {0x00, 0x00, 0x00, 0x00, 0x00};
static uint32_t last_p_time     = 0;

/* ── Send latest mouse report to EP1 ────────────────────────────────────── */

static void send_mouse_report(void)
{
    Endpoint_SelectEndpoint(MOUSE_IN_EPADDR);
    if (!Endpoint_IsEnabled() || !Endpoint_IsConfigured())
        return;
    if (Endpoint_IsINReady()) {
        Endpoint_Write_Stream_LE(g_mouse_report, MOUSE_REPORT_SIZE, NULL);
        Endpoint_ClearIN();
    }
}

/* ── Send latest keyboard report to EP2 ─────────────────────────────────── */

static void send_kbd_report(void)
{
    Endpoint_SelectEndpoint(KBD_IN_EPADDR);
    if (!Endpoint_IsEnabled() || !Endpoint_IsConfigured())
        return;
    if (Endpoint_IsINReady()) {
        Endpoint_Write_Stream_LE(g_kbd_report, KBD_REPORT_SIZE, NULL);
        Endpoint_ClearIN();
    }
}

/* ── Pre-fetch feature reports from the downstream mouse (with retries) ── */

static void prefetch_features(void)
{
    if (g_dev_state != DEV_ENUMERATED)
        return;

    for (uint8_t attempt = 0; attempt < 3; attempt++) {
        memset(feat4_cache, 0, sizeof(feat4_cache));
        const uint8_t sp4[8] = {0xA1, 0x01, 0x04, 0x03,
                                 0x01, 0x00, 0x08, 0x02};
        uint8_t r = max3421e_ctrl_rd(g_dev_addr, sp4, feat4_cache, 520);
        if (r == hrSUCCESS && feat4_cache[0] != 0x00)
            break;
        _delay_ms(50);
    }

    for (uint8_t attempt = 0; attempt < 3; attempt++) {
        memset(feat5_cache, 0, sizeof(feat5_cache));
        const uint8_t sp5[8] = {0xA1, 0x01, 0x05, 0x03,
                                 0x01, 0x00, 0x06, 0x00};
        uint8_t r = max3421e_ctrl_rd(g_dev_addr, sp5, feat5_cache, 6);
        if (r == hrSUCCESS)
            break;
        _delay_ms(50);
    }

    features_fetched = true;
}

/* ── Forward deferred SET_REPORTs and re-fetch ──────────────────────────── */

static void forward_pending_reports(void)
{
    if (g_dev_state != DEV_ENUMERATED)
        return;

    if (set4_pending) {
        uint8_t sp[8] = {0x21, 0x09, 0x04, 0x03, set4_iface, 0x00,
                         (uint8_t)(set4_len & 0xFF),
                         (uint8_t)(set4_len >> 8)};
        max3421e_ctrl_wr(g_dev_addr, sp, feat4_cache, set4_len);

        const uint8_t gp[8] = {0xA1, 0x01, 0x04, 0x03,
                                set4_iface, 0x00, 0x08, 0x02};
        max3421e_ctrl_rd(g_dev_addr, gp, feat4_cache, 520);
        set4_pending = false;
    }

    if (set5_pending) {
        uint8_t sp[8] = {0x21, 0x09, 0x05, 0x03, set5_iface, 0x00,
                         (uint8_t)(set5_len & 0xFF), 0x00};
        max3421e_ctrl_wr(g_dev_addr, sp, feat5_cache, set5_len);

        const uint8_t gp[8] = {0xA1, 0x01, 0x05, 0x03,
                                set5_iface, 0x00, 0x06, 0x00};
        max3421e_ctrl_rd(g_dev_addr, gp, feat5_cache, 6);
        set5_pending = false;
    }
}

/* ── LUFA USB Event Handlers ────────────────────────────────────────────── */

void EVENT_USB_Device_Connect(void) {}
void EVENT_USB_Device_Disconnect(void) {}

void EVENT_USB_Device_ConfigurationChanged(void)
{
    Endpoint_ConfigureEndpoint(MOUSE_IN_EPADDR,
                               EP_TYPE_INTERRUPT,
                               MOUSE_EPSIZE, 1);
    Endpoint_ConfigureEndpoint(KBD_IN_EPADDR,
                               EP_TYPE_INTERRUPT,
                               KBD_EPSIZE, 1);
    Endpoint_ConfigureEndpoint(VENDOR_IN_EPADDR,
                               EP_TYPE_INTERRUPT,
                               VENDOR_EPSIZE, 1);
}

/* ── HID class-specific control requests ────────────────────────────────── */

void EVENT_USB_Device_ControlRequest(void)
{
    if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_TYPE) != REQTYPE_CLASS)
        return;

    uint8_t iface = (uint8_t)(USB_ControlRequest.wIndex & 0xFF);
    if (iface > 2) return;

    switch (USB_ControlRequest.bRequest) {

    case HID_REQ_SetIdle:
        if (USB_ControlRequest.bmRequestType ==
            (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
        {
            Endpoint_ClearSETUP();
            idle_rate[iface] = (USB_ControlRequest.wValue >> 8);
            Endpoint_ClearStatusStage();
        }
        break;

    case HID_REQ_GetIdle:
        if (USB_ControlRequest.bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
        {
            Endpoint_ClearSETUP();
            while (!Endpoint_IsINReady());
            Endpoint_Write_8(idle_rate[iface]);
            Endpoint_ClearIN();
            Endpoint_ClearStatusStage();
        }
        break;

    case HID_REQ_SetProtocol:
        if (USB_ControlRequest.bmRequestType ==
            (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
        {
            Endpoint_ClearSETUP();
            Endpoint_ClearStatusStage();
        }
        break;

    case HID_REQ_GetProtocol:
        if (USB_ControlRequest.bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
        {
            Endpoint_ClearSETUP();
            while (!Endpoint_IsINReady());
            Endpoint_Write_8(0x01);
            Endpoint_ClearIN();
            Endpoint_ClearStatusStage();
        }
        break;

    case HID_REQ_SetReport:
        if (USB_ControlRequest.bmRequestType ==
            (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
        {
            uint8_t  rtype = (uint8_t)(USB_ControlRequest.wValue >> 8);
            uint8_t  rid   = (uint8_t)(USB_ControlRequest.wValue & 0xFF);
            uint16_t wlen  = USB_ControlRequest.wLength;

            /* ── Injection channel: Interface 2, Report ID 0x10 ── */
            if (iface == 2 && rtype == 0x03 && rid == 0x10 && wlen <= INJECT_REPORT_SIZE) {
                Endpoint_ClearSETUP();
                uint8_t ibuf[INJECT_REPORT_SIZE] = {0};
                Endpoint_Read_Control_Stream_LE(ibuf, wlen);
                Endpoint_ClearIN();
                /* The host may or may not include the Report ID byte.
                   Detect and skip if present. */
                uint8_t off = (wlen == INJECT_REPORT_SIZE && ibuf[0] == 0x10) ? 1 : 0;
                if (wlen - off >= 1) {
                    uint8_t cmd = ibuf[off];
                    if (cmd == 0x01) {
                        /* Command 0x01: movement (dx, dy) */
                        if (wlen - off >= 5) {
                            int16_t dx = (int16_t)((uint16_t)ibuf[off+1] | ((uint16_t)ibuf[off+2] << 8));
                            int16_t dy = (int16_t)((uint16_t)ibuf[off+3] | ((uint16_t)ibuf[off+4] << 8));
                            inject_dx = dx;
                            inject_dy = dy;
                            inject_pending = true;
                        }
                    } else if (cmd == 0x02) {
                        /* Command 0x02: set cooldown (ms) */
                        if (wlen - off >= 3) {
                            uint16_t ms = (uint16_t)(ibuf[off+1] | ((uint16_t)ibuf[off+2] << 8));
                            cooldown_ms = ms;
                        }
                    }
                }
            }
            /* ── Forward other reports (e.g., DPI via Report ID 0x11) ── */
            else if (rtype == 0x03 && rid == 0x04 && wlen <= sizeof(feat4_cache)) {
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(feat4_cache, wlen);
                Endpoint_ClearIN();
                set4_len   = wlen;
                set4_iface = iface;
                set4_pending = true;
            } else if (rtype == 0x03 && rid == 0x05 && wlen <= sizeof(feat5_cache)) {
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(feat5_cache, wlen);
                Endpoint_ClearIN();
                set5_len   = wlen;
                set5_iface = iface;
                set5_pending = true;
            }
        }
        break;

    case HID_REQ_GetReport:
        if (USB_ControlRequest.bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
        {
            uint8_t  rtype = (uint8_t)(USB_ControlRequest.wValue >> 8);
            uint8_t  rid   = (uint8_t)(USB_ControlRequest.wValue & 0xFF);
            uint16_t wlen  = USB_ControlRequest.wLength;

            Endpoint_ClearSETUP();

            /* Report cached injection state if requested (for debug) */
            if (iface == 2 && rtype == 0x03 && rid == 0x10) {
                uint8_t rbuf[INJECT_REPORT_SIZE] = {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                rbuf[1] = (uint8_t)(inject_dx & 0xFF);
                rbuf[2] = (uint8_t)((uint16_t)inject_dx >> 8);
                rbuf[3] = (uint8_t)(inject_dy & 0xFF);
                rbuf[4] = (uint8_t)((uint16_t)inject_dy >> 8);
                uint16_t n = (wlen < INJECT_REPORT_SIZE) ? wlen : INJECT_REPORT_SIZE;
                Endpoint_Write_Control_Stream_LE(rbuf, n);
                Endpoint_ClearOUT();
            } else if (rtype == 0x03 && rid == 0x04) {
                uint16_t n = (wlen < sizeof(feat4_cache))
                             ? wlen : sizeof(feat4_cache);
                Endpoint_Write_Control_Stream_LE(feat4_cache, n);
                Endpoint_ClearOUT();
            } else if (rtype == 0x03 && rid == 0x05) {
                uint16_t n = (wlen < sizeof(feat5_cache))
                             ? wlen : sizeof(feat5_cache);
                Endpoint_Write_Control_Stream_LE(feat5_cache, n);
                Endpoint_ClearOUT();
            } else {
                uint8_t zeros[8] = {0};
                uint8_t n = (wlen > 8) ? 8 : (uint8_t)wlen;
                Endpoint_Write_Control_Stream_LE(zeros, n);
                Endpoint_ClearOUT();
            }
        }
        break;

    default:
        break;
    }
}

/* ── Main ───────────────────────────────────────────────────────────────── */

int main(void)
{
    MCUSR &= ~(1 << WDRF);
    wdt_disable();
    clock_prescale_set(clock_div_1);

    LED_DDR |= (1 << LED_BIT);

    USB_Init();
    sei();

    max3421e_init();

    /* Let USB_USBTask run while the downstream mouse enumerates,
       so the PC can complete its enumeration of us in parallel. */
    if (g_dev_state == DEV_CONNECTED) {
        g_dev_state = DEV_ENUMERATING;
        _delay_ms(100);
        if (max3421e_enumerate())
            g_dev_state = DEV_ENUMERATED;
        else
            g_dev_state = DEV_DISCONNECTED;
    }

    /* Give the mouse a moment to settle, then pre-fetch feature reports
       while also servicing USB so the PC doesn't time out. */
    for (uint16_t i = 0; i < 200; i++) {
        USB_USBTask();
        _delay_ms(1);
    }

    if (g_dev_state == DEV_ENUMERATED) {
        prefetch_features();
    }

    uint8_t raw[64];    /* max packet size is 64 bytes */
    uint8_t rx_len;

    for (;;) {
        USB_USBTask();
        max3421e_task();

        if (USB_DeviceState == DEVICE_STATE_Configured)
            LED_ON();

        if (USB_DeviceState != DEVICE_STATE_Configured)
            continue;
        if (g_dev_state != DEV_ENUMERATED)
            continue;

        /* Re-fetch if first attempt was too early */
        if (!features_fetched)
            prefetch_features();

        /* Forward any pending SET_REPORTs and re-fetch fresh data */
        forward_pending_reports();

        /* ── Inject synthetic mouse move from usermode app ──
               Runs BEFORE real-mouse polling so it gets first access to EP1. */
        if (inject_pending) {
            Endpoint_SelectEndpoint(MOUSE_IN_EPADDR);
            if (Endpoint_IsINReady()) {
                uint32_t now = (uint32_t)millis();
                if ((now - last_p_time) >= cooldown_ms) {
                    int16_t dx = inject_dx;
                    int16_t dy = inject_dy;
                    inject_pending = false;
                    last_p_time = now;

                    /* Build 14-byte mouse report with real button state + injected movement */
                    uint8_t rpt[MOUSE_REPORT_SIZE];
                    rpt[0] = last_buttons[0];              /* buttons 1-8 */
                    rpt[1] = last_buttons[1];              /* buttons 9-16 */
                    rpt[2] = (uint8_t)(dx & 0xFF);
                    rpt[3] = (uint8_t)((uint16_t)dx >> 8);
                    rpt[4] = (uint8_t)(dy & 0xFF);
                    rpt[5] = (uint8_t)((uint16_t)dy >> 8);
                    rpt[6] = last_wheel;
                    rpt[7] = last_pan;
                    rpt[8] = last_vendor[0];
                    rpt[9] = last_vendor[1];
                    rpt[10] = last_vendor[2];
                    rpt[11] = last_vendor[3];
                    rpt[12] = last_vendor[4];
                    rpt[13] = 0x00; /* extra padding */

                    Endpoint_Write_Stream_LE(rpt, MOUSE_REPORT_SIZE, NULL);
                    Endpoint_ClearIN();
                } else {
                    /* cooldown active: drop this injection, but clear pending flag */
                    inject_pending = false;
                }
            }
        }

        /* ── Poll EP1 (mouse, Interface 0) ── */
        rx_len = 0;
        uint8_t r1 = max3421e_in_poll(g_dev_addr, 1, raw, &rx_len);
        if (r1 == hrSUCCESS && rx_len >= 14) {
            memcpy(g_mouse_report, raw, MOUSE_REPORT_SIZE);
            /* Extract button state (first 2 bytes) and vendor bytes (last 5) */
            last_buttons[0] = raw[0];
            last_buttons[1] = raw[1];
            if (rx_len >= 7) last_wheel = raw[6];
            if (rx_len >= 8) last_pan   = raw[7];
            if (rx_len >= 13) {
                last_vendor[0] = raw[8];
                last_vendor[1] = raw[9];
                last_vendor[2] = raw[10];
                last_vendor[3] = raw[11];
                last_vendor[4] = raw[12];
            }
            g_mouse_pending = true;
        }

        if (g_mouse_pending) {
            send_mouse_report();
            g_mouse_pending = false;
        }

        /* ── Poll EP2 (keyboard, Interface 1) ── */
        rx_len = 0;
        uint8_t r2 = max3421e_in_poll(g_dev_addr, 2, raw, &rx_len);
        if (r2 == hrSUCCESS && rx_len >= 9) {
            memcpy(g_kbd_report, raw, KBD_REPORT_SIZE);
            g_kbd_pending = true;
        }

        if (g_kbd_pending) {
            send_kbd_report();
            g_kbd_pending = false;
        }

        /* ── Poll EP3 (vendor, Interface 2) ──
               Forward everything untouched. The real mouse expects
               these reports for DPI, RGB, etc. */
        rx_len = 0;
        uint8_t r3 = max3421e_in_poll(g_dev_addr, 3, raw, &rx_len);
        if (r3 == hrSUCCESS && rx_len > 0) {
            /* Forward vendor reports to the host exactly as received */
            Endpoint_SelectEndpoint(VENDOR_IN_EPADDR);
            if (Endpoint_IsEnabled() && Endpoint_IsConfigured() && Endpoint_IsINReady()) {
                Endpoint_Write_Stream_LE(raw, rx_len, NULL);
                Endpoint_ClearIN();
            }
        }
    }
}