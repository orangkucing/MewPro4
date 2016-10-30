#include "MewPro.h"

void setup()
{
  // Remark. AVR8 8MHz is too slow to catch up with the highest 115200 baud.
  //     cf. http://forum.arduino.cc/index.php?topic=54623.0
  // Set 57600 baud or slower.
  Serial.begin(57600);

  setupLED(); // onboard LED (if any) setup
  initEEPROM();

  pinMode(BPRDY, INPUT);
  pinMode(I2CINT, INPUT);
  pinMode(HBUSRDY, INPUT);
  digitalWrite(PWRBTN, HIGH);
  pinMode(PWRBTN, OUTPUT);
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

