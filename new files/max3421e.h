/*
 * max3421e.h
 * MAX3421E USB Host Controller driver – registers, SPI functions,
 * and device state management.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>

/* ── SPI pin assignments for Arduino Leonardo + USB Host Shield 2.0 ──────
 *
 *  Hardware SPI (ATmega32U4 ICSP header):
 *    SCK   PB1  (Arduino pin 15 / ICSP-3)
 *    MOSI  PB2  (Arduino pin 16 / ICSP-4)
 *    MISO  PB3  (Arduino pin 14 / ICSP-1)
 *
 *  MAX3421E control lines:
 *    CS    PB6  (Arduino pin 10)
 *    INT   PB5  (Arduino pin  9) – polled, active-low
 *    RST   PE6  (Arduino pin  7) – active-low, optional
 * ─────────────────────────────────────────────────────────────────────── */
#define MAX_CS_DDR   DDRB
#define MAX_CS_PORT  PORTB
#define MAX_CS_BIT   PB6

#define MAX_INT_DDR  DDRB
#define MAX_INT_PIN  PINB
#define MAX_INT_BIT  PB5

#define MAX_RST_DDR  DDRE
#define MAX_RST_PORT PORTE
#define MAX_RST_BIT  PE6

/* Hardware SPI SS must be output to stay in master mode */
#define SPI_SS_DDR   DDRB
#define SPI_SS_BIT   PB0

#define MAX_CS_LO()  (MAX_CS_PORT &= ~(1 << MAX_CS_BIT))
#define MAX_CS_HI()  (MAX_CS_PORT |=  (1 << MAX_CS_BIT))
#define MAX_INT_RD() (MAX_INT_PIN  &   (1 << MAX_INT_BIT))

/* ── Register addresses (pre-shifted: addr = register_number << 3) ─────── */
#define rRCVFIFO  0x08   /* Receive FIFO                 reg  1 */
#define rSNDFIFO  0x10   /* Send FIFO                    reg  2 */
#define rSUDFIFO  0x20   /* Setup Data FIFO              reg  4 */
#define rRCVBC    0x30   /* Receive Byte Count           reg  6 */
#define rSNDBC    0x38   /* Send Byte Count              reg  7 */
#define rUSBIRQ   0x68   /* USB Interrupt Request        reg 13 */
#define rUSBIEN   0x70   /* USB Interrupt Enable         reg 14 */
#define rUSBCTL   0x78   /* USB Control                  reg 15 */
#define rCPUCTL   0x80   /* CPU Control                  reg 16 */
#define rPINCTL   0x88   /* Pin Control                  reg 17 */
#define rREVISION 0x90   /* Chip Revision                reg 18 */
#define rHIRQ     0xC8   /* Host Interrupt Request       reg 25 */
#define rHIEN     0xD0   /* Host Interrupt Enable        reg 26 */
#define rMODE     0xD8   /* Mode                         reg 27 */
#define rPERADDR  0xE0   /* Peripheral Address           reg 28 */
#define rHCTL     0xE8   /* Host Control                 reg 29 */
#define rHXFR     0xF0   /* Host Transfer                reg 30 */
#define rHRSL     0xF8   /* Host Result                  reg 31 */

/* SPI direction bit (ORed with register address for writes) */
#define MAX_WR    0x02

/* ── MODE register bits ─────────────────────────────────────────────────── */
#define bmHOST      0x01
#define bmLOWSPEED  0x02
#define bmHUBPRE    0x04
#define bmSOFKAENAB 0x08
#define bmSEPIRQ    0x10
#define bmDELAYISO  0x20
#define bmDMPULLDN  0x40
#define bmDPPULLDN  0x80

/* ── HIRQ register bits ─────────────────────────────────────────────────── */
#define bmBUSEVENTIRQ 0x01
#define bmRWUIRQ      0x02
#define bmRCVDAVIRQ   0x04
#define bmSNDBAVIRQ   0x08
#define bmSUSDNIRQ    0x10
#define bmCONDETIRQ   0x20
#define bmFRAMEIRQ    0x40
#define bmHXFRDNIRQ   0x80

/* ── HCTL register bits ─────────────────────────────────────────────────── */
#define bmBUSRST    0x01
#define bmFRMRST    0x02
#define bmSAMPLEBUS 0x04
#define bmSIGRSF    0x08
#define bmRCVTOG0   0x10
#define bmRCVTOG1   0x20
#define bmSNDTOG0   0x40
#define bmSNDTOG1   0x80

/* ── HRSL register ──────────────────────────────────────────────────────── */
#define bmRCVTOGRD  0x10
#define bmSNDTOGRD  0x20
#define bmKSTATUS   0x40
#define bmJSTATUS   0x80
#define HRSLT_MASK  0x0F

