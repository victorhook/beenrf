#include "nrf24.h"
#include "core.h"
#include "spi.h"
#include "led.h"

// Commands
#define NRF_COMMAND_R_REGISTER          0b000      // <- 3 MSB; 5 LSB are: AAAAA address
#define NRF_COMMAND_W_REGISTER          0b001      // <- 3 MSB; 5 LSB are: AAAAA address
#define NRF_COMMAND_R_RX_PAYLOAD        0b01100001
#define NRF_COMMAND_W_TX_PAYLOAD        0b10100000
#define NRF_COMMAND_W_ACK_PAYLOAD       0b10101    // <- 5 MSB; 3 LSB are: PPP for pipe
#define NRF_COMMAND_W_TX_PAYLOAD_NO_ACK 0b1011000
#define NRF_COMMAND_FLUSH_TX            0b11100001
#define NRF_COMMAND_FLUSH_RX            0b11100010
#define NRF_COMMAND_REUSE_TX_PL         0b11100011
#define NRF_COMMAND_ACTIVATE            0b01010000
#define NRF_COMMAND_R_RX_PL_WIDa        0b01100000
#define NRF_COMMAND_NOP                 0b11111111

// Registers
#define NRF_REGISTER_CONFIG             0x00
#define NRF_REGISTER_EN_AA              0x01
#define NRF_REGISTER_EN_RXADDR          0x02
#define NRF_REGISTER_SETUP_AW           0x03
#define NRF_REGISTER_SETUP_RETR         0x04
#define NRF_REGISTER_RF_CH              0x05
#define NRF_REGISTER_RF_SETUP           0x06
#define NRF_REGISTER_STATUS             0x07
#define NRF_REGISTER_OBSERVE_TX         0x08
#define NRF_REGISTER_CD                 0x09
#define NRF_REGISTER_RX_ADDR_P0         0x0A
#define NRF_REGISTER_RX_ADDR_P1         0x0B
#define NRF_REGISTER_RX_ADDR_P2         0x0C
#define NRF_REGISTER_RX_ADDR_P3         0x0D
#define NRF_REGISTER_RX_ADDR_P4         0x0E
#define NRF_REGISTER_RX_ADDR_P5         0x0F
#define NRF_REGISTER_TX_ADDR            0x10
#define NRF_REGISTER_RX_PW_P0           0x11
#define NRF_REGISTER_RX_PW_P1           0x12
#define NRF_REGISTER_RX_PW_P2           0x13
#define NRF_REGISTER_RX_PW_P3           0x14
#define NRF_REGISTER_RX_PW_P4           0x15
#define NRF_REGISTER_RX_PW_P5           0x16
#define NRF_REGISTER_FIFO_STATUS        0x17
#define NRF_REGISTER_DYNPD              0x1C
#define NRF_REGISTER_FEATURE            0x1D

#define DEFAULT_NRF_ADDRESS 0xe7e7e7e7e7

// Bits
#define NRF_PRIM_RX_BIT 0
#define NRF_PWR_UP_BIT  1

//#define cs(value) digitalWrite(GPIO_NRF_CS, value)
//#define ce(value) digitalWrite(GPIO_NRF_CE, value)

#define ce(value)  (PIN_NRF_CE = value)
#define cs(value)  (PIN_NRF_CS = value)

static uint8_t _i = 0;
// Temporary microsecond delays
#define delay_us_150() \
  _i = 80; 			   \
  while (_i-- > 0) {   \
    __asm__("nop\n");  \
  }

#define delay_us_15() \
  _i = 35; 			  \
  while (_i-- > 0) {  \
    __asm__("nop\n"); \
  }


typedef struct {
  uint8_t TX_FULL : 1;
  uint8_t RX_P_NO : 3;
  uint8_t MAX_RT  : 1;
  uint8_t TX_DS   : 1;
  uint8_t RX_DR   : 1;
  uint8_t reserved: 1;
} status_reg_t;


static uint8_t spi_write(uint8_t cmd, uint8_t* data, uint8_t len)
{
  cs(LOW);
  uint8_t status = spi_exchange(cmd);
  int sent = 0;
  while (sent < len) {
    spi_exchange(data[sent]);
    sent++;
  }
  cs(HIGH);
  return status;
}

