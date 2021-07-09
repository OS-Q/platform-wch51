// ADS1255 PGA gain level, hard coded for now
#define GAIN_LOG2   6

/*
  CH552 + ADS1255 Analog Frontend for Magnetic sensor
  a very simple successor of https://github.com/libc0607/ch552-tmr-afe/tree/main/hx712-afe
  Hardware at https://oshwhub.com/libc0607/ch552-ads1255-simple-afe-v1

  Please install https://github.com/DeqingSun/ch55xduino first,
  then set "CH552 Board", "Default CDC", "USB", "24MHz (internal), 5V"
  Use WCHISPTool (http://www.wch.cn/download/WCHISPTool_Setup_exe.html) to upload

  Note that this project is just a *toy*,
  The goal of the design is to display a bunch of floating inexplicable numbers (True RNG (?)
  also the hardware design was shit (AVDD is directly powered by VUSB!)
  it may be used by some people to fool others in the future
  or may appear in some papers as "coursework"
  Also the resources of CH552 are very limited so the code looks like shit without any reusability

  Please do some Google if you need a better ads125x driver
  the codes in the first page of search results are really better than things below
*/

#include "include/ch5xx.h"  // SFR registers def
#include <SPI.h>

// =========== Pin definitions ===================
#define PIN_LED       33    // out
#define PIN_KEY       11    // in
#define PIN_CLKCTL    34    // out
#define PIN_ADCSYNC   31    // out
#define PIN_ADCRST    30    // out
#define PIN_ADCCS     32    // out
#define PIN_ADCDRDY   14    // in

// SPI group, not used
#define PIN_MOSI      15    // out
#define PIN_MISO      16    // in
#define PIN_CLK       17    // out
// =========== End of Pin definitions ============

// ADS1255 Defs
#define ADS1255_SPI_CLK       100000
// registers used
#define ADS1255_REG_STATUS    0x00
#define ADS1255_REG_MUX       0x01
#define ADS1255_REG_ADCON     0x02
#define ADS1255_REG_DRATE     0x03
#define ADS1255_REG_IO        0x04
// commands
#define ADS1255_CMD_WAKEUP    0x00
#define ADS1255_CMD_RDATA     0x01
#define ADS1255_CMD_RDATAC    0x03
#define ADS1255_CMD_SDATAC    0x0f
#define ADS1255_CMD_RREG      0x10
#define ADS1255_CMD_WREG      0x50
#define ADS1255_CMD_SELFCAL   0xF0
#define ADS1255_CMD_SYNC      0xFC
#define ADS1255_CMD_STANDBY   0xFD
#define ADS1255_CMD_RESET     0xFE
// gain
#define ADS1255_GAIN_LEVEL_MAX 6
#define ADS1255_GAIN_LEVEL_MIN 0

uint8_t old_drdy = HIGH;
uint32_t adc_cnt = 0;
uint8_t adc_gain = GAIN_LOG2;
uint16_t adc_rate = 0;

const uint8_t ads1255_rate_table[] = {
  0x92, 	// 500,
  0x82,		//100,
  0x72,		// 60,
  0x33,  	// 15,
  0x13, 	// 5,
};
#define ADS1255_RATE_TABLE_SIZE (sizeof(ads1255_rate_table))

void _ads1255_wait_drdy()
{
  while (digitalRead(PIN_ADCDRDY) == HIGH);
}

void _ads1255_reg_read(uint8_t reg_addr, uint8_t n, uint8_t* p_data)
{
  uint8_t i = 0;

  digitalWrite(PIN_ADCCS, LOW);
  delayMicroseconds(5); // t3

  _ads1255_wait_drdy();
  SPI_transfer(ADS1255_CMD_RREG | (reg_addr & 0x0F));   // 0001 rrrr
  SPI_transfer(n - 1);

  delayMicroseconds(20); // t6 > 50 * (1/7.68M) ~= 6.5us
  for (i = 0; i < n; i++)
    p_data[i] = SPI_transfer(0xFF);

  delayMicroseconds(5); // t10 > 8 * (1/7.68M)
  digitalWrite(PIN_ADCCS, HIGH);
  return;
}

void _ads1255_reg_write(uint8_t reg_addr, uint8_t write_data)
{
  digitalWrite(PIN_ADCCS, LOW);
  delayMicroseconds(5); // t3

  _ads1255_wait_drdy();
  SPI_transfer(ADS1255_CMD_WREG | (reg_addr & 0x0F));   // 0101 wwww
  SPI_transfer(0);  // n-1, n=1
  SPI_transfer(write_data);

  delayMicroseconds(5); // t10 > 8 * (1/7.68M)
  digitalWrite(PIN_ADCCS, HIGH);
  return;
}

void _ads1255_cmd_send(uint8_t cmd)
{
  digitalWrite(PIN_ADCCS, LOW);
  delayMicroseconds(5); // t3

  _ads1255_wait_drdy();
  SPI_transfer(cmd);
  delayMicroseconds(5);

  digitalWrite(PIN_ADCCS, HIGH);
  return;
}

void _ads1255_call_reset()
{
  _ads1255_cmd_send(ADS1255_CMD_RESET);
  delay(5);
  _ads1255_cmd_send(ADS1255_CMD_SDATAC);
  delay(1);
}

// gain: 0-1, 1-2, 2-4, 3-8, 4-16, 5-32, 6-64
void ads1255_set_gain(uint8_t gain)
{
  uint8_t tmp;
  _ads1255_reg_read(ADS1255_REG_ADCON, 1, &tmp);
  _ads1255_reg_write(ADS1255_REG_ADCON, ((tmp & 0xF8) | (gain & 0x07)) );

}

