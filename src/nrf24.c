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

#define NRF_BIT_CONFIG_MASK_RX_DR  6  // R/W Mask interrupt caused by RX_DR
#define NRF_BIT_CONFIG_MASK_TX_DS  5  // R/W Mask interrupt caused by TX_DS
#define NRF_BIT_CONFIG_MASK_MAX_RT 4  // R/W Mask interrupt caused by MAX_RT
#define NRF_BIT_CONFIG_EN_CRC      3  // R/W Enable CRC. Forced high if one of the bits in the EN_AA is high
#define NRF_BIT_CONFIG_CRCO        2  // R/W CRC encoding scheme
#define NRF_BIT_CONFIG_PWR_UP      1  // R/W 1: POWER UP, 0:POWER DOWN
#define NRF_BIT_CONFIG_PRIM_RX     0  // RX/TX control: 1: PRX, 0: PTX

#define NRF_BIT_STATUS_RX_DR   6  // R/W Data Ready RX FIFO interrupt. Asserted when new data arrives RX FIFOc. Write 1 to clear bit.
#define NRF_BIT_STATUS_TX_DS   5  // R/W Data Sent TX FIFO interrupt. Asserted when packet transmitted on TX. If AUTO_ACK is activated, this bit is set high only when ACK is received. Write 1 to clear bit.
#define NRF_BIT_STATUS_MAX_RT  4  // R/W Maximum number of TX retransmits interrupt Write 1 to clear bit. If MAX_RT is asserted it must be cleared to enable further communication.
#define NRF_BIT_STATUS_RX_P_NO 3  // 3:1  R Data pipe number for the payload available for reading from RX_FIFO 000-101: Data Pipe Number 110: Not Used 111: RX FIFO Empty
#define NRF_BIT_STATUS_TX_FULL 0  //R TX FIFO full flag. 1: TX FIFO full.0: Available locations in TX FIFO.

#define NRF_PARAM_ADDRESS_WIDTH  NRF_ADDRESS_WIDTH_5_BYTES
#define NRF_PARAM_CHANNEL        0
#define NRF_PARAM_DATA_RATE      NRF_DATA_RATE_250kbps
#define NRF_PARAM_OUTPUT_POWER   NRF_OUTPUT_POWER_0dBm
#define NRF_PARAM_CRC_LENGTH     NRF_CRC_SCHEME_2_BYTES
#define NRF_PARAM_ADDRESS        "00000"


//#define cs(value) digitalWrite(GPIO_NRF_CS, value)
//#define ce(value) digitalWrite(GPIO_NRF_CE, value)

#define ce(value)  (PIN_NRF_CE = value)
#define cs(value)  (PIN_NRF_CS = value)

static uint8_t _i = 0;

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

// Helper functions
static uint8_t spi_write(uint8_t cmd, uint8_t* data, uint8_t len);
static uint8_t spi_read(uint8_t cmd, uint8_t* buf, uint8_t len);
static uint8_t write_register(uint8_t address, uint8_t* data, uint8_t len);
static uint8_t write_register_one_byte(uint8_t address, uint8_t data);
static uint8_t read_register(uint8_t address, uint8_t* buf, uint8_t len);
static uint8_t read_payload(uint8_t* buf, uint8_t len);
static uint8_t write_payload_ack(uint8_t* data, uint8_t len, uint8_t pipe);
static uint8_t nrf_nop();
static uint8_t rx_payload_width(uint8_t* width);
static uint8_t flush_rx();
static uint8_t flush_tx();

// --- Public --- //

uint8_t nrf_read(uint8_t* buf) {
  // Returns bytes read
  uint8_t status = nrf_nop();
  uint8_t size = 0;

  if (status & (1 << NRF_BIT_STATUS_RX_DR)) {
    // We got new message
    uint8_t rx_pipe = status & (0b111 << 1);

    rx_payload_width(&size);
    read_payload(buf, size);

    status |= (1 << NRF_BIT_STATUS_RX_DR);
    write_register(NRF_REGISTER_STATUS, &status, 1);
  }

  return size;
}


