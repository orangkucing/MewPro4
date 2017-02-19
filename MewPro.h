#include <Arduino.h>
#include "Pins.h"
#include <EEPROM.h>

// enable console output
boolean debug = true;

// undef if MewPro 2 board is used as standalone
#define USE_GENLOCK

// a modified version of WireS library is already included in MewPro4 source package to minimize memory consumption
#include "WireS.h"

#ifdef USE_GENLOCK
//////////////////////////////////////////////////////////
// Options for genlock
//   Choose either "#define" to use or "#undef" not to use. 

// undef unless using GoPro Omni's privilaged firmware
#undef OMNI_SUPPORT

#else
//////////////////////////////////////////////////////////
// Options for standalone (not genlock)
//   Choose either "#define" to use or "#undef" not to use. 
//   if #define then don't forget to remove // before //#include

//********************************************************
// d_TimeAlarms: MewPro driven timelapse
#undef  USE_TIME_ALARMS
// Time and TimeAlarms libraries are downloadable from
//   http://www.pjrc.com/teensy/td_libs_Time.html
//   and http://www.pjrc.com/teensy/td_libs_TimeAlarms.html
//#include <TimeLib.h> // *** please comment out this line if USE_TIME_ALARMS is not defined ***
//#include <TimeAlarms.h> // *** please comment out this line if USE_TIME_ALARMS is not defined ***

//********************************************************
// e_Shutters: A remote shutters without contact bounce or chatter
#undef  USE_SHUTTER

// end of Options
//////////////////////////////////////////////////////////
#endif /* USE_GENLOCK */

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

byte omni_hour, omni_minute, omni_second; // to sync timestamps (for Omni)

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