static uint8_t spi_read(uint8_t cmd, uint8_t* buf, uint8_t len)
{
  cs(LOW);
  uint8_t status = spi_exchange(cmd);
  int read = 0;
  while (read < len) {
    buf[read] = spi_exchange(0);
    read++;
  }
  cs(HIGH);
  return status;
}

// --- NRF Commands --- //

uint8_t write_register(uint8_t address, uint8_t* data, uint8_t len)
{
  uint8_t cmd = (NRF_COMMAND_W_REGISTER << 5) | address;
  return spi_write(cmd, data, len);
}

uint8_t read_register(uint8_t address, uint8_t* buf, uint8_t len)
{
  uint8_t cmd = (NRF_COMMAND_R_REGISTER << 5) | address;
  return spi_read(cmd, buf, len);
}

uint8_t nrf_nop()
{
  return spi_write(NRF_COMMAND_NOP, 0, 0);
}

uint8_t rx_payload_width(uint8_t* width)
{
  return spi_read(NRF_COMMAND_R_RX_PL_WIDa, width, 1);
}

uint8_t flush_tx()
{
  return spi_write(NRF_COMMAND_FLUSH_TX, 0, 0);
}

uint8_t flush_rx()
{
  return spi_write(NRF_COMMAND_FLUSH_RX, 0, 0);
}

uint8_t read_payload(uint8_t* buf, uint8_t len)
{
  return spi_read(NRF_COMMAND_R_RX_PAYLOAD, buf, len);
}

uint8_t write_payload(uint8_t* data, uint8_t len)
{
  return spi_write(NRF_COMMAND_W_TX_PAYLOAD, data, len);
}

uint8_t write_payload_no_ack(uint8_t* data, uint8_t len)
{
  return spi_write(NRF_COMMAND_W_TX_PAYLOAD_NO_ACK, data, len);
}

uint8_t write_payload_ack(uint8_t* data, uint8_t len, uint8_t pipe)
{
  uint8_t cmd = (NRF_COMMAND_W_ACK_PAYLOAD << 5) | pipe;
  return spi_write(cmd, data, len);
}

// --- Utility functions --- //

static uint8_t set_rx_address(uint8_t pipe, uint8_t* address, uint8_t len)
{
  // Pipe 0 can be any 5 bytes.
  // Pipe 1-5 must have the same first 4 bytes as Pipe 0, but the unique LSB.
  return write_register(NRF_REGISTER_RX_ADDR_P0 + pipe, address, len);
}

static uint8_t set_tx_address(uint8_t* address, uint8_t len)
{
  // The address of TX should be the same as pipe 0.
  return write_register(NRF_REGISTER_TX_ADDR, address, len);
}

static uint8_t fifo_status(uint8_t* fifo)
{
  return read_register(NRF_REGISTER_FIFO_STATUS, fifo, 1);
}

static uint8_t set_tx()
{
  // 1. Set the configuration bit PRIM_RX low.
  uint8_t config;
  read_register(NRF_REGISTER_CONFIG, &config, 1);
  config |= (1 << NRF_PWR_UP_BIT); // Ensure we're in PWR_UP state
  config &= ~(1 << NRF_PRIM_RX_BIT);
  return write_register(NRF_REGISTER_CONFIG, &config, 1);
}

static uint8_t data_ready()
{
  #define RX_DR 6
  return nrf_nop() & (1 << RX_DR);
}

static uint8_t transmit()
{
  ce(HIGH);
  delay_us_15();
  ce(LOW);
  return 0;
}