int nrf_init()
{
  // Initializes the NRF radio as receiver, being able to put ACK payload.
  spi_init();
  cs(HIGH);
  ce(LOW);

  // Toggle power down
  write_register_one_byte(NRF_REGISTER_CONFIG, 0);
  delay(1);

  // 0. Config
  write_register_one_byte(NRF_REGISTER_CONFIG, (1 << NRF_BIT_CONFIG_EN_CRC) | (1 << NRF_BIT_CONFIG_CRCO) | (1 << NRF_BIT_CONFIG_PWR_UP) | (1 << NRF_BIT_CONFIG_PRIM_RX));
  delay(2); // 1.5ms startup

  // Flush RX and TX

  // 1. EN_AA
  write_register_one_byte(NRF_REGISTER_EN_AA, 0b00111111);  // Enable auto ack on all pipes

  // 3. SETUP_AW
  write_register_one_byte(NRF_REGISTER_SETUP_AW, NRF_PARAM_ADDRESS_WIDTH);  // Enable auto ack on all pipes

  // 4. SETUP_RETR - TODO
  //write_register_one_byte(NRF_REGISTER_SETUP_AW, NRF_PARAM_ADDRESS_WIDTH);

  // 5. RF_CH
  write_register_one_byte(NRF_REGISTER_RF_CH, NRF_PARAM_CHANNEL);

  // 6. RF_SETUP
  write_register_one_byte(NRF_REGISTER_RF_SETUP, (NRF_PARAM_DATA_RATE << 3) | (NRF_PARAM_OUTPUT_POWER << 1));

  // 7. STATUS
  //write_register_one_byte(NRF_REGISTER_RF_CH, NRF_PARAM_CHANNEL);

  // 0A. RX Addr 0
  char* rx_addr = NRF_PARAM_ADDRESS;
  write_register(NRF_REGISTER_RX_ADDR_P0, (uint8_t*) rx_addr, 5);

  // 1C. DYNPD
  write_register_one_byte(NRF_REGISTER_DYNPD, 0b00111111);  // Enable dynamic payload on all pipes

  // 1D. FEATURE
  write_register_one_byte(NRF_REGISTER_FEATURE, 0b00000111);  // Enable dynamic payload length and autoack

  //write_register_one_byte(NRF_REGISTER_STATUS, 0);  // Clear status?

  ce(HIGH);

  return 0;
}


// --- Private helpers --- //

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

static uint8_t write_register(uint8_t address, uint8_t* data, uint8_t len)
{
  uint8_t cmd = (NRF_COMMAND_W_REGISTER << 5) | address;
  return spi_write(cmd, data, len);
}

static uint8_t write_register_one_byte(uint8_t address, uint8_t data)
{
  uint8_t cmd = (NRF_COMMAND_W_REGISTER << 5) | address;
  return spi_write(cmd, &data, 1);
}

static uint8_t read_register(uint8_t address, uint8_t* buf, uint8_t len)
{
  uint8_t cmd = (NRF_COMMAND_R_REGISTER << 5) | address;
  return spi_read(cmd, buf, len);
}

static uint8_t read_payload(uint8_t* buf, uint8_t len)
{
  return spi_read(NRF_COMMAND_R_RX_PAYLOAD, buf, len);
}

static uint8_t write_payload_ack(uint8_t* data, uint8_t len, uint8_t pipe)
{
  uint8_t cmd = (NRF_COMMAND_W_ACK_PAYLOAD << 5) | pipe;
  return spi_write(cmd, data, len);
}

static inline uint8_t nrf_nop()
{
  return spi_write(NRF_COMMAND_NOP, 0, 0);
}

static inline uint8_t flush_rx()
{
  return spi_write(NRF_COMMAND_FLUSH_RX, 0, 0);
}

static inline uint8_t flush_tx()
{
  return spi_write(NRF_COMMAND_FLUSH_TX, 0, 0);
}

static inline uint8_t rx_payload_width(uint8_t* width)
{
  return spi_read(NRF_COMMAND_R_RX_PL_WIDa, width, 1);
}
