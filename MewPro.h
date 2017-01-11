#include <Arduino.h>
#include "Pins.h"
#include <EEPROM.h>

// enable console output
// set false if this is MewPro #0 of dual dongle configuration
boolean debug = true;

#undef OMNI_SUPPORT

// a modified version of WireS library is already included in MewPro4 source package to minimize memory consumption
#include "WireS.h"

// end of Options
//////////////////////////////////////////////////////////

#include "startup.h"

// i2c transaction state (for Omni firmware only)
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

// GoPro Omni EEPROM IDs
const int ID_PRIMARY = 0x24;
const int ID_SECONDARY = 0x25;
// GoPro Dual Hero EEPROM IDs
const int ID_MASTER = 0x4;
const int ID_SLAVE  = 0x5;

volatile int eepromId;
#define isOmni() (eepromId & 0x20)

// .cpp file will be compiled separately
extern boolean ledState;
extern void ledOff();
extern void ledOn();
extern void setupLED();

// bacpac commands
//

#define MODE_VIDEO      0
#define MODE_PHOTO      1
#define MODE_MULTI_SHOT 2
#define MODE_SETUP      5
int mode = MODE_VIDEO;

#define STATUS_POWERON 0
#define STATUS_RECORDING 0x08
#define STATUS_BUSY 0x10
#define STATUS_IDLE 0x30

#define MEWPRO_BUFFER_LENGTH 128

char *queueadr; // flash address of string
int queueidx = -1;
boolean serialfirst = false;

byte buf[MEWPRO_BUFFER_LENGTH];
int bufp = 6;

// read the I2C buffer directly
volatile char recvc = 0;
char bootID = 0;
#define RECV(a) (Wire.i2cData.Buffer[a + (isOmni() ? 4 : 0)])