// --- Setting helper functions --- /
static void nrf24_set_address_width(nrf_address_width_t address_width) {
  write_register(NRF_REGISTER_SETUP_AW, &address_width, 1);
}
static void nrf24_set_channel(uint8_t channel) {
  write_register(NRF_REGISTER_RF_CH, &channel, 1);
}
static void nrf24_set_data_rate(nrf_data_rate_t data_rate) {
  uint8_t buf;
  read_register(NRF_REGISTER_RF_SETUP, &buf, 1);
  buf &= ~(0b111 << 3);  // Clear bits: RF_DR_LOW, PLL_LOCK (used only in test), RF_DR_LOW
  buf |= (data_rate << 3);
  write_register(NRF_REGISTER_RF_SETUP, &buf, 1);
}
static void nrf24_set_auto_ack(bool auto_ack) {
  uint8_t buf = auto_ack ? 0b00111111 : 0;
  write_register(NRF_REGISTER_EN_AA, &buf, 1);
}
static void nrf24_set_enable_dynamic_payloads(bool dynamic_payload) {
  uint8_t buf;
  read_register(NRF_REGISTER_FEATURE, &buf, 1);
  buf &= ~(1 << 2);  // Clear EN_DPL bit
  buf |= (dynamic_payload << 2);
  write_register(NRF_REGISTER_FEATURE, &buf, 1);
  buf = dynamic_payload ? 0b00111111 : 0;
  write_register(NRF_REGISTER_DYNPD, &buf, 1);
}
/* Requires auto-ack and dynamic payloads features */
static void nrf24_set_enable_ack_payload(bool ack_payload) {
  uint8_t buf;
  read_register(NRF_REGISTER_FEATURE, &buf, 1);
  buf &= ~(1 << 1);  // Clear EN_ACK_PAY bit
  buf |= (ack_payload << 1);
  write_register(NRF_REGISTER_FEATURE, &buf, 1);
}
/* Only if the dynamic payloads feature is disabled -- it is disabled by default */
static void nrf24_set_payload_size(uint8_t payload_size) {
  for (uint8_t pipe = 0; pipe < 5; pipe++) {
    write_register(NRF_REGISTER_RX_PW_P0+pipe, &payload_size, 1);
  }
}
static void nrf24_set_output_power(nrf_output_power_t power) {
  uint8_t buf;
  read_register(NRF_REGISTER_RF_SETUP, &buf, 1);
  buf &= ~(0b11 << 1);  // Clear RF_PWR bits
  buf |= (power << 1);
  write_register(NRF_REGISTER_RF_SETUP, &buf, 1);
}
/* The auto-ack feature automatically enables CRC because it is required */
static void nrf24_set_crc_length(nrf_crc_encoding_scheme_t crc_len) {
  uint8_t buf;
  read_register(NRF_REGISTER_CONFIG, &buf, 1);
  buf &= ~(0b11 << 2);  // Clear CRCO bits
  buf |= (crc_len << 2);
  write_register(NRF_REGISTER_CONFIG, &buf, 1);
}

// --- Public --- //

int nrf_init()
{
  spi_init();
  cs(HIGH);
  ce(LOW);

  uint8_t buf = 0xff;
  write_register(NRF_REGISTER_SETUP_RETR, &buf, 1);

  nrf24_set_address_width(NRF_ADDRESS_WIDTH_5_BYTES);
  nrf24_set_channel(0);
  nrf24_set_data_rate(NRF_DATA_RATE_250kbps);
  nrf24_set_output_power(NRF_OUTPUT_POWER_0dBm);
  nrf24_set_auto_ack(true);
  nrf24_set_enable_dynamic_payloads(true);
  nrf24_set_enable_ack_payload(true);
  nrf24_set_crc_length(NRF_CRC_SCHEME_2_BYTES);

  // Addresses
  char* addr = "00000";
  set_tx_address((uint8_t*) addr, 5);
  set_rx_address(0, (uint8_t*) addr, 5);

  // RX setup
  read_register(NRF_REGISTER_CONFIG, &buf, 1);
  buf |= (1 << NRF_PRIM_RX_BIT); // Set RX bit 1
  buf |= (1 << 1); // Set power up bit 1
  write_register(NRF_REGISTER_CONFIG, &buf, 1);
  ce(HIGH);
  delay(1); // Only need 130 us delay.

  return 0;
}

uint8_t nrf_read(uint8_t* buf) {
  // Returns bytes read
  uint8_t status = nrf_nop();
  uint8_t size = 0;

  if (status & (1 << RX_DR)) {
    // We got new message
    uint8_t rx_pipe = status & (0b111 << 1);

    rx_payload_width(&size);
    read_payload(buf, size);

    led_set(0);

    status |= (1 << RX_DR);
    write_register(NRF_REGISTER_STATUS, &status, 1);

    delay(20);
  }

  return size;
}

int nrf_write_data(const uint8_t* data, uint8_t len)
{
  uint8_t status = set_tx();

  status |= (1 << 4);  // Write 1 to clear MAX_RT bit
  write_register(NRF_REGISTER_STATUS, &status, 1);

  write_payload(data, len);

  transmit();

  return 0;
}
