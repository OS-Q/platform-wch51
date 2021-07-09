/* CH552 + HX712 analog frontend
 * https://github.com/libc0607/ch552-tmr-afe
 * Hardware at https://oshwhub.com/libc0607/hx71x-tmr-afe-v1
 */

//===== Pin defs =====
#define PIN_LED1    33
#define PIN_LED2    11
#define PIN_KEY     34
#define PIN_DOUT    16
#define PIN_PDSCK   17

uint32_t adc_cnt = 0;     // adc counter (for led blink)
uint8_t adc_rate = 10;    // adc rate, 10/40 (Hz)
uint8_t old_dout = HIGH;  // for negedge detect of DOUT

int32_t hx712_read_data(uint8_t next_rate) {
  
  uint32_t dat = 0x0;
  uint8_t i;
  int d;
  
  digitalWrite(PIN_PDSCK, LOW);
  delayMicroseconds(20); // T1

  for (i=24; i>0; i--) {
    digitalWrite(PIN_PDSCK, HIGH);
    delayMicroseconds(5); // T3
    d = digitalRead(PIN_DOUT);
    dat = dat<<1;
    delayMicroseconds(5); // T3
    if (d == HIGH) {
      dat++;
      //USBSerial_print_s("1.");
    } else {
      //USBSerial_print_s("0.");
    }
    digitalWrite(PIN_PDSCK, LOW);
    delayMicroseconds(10); // T4
  }
  dat ^= 0x800000;

  // 25
  digitalWrite(PIN_PDSCK, HIGH);
  delayMicroseconds(10); // T3
  digitalWrite(PIN_PDSCK, LOW);
  delayMicroseconds(10); // T4

  // 26, 27
  if (next_rate == 40) {
    digitalWrite(PIN_PDSCK, HIGH);
    delayMicroseconds(10); // T3
    digitalWrite(PIN_PDSCK, LOW);
    delayMicroseconds(10); // T4
    digitalWrite(PIN_PDSCK, HIGH);
    delayMicroseconds(10); // T3
    digitalWrite(PIN_PDSCK, LOW);
    delayMicroseconds(10); // T4
  }

  return dat;
}

void send_frame_string(int32_t adc_val) {
  USBSerial_println_i(adc_val);
}

void setup() {

  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_KEY, INPUT_PULLUP);
  pinMode(PIN_DOUT, INPUT);
  pinMode(PIN_PDSCK, OUTPUT);

  // enable hx712
  digitalWrite(PIN_PDSCK, LOW);

  // blink
  digitalWrite(PIN_LED1, HIGH);
  digitalWrite(PIN_LED2, HIGH);
  delay(100);
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, LOW);
}

void loop() {
  int32_t dat_24b;

  // read adc
  if (digitalRead(PIN_DOUT) == LOW) {
    delayMicroseconds(20);
    if (digitalRead(PIN_DOUT) == LOW && old_dout == HIGH) { // got negedge
      old_dout = LOW;
      
      // data ready, get data
      dat_24b = hx712_read_data(adc_rate);
      adc_cnt++;

      // send data to usb cdc
      send_frame_string(dat_24b);
      
      // check key 
      if (digitalRead(PIN_KEY) == LOW) {
        delay(10);
        if (digitalRead(PIN_KEY) == LOW) {
          adc_rate = (adc_rate == 10)? 40: 10;
          while(digitalRead(PIN_KEY) == LOW);
        }
      }

      // update led
      if (adc_rate == 10) {
        digitalWrite(PIN_LED1, ((adc_cnt%2)==1)? LOW: HIGH );
        digitalWrite(PIN_LED2, LOW);
      } else {
        digitalWrite(PIN_LED1, LOW);
        digitalWrite(PIN_LED2, ((adc_cnt%2)==1)? LOW: HIGH );
      }

      // nothing else to do, let PC do more
      
    }
  } else {
    old_dout = HIGH;
  }
  delay(2);
}
