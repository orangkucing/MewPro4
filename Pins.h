// Arduino pins
//                           0;    // ADC                  (Used by Serial port TXO)
//                           1;    // ADC                  (Used by Serial port RXI)
const int I2CINT           = 2;    // ADC PWM
const int TRIG             = 3;    // ADC PWM              (for Hero 3+ Black only)
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