/* Result codes */
#define hrSUCCESS  0x00
#define hrBUSY     0x01
#define hrBADREQ   0x02
#define hrUNDEF    0x03
#define hrNAK      0x04
#define hrSTALL    0x05
#define hrTOGERR   0x06
#define hrWRONGPID 0x07
#define hrBADBC    0x08
#define hrPIDERR   0x09
#define hrPKTERR   0x0A
#define hrCRCERR   0x0B
#define hrKERR     0x0C
#define hrJERR     0x0D
#define hrTIMEOUT  0x0E
#define hrBABBLE   0x0F

/* ── USBCTL register bits ───────────────────────────────────────────────── */
#define bmCHIPRES  0x20
#define bmPWRDOWN  0x10

/* ── USBIRQ register bits ───────────────────────────────────────────────── */
#define bmVBUSIRQ   0x40   /* b6 */
#define bmNOVBUSIRQ 0x20   /* b5 */
#define bmOSCOKIRQ  0x01   /* b0 */

/* ── PINCTL register bits ───────────────────────────────────────────────── */
#define bmFDUPSPI   0x10
#define bmINTLEVEL  0x08
#define bmPOSINT    0x04

/* ── Transfer token types (written to rHXFR = token | endpoint_num) ─────── */
#define tokIN     0x00   /* IN  token */
#define tokSETUP  0x10   /* SETUP token */
#define tokOUT    0x20   /* OUT token */
#define tokINHS   0x80   /* IN  + handshake (status stage after OUT data) */
#define tokOUTHS  0xA0   /* OUT + handshake (status stage after IN data) */

/* ── USB Standard Request fields ────────────────────────────────────────── */
#define USB_SETUP_HOST_TO_DEV   0x00
#define USB_SETUP_DEV_TO_HOST   0x80
#define USB_SETUP_TYPE_STD      0x00
#define USB_SETUP_TYPE_CLASS    0x20
#define USB_SETUP_TYPE_VENDOR   0x40
#define USB_SETUP_RCPT_DEV      0x00
#define USB_SETUP_RCPT_IFACE    0x01
#define USB_SETUP_RCPT_EP       0x02

#define USB_REQ_GET_DESCRIPTOR   0x06
#define USB_REQ_SET_ADDRESS      0x05
#define USB_REQ_SET_CONFIGURATION 0x09
#define HID_REQ_SET_REPORT       0x09
#define HID_REQ_GET_REPORT       0x01

#define USB_DESC_DEVICE          0x01
#define USB_DESC_CONFIG          0x02

/* ── Device state machine ───────────────────────────────────────────────── */
typedef enum {
    DEV_DISCONNECTED = 0,
    DEV_CONNECTED,      /* bus settled, not yet enumerated */
    DEV_ENUMERATING,    /* enumeration in progress – ignore CONDET */
    DEV_ENUMERATED,     /* address set, config active */
} DeviceState_t;

extern volatile DeviceState_t g_dev_state;
extern uint8_t g_dev_addr;   /* USB address assigned to the downstream mouse */

/* ── Public API ─────────────────────────────────────────────────────────── */
void    max3421e_init(void);
uint8_t max3421e_reg_rd(uint8_t reg);
void    max3421e_reg_wr(uint8_t reg, uint8_t val);
void    max3421e_bytes_rd(uint8_t reg, uint8_t n, uint8_t *dst);
void    max3421e_bytes_wr(uint8_t reg, uint8_t n, const uint8_t *src);

/* Wait for HXFRDN and return HRSL result nibble (0x00–0x0F) */
uint8_t max3421e_dispatch(uint8_t token_ep);

/* USB control transfer helpers (uint16_t length for large Feature reports) */
uint8_t max3421e_ctrl_rd(uint8_t addr, const uint8_t setup[8],
                          uint8_t *buf, uint16_t len);
uint8_t max3421e_ctrl_wr(uint8_t addr, const uint8_t setup[8],
                          const uint8_t *data, uint16_t len);
uint8_t max3421e_ctrl_nodata(uint8_t addr, const uint8_t setup[8]);

/* Interrupt IN poll – returns hrSUCCESS or hrNAK or error code */
uint8_t max3421e_in_poll(uint8_t addr, uint8_t ep,
                          uint8_t *buf, uint8_t *out_len);

/* Enumerate the downstream device and put it in DEV_ENUMERATED state.
   Returns true on success. */
bool    max3421e_enumerate(void);

/* Called from main loop – handles connect/disconnect events */
void    max3421e_task(void);