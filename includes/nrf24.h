#ifndef NRF_24_H
#define NRF_24_H

#include "stdint.h"

#define GPIO_NRF_CS 8
#define GPIO_NRF_CE 7

typedef enum {
    NRF_ADDRESS_WIDTH_3_BYTES = 0b01,
    NRF_ADDRESS_WIDTH_4_BYTES = 0b10,
    NRF_ADDRESS_WIDTH_5_BYTES = 0b11
} nrf_address_width_t;

typedef enum {
    NRF_DATA_RATE_1MBps   = 0b000,
    NRF_DATA_RATE_2MBps   = 0b001,
    NRF_DATA_RATE_250kbps = 0b100
} nrf_data_rate_t;

typedef enum {
    NRF_OUTPUT_POWER_neg18dBm = 0b00,
    NRF_OUTPUT_POWER_neg12dBm = 0b01,
    NRF_OUTPUT_POWER_neg6dBm  = 0b10,
    NRF_OUTPUT_POWER_0dBm     = 0b11
} nrf_output_power_t;

typedef enum {
    NRF_CRC_SCHEME_1_BYTES = 0,
    NRF_CRC_SCHEME_2_BYTES = 1,
    NRF_CRC_DISABLED       = 2,
} nrf_crc_encoding_scheme_t;

typedef struct {
    nrf_address_width_t        address_width;
    uint8_t                    channel;                   // 0 -> 125
    nrf_data_rate_t            data_rate;
    nrf_output_power_t         output_power;
    uint16_t                   auto_retransmit_delay_us; // Multiple of 250: valid: 250 -> 4000
    uint8_t                    auto_retransmit_count;    // 0 disabled. valid: 1 -> 15
    uint8_t                    enable_crc;               // 1: Enabled, 0: Disabled
    nrf_crc_encoding_scheme_t  crc_encoding_scheme;      // '0': 1 byte, '1' 2 bytes
    uint8_t                    power_up;                 // '1': Power up, '0' Power down
    uint8_t                    address_rx[5];
    uint8_t                    address_tx[5];
} nrf_settings_t;

void set_nrf_default_settings(nrf_settings_t* settings);

int nrf_init();

int nrf_write_data(const uint8_t* data, uint8_t len);

uint8_t nrf_read(uint8_t* buf);

void test();

#endif /* NRF_24_H */