void ads1255_set_drate(uint8_t drate_tbl_val)
{
  _ads1255_reg_write(ADS1255_REG_DRATE, drate_tbl_val);
}

void ads1255_debug_dump_reg()
{
  uint8_t tmp[11];
  uint8_t i;

  USBSerial_println_s("[DBG] ads1255 regdump: ");
  _ads1255_reg_read(0x00, 11, tmp);
  for (i = 0; i < 11; i++) {
    USBSerial_print_s("[DBG] REG ");
    USBSerial_print_ub(i, 16);
    USBSerial_print_s("h: ");
    USBSerial_print_ub(tmp[i], 16);
    USBSerial_println_s("h");
  }
  USBSerial_println_s("[DBG] end of ads1255 regdump");

}

void ads1255_call_selfcal()
{
  _ads1255_cmd_send(ADS1255_CMD_SELFCAL);
  _ads1255_wait_drdy();
}

// should be called after negedge of drdy
int32_t ads1255_call_read_data()
{
  int32_t d = 0;

  digitalWrite(PIN_ADCCS, LOW);
  delayMicroseconds(1); // t3

  while (digitalRead(PIN_ADCDRDY) == HIGH);
  SPI_transfer(ADS1255_CMD_RDATA);
  delayMicroseconds(1);

  d = d | SPI_transfer(0xff);
  d = d << 8;
  d = d | SPI_transfer(0xff);
  d = d << 8;
  d = d | SPI_transfer(0xff);
  if (d & 0x800000)
    d |= 0xff000000;

  delayMicroseconds(1);
  digitalWrite(PIN_ADCCS, HIGH);

  return d;
}

uint8_t ads1255_init()
{
  uint8_t tmp;

  _ads1255_call_reset();

  // turn off clock output, turn off sensor detect
  _ads1255_reg_read(ADS1255_REG_ADCON, 1, &tmp);
  _ads1255_reg_write(ADS1255_REG_ADCON, tmp & 0x07);

  // set all gpios to output high
  _ads1255_reg_write(ADS1255_REG_IO, 0x0f);

  // enable buffer, disable autocal
  _ads1255_reg_write(ADS1255_REG_STATUS, 0x02);
}

void setup()
{
  // Pin setup
  pinMode(PIN_LED,      OUTPUT);
  pinMode(PIN_CLKCTL,   OUTPUT);
  pinMode(PIN_ADCSYNC,  OUTPUT);
  pinMode(PIN_ADCRST,   OUTPUT);
  pinMode(PIN_ADCCS,    OUTPUT);
  pinMode(PIN_MOSI,     OUTPUT);
  pinMode(PIN_CLK,      OUTPUT);
  pinMode(PIN_KEY,      INPUT_PULLUP);
  pinMode(PIN_ADCDRDY,  INPUT_PULLUP);
  pinMode(PIN_MISO,     INPUT_PULLUP);
  digitalWrite(PIN_ADCRST, HIGH);
  digitalWrite(PIN_ADCSYNC, HIGH);

  while (!USBSerial());
  delay(100);
  USBSerial_println_s("[INF] USB serial connected, startup");

  // SPI Setup
  // CH552 only support SPI mode 0 & 3,
  // so an 74LVC1G86 is used to invert the clk signal with a GPIO
  //                     ------
  // PIN_CLKCTL >------>| XOR  |>-----> ADC_CLK
  // PIN_CLK >--------->| 1G86 |
  //                     ------
  // (SPI Mode 0) + (PIN_CLKCTL=LOW)  => (SPI Mode 0)
  // (SPI Mode 3) + (PIN_CLKCTL=HIGH) => (SPI Mode 1) -- ADS1255 needs
  // (SPI Mode 0) + (PIN_CLKCTL=HIGH) => (SPI Mode 2)
  // (SPI Mode 3) + (PIN_CLKCTL=LOW)  => (SPI Mode 3)
  digitalWrite(PIN_CLKCTL, HIGH);
  SPI_begin();
  SPI_beginTransaction(SPISettings(ADS1255_SPI_CLK, MSBFIRST, SPI_MODE3));

  // init ads1255
  ads1255_init();
  ads1255_set_gain(adc_gain);
  ads1255_set_drate(ads1255_rate_table[adc_rate]);


  // Finish setup, blink
  digitalWrite(PIN_LED, LOW);
  delay(500);
  digitalWrite(PIN_LED, HIGH);
  delay(500);

  ads1255_call_selfcal();	// waiting for a stable vref
  USBSerial_println_s("[INF] Init complete, start reading ...");
  adc_cnt = 0;
  delay(50);
  ads1255_debug_dump_reg();

}

void loop()
{
  int32_t dat_24b;

  if (digitalRead(PIN_ADCDRDY) == LOW) {
    delayMicroseconds(5);
    if (digitalRead(PIN_ADCDRDY) == LOW && old_drdy == HIGH) {
      old_drdy = LOW;

      // got negedge, read data
      adc_cnt++;
      dat_24b = ads1255_call_read_data();

      // print to usb cdc
      USBSerial_println_i(dat_24b);

      // gain control
      // pass

      // only check keys after one readout period, hardware filtered
      if (digitalRead(PIN_KEY) == LOW) {
        digitalWrite(PIN_LED, HIGH);  // disable led
        adc_rate = (adc_rate >= ADS1255_RATE_TABLE_SIZE - 1) ? 0 : adc_rate + 1;
        ads1255_set_drate(ads1255_rate_table[adc_rate]);
        while (digitalRead(PIN_KEY) == LOW);
      }

      // invert led
      digitalWrite(PIN_LED, (adc_cnt & 0x1) ? LOW : HIGH);
    }
  } else {
    old_drdy = HIGH;
  }
}
