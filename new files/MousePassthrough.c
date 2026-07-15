#include "max3421e.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

volatile DeviceState_t g_dev_state = DEV_DISCONNECTED;
uint8_t g_dev_addr = 0;

/* Per-endpoint receive data toggle.  The MAX3421E has ONE shared toggle
   register, so we must save/restore it when switching between endpoints. */
static uint8_t ep_rcvtog[4];  /* 0=DATA0, 1=DATA1 for EP 0..3 */

/* ── SPI primitives ─────────────────────────────────────────────────────── */

static uint8_t spi_byte(uint8_t b)
{
    SPDR = b;
    while (!(SPSR & (1 << SPIF)));
    return SPDR;
}

/* ── Register I/O ───────────────────────────────────────────────────────── */

uint8_t max3421e_reg_rd(uint8_t reg)
{
    MAX_CS_LO();
    spi_byte(reg);
    uint8_t v = spi_byte(0);
    MAX_CS_HI();
    return v;
}

void max3421e_reg_wr(uint8_t reg, uint8_t val)
{
    MAX_CS_LO();
    spi_byte(reg | MAX_WR);
    spi_byte(val);
    MAX_CS_HI();
}

void max3421e_bytes_rd(uint8_t reg, uint8_t n, uint8_t *dst)
{
    MAX_CS_LO();
    spi_byte(reg);
    for (uint8_t i = 0; i < n; i++)
        dst[i] = spi_byte(0);
    MAX_CS_HI();
}

void max3421e_bytes_wr(uint8_t reg, uint8_t n, const uint8_t *src)
{
    MAX_CS_LO();
    spi_byte(reg | MAX_WR);
    for (uint8_t i = 0; i < n; i++)
        spi_byte(src[i]);
    MAX_CS_HI();
}

/* ── Init ───────────────────────────────────────────────────────────────── */

void max3421e_init(void)
{
    /* SPI pins */
    SPI_SS_DDR  |=  (1 << SPI_SS_BIT);
    DDRB        |=  (1 << PB1) | (1 << PB2);
    DDRB        &= ~(1 << PB3);
    PORTB       |=  (1 << SPI_SS_BIT);

    /* MAX3421E control lines */
    MAX_CS_DDR  |=  (1 << MAX_CS_BIT);
    MAX_CS_HI();
    MAX_INT_DDR &= ~(1 << MAX_INT_BIT);
    MAX_RST_DDR |=  (1 << MAX_RST_BIT);
    MAX_RST_PORT |= (1 << MAX_RST_BIT);

    /* SPI master, mode 0, fosc/4 */
    SPCR = (1 << SPE) | (1 << MSTR);
    SPSR = 0;

    /* Chip reset */
    max3421e_reg_wr(rUSBCTL, bmCHIPRES);
    _delay_ms(5);
    max3421e_reg_wr(rUSBCTL, 0x00);

    /* Wait for oscillator (~10 ms timeout) */
    for (uint16_t t = 0; t < 10000; t++) {
        if (max3421e_reg_rd(rUSBIRQ) & bmOSCOKIRQ) break;
        _delay_us(1);
    }

    /* Full-duplex SPI, active-low INT level */
    max3421e_reg_wr(rPINCTL, bmFDUPSPI | bmINTLEVEL);

    /* Host mode with D+/D- pull-downs, SOF generation ON */
    max3421e_reg_wr(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST | bmSOFKAENAB);

    /* Clear all host IRQs */
    max3421e_reg_wr(rHIRQ, 0xFF);

    /* Wait a bit for bus to settle, then check if device already attached */
    _delay_ms(100);
    max3421e_reg_wr(rHCTL, bmSAMPLEBUS);
    _delay_ms(2);
    uint8_t hrsl = max3421e_reg_rd(rHRSL);
    if (hrsl & (bmJSTATUS | bmKSTATUS))
        g_dev_state = DEV_CONNECTED;
}

/* ── Transfer dispatcher (with timeout) ─────────────────────────────────── */

uint8_t max3421e_dispatch(uint8_t token_ep)
{
    max3421e_reg_wr(rHXFR, token_ep);
    for (uint16_t t = 0; t < 20000; t++) {
        if (max3421e_reg_rd(rHIRQ) & bmHXFRDNIRQ) {
            max3421e_reg_wr(rHIRQ, bmHXFRDNIRQ);
            return (max3421e_reg_rd(rHRSL) & HRSLT_MASK);
        }
        _delay_us(1);
    }
    return hrTIMEOUT;
}

