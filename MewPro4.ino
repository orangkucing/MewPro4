#include <Arduino.h>
#include <EEPROM.h>
#include "MewPro.h"

// enable console output
// set false if this is MewPro #0 of dual dongle configuration
boolean debug = true;

// a modified version of WireS library is already included in MewPro4 source package to minimize memory consumption
#include "WireS.h"

// end of Options
//////////////////////////////////////////////////////////

#include "startup.h"

// i2c transaction state
const int SESSION_IDLE           = 0;
// send command length
const int SESSION_CMDLEN_SENT    = 1;
// send command body
const int SESSION_CMDBODY_SENT   = 2;
// prepare reply request
const int SESSION_RPLRQBUF_READY = 3; 
// send reply request length
const int SESSION_RPLRQLEN_SENT  = 4;
// send reply request body
const int SESSION_RPLRQBODY_SENT = 5;
volatile char i2cState = SESSION_IDLE;

volatile unsigned char session = 0xFF;

byte hour, minute, second; // to sync timestamps

void setup()
{
  // Remark. AVR8 8MHz is too slow to catch up with the highest 115200 baud.
  //     cf. http://forum.arduino.cc/index.php?topic=54623.0
  // Set 57600 baud or slower.
  Serial.begin(57600);

  setupLED(); // onboard LED setup
  initEEPROM();

  pinMode(BPRDY, INPUT);
  pinMode(I2CINT, INPUT);
  pinMode(HBUSRDY, INPUT);
  digitalWrite(PWRBTN, HIGH);
  pinMode(PWRBTN, OUTPUT);
}

void loop() 
{
  char *addr;
  if (queueState == QUEUE_EMPTY) {
    addr = (char *)pgm_read_word(&(startUp[(unsigned char)(startupSession++)]));
    if (addr != NULL) {
      queueIn(addr);
      queueState = QUEUE_BUSY;
    } else { // boot finished
      queueState = QUEUE_HALT;
    }
  }
  checkBacpacCommands();
  checkCameraCommands();
}

