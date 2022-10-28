#include "core.h"
#include "machine.h"

#define SPI_BIT_BANG


uint8_t spi_init() {
    #ifndef SPI_BIT_BANG
    SPI0CFG = SPI0CFG_MSTEN__MASTER_ENABLED |       // Enable master
              SPI0CFG_CKPHA__DATA_CENTERED_FIRST |  // Data on first clock pulse
              SPI0CFG_CKPOL__IDLE_LOW;              // Clock idle LOW
    SPI0CN0 = SPI0CN0_SPIEN__ENABLED;               // Enable SPI module

    // fsck = SYSCLK / (2 * (SPI0CKR + 1))
    // SYSCLK = 24.5MHz -> SPICKR = 121 -> ~100kHz
    SPI0CKR = 121;                                  // Set clock to 100 kHZ

    // Set GPIO
    //PIN_SCK
    //PIN_MOSI
    //PIN_MISO
    //PIN_NRF_CE
    //PIN_NRF_CS

    XBR0 |= XBR0_SPI0E__ENABLED;
    #endif



    return 0;
}

#ifndef SPI_BIT_BANG

uint8_t spi_exchange(const uint8_t data) {
    // 1. Write the data to be sent to SPInDAT. The transfer will begin on the bus at this time.
    SPI0DAT = data;

    // 2. Wait for the SPIF flag to generate an interrupt, or poll SPIF until it is set to 1.
    while (!SPI0CN0_SPIF);

    // 3. Read the received data from SPInDAT.
    uint8_t ret = SPI0DAT;

    // 4. Clear the SPIF flag to 0.
    SPI0CN0_SPIF = 0;

    // 5. Repeat the sequence for any additional transfers.
    return ret;
}


uint8_t spi_send(const uint8_t data) {
    return spi_exchange(data);
}

#endif

#ifdef SPI_BIT_BANG


#define HIGH 1
#define LOW  0
#define ce(value)   (PIN_NRF_CE = value)
#define cs(value)   (PIN_NRF_CS = value)
#define sck(value)  (PIN_SCK = value)
#define mosi(value) (PIN_MOSI = value)
#define miso()       PIN_MISO

static uint8_t i;


#define wait_half_period() \
    i = 11; \
    while (i-- > 0) { \
     __asm__("nop\n"); \
    }


uint8_t spi_bit_bang_exchange_byte(const uint8_t data) {
    cli();
    uint8_t ret = 0;
    for (int bit = 7; bit >= 0; bit--) {
        mosi(data & (1 << bit));
        wait_half_period();
        sck(HIGH);
        ret |= (miso() << bit);
        wait_half_period();
        sck(LOW);
    }
    sei();
    return ret;
}

// --- Public API --- //
uint8_t spi_exchange(const uint8_t data) {
    return spi_bit_bang_exchange_byte(data);
}

uint8_t spi_send(const uint8_t data) {
    return spi_exchange(data);
}


#endif
