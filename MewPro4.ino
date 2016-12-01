// MewPro4
//   Control GoPro Hero 4 Black cameras from the herobus
//
//   Target device: MewPro 2 or MewPro Cable (ATtiny1634 3.3V 8MHz)
//
//        (MewPro 1 with Arduino Pro Mini will not be supported due to ATmega328's broken TWI slave hardware)
//
//   Copyright (c) 2016 Hisashi ITO, Orangkucing Lab (info at mewpro.cc)
//   The source code is under MIT licence. For more detail please see the LICENCE file.

#include "MewPro.h"

void setup()
{
  // Remark. AVR8 8MHz is too slow to catch up with the highest 115200 baud.
  //     cf. http://forum.arduino.cc/index.php?topic=54623.0
  // Set 57600 baud or slower.
  Serial.begin(57600);

  setupLED(); // onboard LED (if any) setup
  initEEPROM();

  if (!isOmni()) {
    digitalWrite(I2CINT, HIGH); pinMode(I2CINT, OUTPUT);
    resetI2C();
    digitalWrite(BPRDY, LOW); pinMode(BPRDY, OUTPUT);
  } else {
    pinMode(BPRDY, INPUT);
    pinMode(I2CINT, INPUT);
  }
  pinMode(HBUSRDY, INPUT_PULLUP);
  digitalWrite(PWRBTN, HIGH); pinMode(PWRBTN, OUTPUT);
}

void loop() 
{
  // start up sessions
  if (queueState == QUEUE_EMPTY) {
    char *addr = (char *)pgm_read_word(&(startUp[(unsigned char)(startupSession++)]));
    if (addr != NULL) {
      queueIn(addr);
      queueState = QUEUE_BUSY;
    } else { // boot finished
      queueState = QUEUE_HALT;
    }
  }
  if (recvc) {
    if (parseI2C_R()) {
      i2cState = SESSION_CMDBODY_SENT; // send reply request again
    } else {
      i2cState = SESSION_IDLE;
      if (queueState != QUEUE_HALT) {
        queueState = QUEUE_EMPTY;
      }
    }    
    recvc = 0;
  }
  checkTerminalCommands();
}

