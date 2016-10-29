// Arduino pins
//                           0;    // ADC                  (Used by Serial port TXO)
//                           1;    // ADC                  (Used by Serial port RXI)
const int I2CINT           = 2;    // ADC PWM
const int TRIG             = 3;    // ADC PWM
const int PWRBTN           = 4;    // ADC
const int HBUSRDY          = 5;    // ADC
//                           6;    // AIN1
//                           7;    // AIN0
//                           8;    // AREF
//                                 // GND
//                                 // VCC
//                                 // (9)  XTAL1
//                                 // (10) XTAL2
//                                 // (17) RESET
//                           11;   // ADC INT0
//                           12;   // ADC             SCK  (Used by I2C SCL)
//                           13;   // ADC PWM              built-in LED (optional)
//                           14;   // ADC PWM
const int BPRDY            = 15;   // ADC Serial1 TXO MISO
//                           16;   // ADC Serial1 RXI MOSI (Used by I2C SDA)
#define digitalPinToInterrupt(a) (0) // INT0

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

#define DEVELOPPER_DEBUG 1