/* ── Retry dispatch with NAK handling ───────────────────────────────────── */

static uint8_t dispatch_retry(uint8_t token_ep, uint16_t nak_limit)
{
    uint8_t r;
    uint16_t n = 0;
    do {
        r = max3421e_dispatch(token_ep);
        if (r == hrNAK) {
            if (nak_limit && ++n >= nak_limit)
                return hrNAK;
        }
    } while (r == hrNAK);
    return r;
}

/* ── Control transfer – read (host←device) ──────────────────────────────── */

uint8_t max3421e_ctrl_rd(uint8_t addr, const uint8_t setup[8],
                          uint8_t *buf, uint16_t buflen)
{
    max3421e_reg_wr(rPERADDR, addr);
    max3421e_bytes_wr(rSUDFIFO, 8, setup);

    uint8_t r = dispatch_retry(tokSETUP, 200);
    if (r != hrSUCCESS) return r;

    max3421e_reg_wr(rHCTL, bmRCVTOG1);

    uint16_t total = 0;
    while (total < buflen) {
        r = dispatch_retry(tokIN, 200);
        if (r == hrTOGERR) {
            uint8_t hrsl = max3421e_reg_rd(rHRSL);
            max3421e_reg_wr(rHCTL,
                (hrsl & bmRCVTOGRD) ? bmRCVTOG1 : bmRCVTOG0);
            continue;
        }
        if (r != hrSUCCESS) return r;

        uint8_t n = max3421e_reg_rd(rRCVBC);
        if (n > (buflen - total)) n = (uint8_t)(buflen - total);
        max3421e_bytes_rd(rRCVFIFO, n, buf + total);
        max3421e_reg_wr(rHIRQ, bmRCVDAVIRQ);
        total += n;
        if (n < 8) break;
    }

    max3421e_reg_wr(rHCTL, bmSNDTOG1);
    max3421e_reg_wr(rSNDBC, 0);
    r = dispatch_retry(tokOUTHS, 200);
    return r;
}

/* ── Control transfer – write (host→device) ─────────────────────────────── */

uint8_t max3421e_ctrl_wr(uint8_t addr, const uint8_t setup[8],
                          const uint8_t *data, uint16_t len)
{
    max3421e_reg_wr(rPERADDR, addr);
    max3421e_bytes_wr(rSUDFIFO, 8, setup);

    uint8_t r = dispatch_retry(tokSETUP, 200);
    if (r != hrSUCCESS) return r;

    max3421e_reg_wr(rHCTL, bmSNDTOG1);

    uint16_t sent = 0;
    while (sent < len) {
        uint8_t chunk = ((len - sent) > 8) ? 8 : (uint8_t)(len - sent);
        max3421e_bytes_wr(rSNDFIFO, chunk, data + sent);
        max3421e_reg_wr(rSNDBC, chunk);
        r = dispatch_retry(tokOUT, 200);
        if (r != hrSUCCESS) return r;
        sent += chunk;
    }

    r = dispatch_retry(tokINHS, 200);
    return r;
}

/* ── Control transfer – no data stage ───────────────────────────────────── */

uint8_t max3421e_ctrl_nodata(uint8_t addr, const uint8_t setup[8])
{
    max3421e_reg_wr(rPERADDR, addr);
    max3421e_bytes_wr(rSUDFIFO, 8, setup);

    uint8_t r = dispatch_retry(tokSETUP, 200);
    if (r != hrSUCCESS) return r;

    max3421e_reg_wr(rHCTL, bmRCVTOG1);
    r = dispatch_retry(tokINHS, 200);
    return r;
}

/* ── Interrupt IN poll with per-endpoint toggle tracking ─────────────────── */

uint8_t max3421e_in_poll(uint8_t addr, uint8_t ep,
                          uint8_t *buf, uint8_t *out_len)
{
    if (ep > 3) return hrBADREQ;

    max3421e_reg_wr(rPERADDR, addr);

    /* Restore this endpoint's saved toggle before the transfer */
    max3421e_reg_wr(rHCTL, ep_rcvtog[ep] ? bmRCVTOG1 : bmRCVTOG0);

    uint8_t r = max3421e_dispatch(tokIN | (ep & 0x0F));

    if (r == hrSUCCESS) {
        uint8_t n = max3421e_reg_rd(rRCVBC);
        if (n > 8) n = 8;
        max3421e_bytes_rd(rRCVFIFO, n, buf);
        max3421e_reg_wr(rHIRQ, bmRCVDAVIRQ);
        *out_len = n;
        /* Successful transfer flips the toggle for next time */
        ep_rcvtog[ep] ^= 1;
    } else if (r == hrTOGERR) {
        /* Our saved toggle was wrong — flip it so the next attempt works */
        ep_rcvtog[ep] ^= 1;
    }

    return r;
}

/* ── Enumeration ────────────────────────────────────────────────────────── */

bool max3421e_enumerate(void)
{
    uint8_t r;
    uint8_t buf[18];

    /* Clear all pending IRQs */
    max3421e_reg_wr(rHIRQ, 0xFF);

    /* USB bus reset: write BUSRST bit — it self-clears when done */
    max3421e_reg_wr(rHCTL, bmBUSRST);
    for (uint16_t t = 0; t < 1000; t++) {
        if (!(max3421e_reg_rd(rHCTL) & bmBUSRST)) break;
        _delay_ms(1);
    }

    /* Re-enable SOF generation (bus reset disables it) */
    max3421e_reg_wr(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST | bmSOFKAENAB);

    /* Wait for first SOF frame to actually be sent */
    max3421e_reg_wr(rHIRQ, bmFRAMEIRQ);
    for (uint16_t t = 0; t < 200; t++) {
        if (max3421e_reg_rd(rHIRQ) & bmFRAMEIRQ) break;
        _delay_ms(1);
    }

    /* Clear any CONDET from the bus reset */
    max3421e_reg_wr(rHIRQ, bmCONDETIRQ);

    /* Let device settle after reset */
    _delay_ms(200);

    /* Reset all saved toggles */
    memset(ep_rcvtog, 0, sizeof(ep_rcvtog));

    /* ── GET_DESCRIPTOR Device (first 8 bytes, address 0) ────────────── */
    const uint8_t get_dev8[8] = {
        0x80, USB_REQ_GET_DESCRIPTOR,
        0x00, USB_DESC_DEVICE,
        0x00, 0x00,
        0x08, 0x00
    };
    r = max3421e_ctrl_rd(0, get_dev8, buf, 8);
    if (r != hrSUCCESS) return false;

    /* ── SET_ADDRESS 1 ───────────────────────────────────────────────── */
    const uint8_t set_addr[8] = {
        0x00, USB_REQ_SET_ADDRESS,
        0x01, 0x00,
        0x00, 0x00,
        0x00, 0x00
    };
    r = max3421e_ctrl_nodata(0, set_addr);
    if (r != hrSUCCESS) return false;
    _delay_ms(20);   /* generous delay for address change */
    g_dev_addr = 1;

    /* ── SET_CONFIGURATION 1 ─────────────────────────────────────────── */
    const uint8_t set_cfg[8] = {
        0x00, USB_REQ_SET_CONFIGURATION,
        0x01, 0x00,
        0x00, 0x00,
        0x00, 0x00
    };
    r = max3421e_ctrl_nodata(g_dev_addr, set_cfg);
    if (r != hrSUCCESS) return false;
    _delay_ms(10);

    /* Reset endpoint toggles to DATA0 after configuration */
    memset(ep_rcvtog, 0, sizeof(ep_rcvtog));

    g_dev_state = DEV_ENUMERATED;
    return true;
}

/* ── Main-loop task: handle connect / disconnect ─────────────────────────── */

void max3421e_task(void)
{
    uint8_t hirq = max3421e_reg_rd(rHIRQ);
    if (!(hirq & bmCONDETIRQ)) return;

    max3421e_reg_wr(rHIRQ, bmCONDETIRQ);

    if (g_dev_state == DEV_ENUMERATING) return;

    max3421e_reg_wr(rHCTL, bmSAMPLEBUS);
    _delay_ms(2);
    uint8_t hrsl     = max3421e_reg_rd(rHRSL);
    bool    attached = (hrsl & (bmJSTATUS | bmKSTATUS)) != 0;

    if (attached) {
        if (g_dev_state == DEV_DISCONNECTED) {
            g_dev_state = DEV_ENUMERATING;
            _delay_ms(200);
            if (max3421e_enumerate()) {
                g_dev_state = DEV_ENUMERATED;
            } else {
                g_dev_state = DEV_DISCONNECTED;
            }
        }
    } else {
        g_dev_state = DEV_DISCONNECTED;
        g_dev_addr  = 0;
    }
}