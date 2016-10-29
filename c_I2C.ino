#define I2C_NOSTOP false
#define I2C_STOP true

#define WIRE              Wire

// I2C slave addresses
const int I2CEEPROM = 0x50;
const int SMARTY = 0x60;

const byte rom_omni_0[] PROGMEM = {
  // Primary: slot #1
  ID_PRIMARY, // id
  0x01, // major version
  0x01, // minor version
  0x06, // number of cameras in the rig
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // serial number (?)
  // check sum
  // 0
  // 0
  // 0
};

const byte rom_omni_1[] PROGMEM = {
  // Secondary: slot numbers 2, 3, 4, 5, 6 correspond to ADDRESS 0, 1, 2, 3, 4, respectively
  #define EMULATED_ADDRESS 0
  #define SLOT_NUMBER EMULATED_ADDRESS+2
    ID_SECONDARY, // id
    0x01, // major version
    0x01, // minor version
    SLOT_NUMBER,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // serial number (?)
    // check sum
    // 0
    // 0
    // 0
};

const byte* const rom_omni[2] PROGMEM = {
  rom_omni_0, rom_omni_1,
};

const byte rom_dual_hero_0[] PROGMEM = {
  // Master
  ID_MASTER, // id
  0x05, // major version (Note: Dual Hero's value = 5)
  0x01, // minor version
  0x0a,
};

const byte rom_dual_hero_1[] PROGMEM = {
  // Slave
  ID_SLAVE, // id
  0x05, // major version (Note: Dual Hero's value = 5)
  0x01, // minor version
  0x0b,
};

const byte* const rom_dual_hero[] PROGMEM = {
  rom_dual_hero_0, rom_dual_hero_1,
};

#ifndef EMULATED_ADDRESS
#define EMULATED_ADDRESS 0
#endif

volatile uint8_t wordAddr;
volatile boolean repeatedStart;
volatile boolean emulateRom;
byte *sendptr;

// interrupts
boolean addressHandler(uint16_t slave, uint8_t count)
{
  emulateRom = ((slave >> 1) == I2CEEPROM);
  repeatedStart = (count > 0 ? true : false);
  if (repeatedStart && WIRE.available()) {
    wordAddr = WIRE.read();
  }
  return true;
}

void receiveHandler(size_t numBytes)
{ 
  if (emulateRom) {
    // EEPROM write (not emulated)
    return;
  }
  
  // SMARTY
  if (numBytes <= 1) {
    return;
  }
#ifndef DEVELOPPER_DEBUG
  if (!isOmni() && RECV(1) == 0 && RECV(2) == 0) {
     // camera's default firmware PROTOCOL FATAL FLAW:
     //   we must ignore all the reply packets otherwise data collision might occur
     return;
  }
#endif
  recvc++;
}

void requestHandler()
{
  if (emulateRom) {
    // EEPROM access
    if (repeatedStart) {
      // Random Read or Sequential Read
      for (int i = 0; i < 16; i++) {
        WIRE.write(EEPROM.read((i + wordAddr) % 16));
      }
    } else {
      // Current Address Read
      WIRE.write((uint8_t)wordAddr);
    }
    return; 
  }
  
  // SMARTY
  if (isOmni()) {
    if (repeatedStart) {
      switch (i2cState) {
        case SESSION_IDLE: // send command length
        case SESSION_RPLRQBUF_READY: // send reply request length (2)
          WIRE.write(buf, (int) 1);
          break;
        case SESSION_CMDLEN_SENT: // command body
        case SESSION_RPLRQLEN_SENT: // reply request body (slave # followed by 1)
          WIRE.write(sendptr, (int) buf[0]);
          digitalWrite(I2CINT, LOW);
          break;
        default:
          return;
      }
      i2cState++;
    }
  } else {
    WIRE.write(sendptr, (int) buf[0]);
    digitalWrite(I2CINT, HIGH);
  }
}

void resetI2C()
{
  emptyQueue();
  WIRE.begin(I2CEEPROM, ((SMARTY << 1) | 1));
  WIRE.onAddrReceive(addressHandler);
  WIRE.onReceive(receiveHandler);
  WIRE.onRequest(requestHandler);
  session = 0xFF;
}

// Write I2C EEPROM
void __romWrite(uint8_t id)
{
  byte d = 0;
  int a;
  byte *addr, c;
  int i = 1;
  switch (id) {
    case ID_PRIMARY:
      i = 0;
      // fall down
    case ID_SECONDARY:
      addr = (byte *)pgm_read_word(&rom_omni[i]);
      for (a = 0; a < 12; a++) {
        c = (byte)pgm_read_byte(addr + a);
        d += c;
        EEPROM.write(a, c);
      }
      EEPROM.write(a++, d);
      for (; a < 16; a++) {
        EEPROM.write(a, (byte) 0);
      }
      break;
    case ID_MASTER:
      i = 0;
      // fall down
    case ID_SLAVE:
      addr = (byte *)pgm_read_word(&rom_dual_hero[i]);
      for (a = 0; a < 4; a++) {
        c = (byte)pgm_read_byte(addr + a);
        EEPROM.write(a, c);
      }
      break;
  }
}

#define ID_TARGET ID_SLAVE
// Write built-in EEPROM
void roleChange()
{
  byte id, d;
  // emulate detouching bacpac by releasing BPRDY line
  pinMode(BPRDY, INPUT);
  delay(1000);

  id = isOmni() ? ID_TARGET : ID_SECONDARY;
  __romWrite(id);
  pinMode(BPRDY, OUTPUT);
  eepromId = id;
  digitalWrite(BPRDY, LOW);
  initEEPROM();
}

void initEEPROM()
{
  // emulate bacpac for Omni (ID_SECONDARY) or default (ID_TARGET) firmwares
  eepromId = EEPROM.read(0);
//  if (eepromId != ID_TARGET && eepromId != ID_SECONDARY) {
    __romWrite(ID_TARGET);
    eepromId = ID_TARGET;
//  }
  switch (eepromId) {
    case ID_MASTER:
      __debug(F("Dual Hero EEPROM (master)"));
      break;
    case ID_SLAVE:
      __debug(F("Dual Hero EEPROM (slave)"));
      break;
    case ID_PRIMARY:
      __debug(F("Omni EEPROM (primary)"));
      break;
    case ID_SECONDARY:
      __debug(F("Omni EEPROM (secondary)"));
      break;
  }
}

// print out debug information to Arduino serial console
void __debug(const __FlashStringHelper *p)
{
  if (debug) {
    Serial.println(p);
  }
}

void printHex(uint8_t d, boolean upper)
{
  char t;
  char a = upper ? 'A' : 'a';
  t = d >> 4 | '0';
  if (t > '9') {
    t += a - '9' - 1;
  }
  Serial.print(t);
  t = d & 0xF | '0';
  if (t > '9') {
    t += a - '9' - 1;
  }
  Serial.print(t);
}

void _printInput()
{
  if (debug) {
    int i = 1;
    int buflen = RECV(0);
    Serial.print(F("> "));
    while (i <= buflen) {
      if ((i == 1 || i == 2) && isprint(RECV(i))) {
        Serial.print(' '); Serial.print((char) RECV(i));
      } else {
        printHex(RECV(i), false);
      }
      Serial.print(' ');
      i++;
    }
    if (recvc == 1) {
      Serial.println("");
    } else {
      Serial.println(F(" * collision detected"));
    } 
  }
}

void SendBufToCamera(byte *p) {
  if (buf[0] > 3) {
    if (debug) {
      int i = 0;
      int buflen = p[2];
      Serial.print(F("< "));
      while (i < buflen) {
        if ((i == 0 || i == 1) && isprint(p[i + 3])) {
          Serial.print(' '); Serial.print((char) p[i + 3]);
        } else {
          printHex(p[i + 3], false);
        }
        Serial.print(' ');
        i++;
      }
      Serial.println("");
    }
    // keep current time if sync requested
    if (p[4] == 'Y' && p[8] == 2 && p[9] == 27) {
      hour = p[13]; minute = p[14]; second = p[15];
    }
  } else {
    if (debug) {
      Serial.println(F("< request reply"));
    }
  }
  if (isOmni()) {
    sendptr = p;
    digitalWrite(I2CINT, HIGH);
  } else {
    sendptr = p + 2;
    buf[0] = p[2] + 1;
    digitalWrite(I2CINT, LOW);
  }
}

// Camera power On
void powerOn()
{
  unsigned long t;
  if (isOmni()) {
    digitalWrite(I2CINT, LOW); pinMode(I2CINT, OUTPUT);
    digitalWrite(PWRBTN, LOW);
    delay(500);
    digitalWrite(PWRBTN, HIGH);
    t = millis();
    while (millis() - t < 1000 && digitalRead(HBUSRDY) != HIGH) { // wait until camera is up; but don't lock up.
      ;
    }
    delay(60);
    resetI2C();
    digitalWrite(BPRDY, LOW); pinMode(BPRDY, OUTPUT);    // Show camera MewPro is ready.
    delay(2500);
    startUp = (char **)omni_startUp;
    startupSession = 0; queueState = QUEUE_EMPTY;
  } else {
    digitalWrite(BPRDY, LOW); pinMode(BPRDY, OUTPUT);    // Show camera MewPro is ready.
    digitalWrite(I2CINT, HIGH); pinMode(I2CINT, OUTPUT);
    digitalWrite(PWRBTN, LOW);
    delay(500);
    digitalWrite(PWRBTN, HIGH);
    t = millis();
    while (millis() - t < 1000 && digitalRead(HBUSRDY) != HIGH) { // wait until camera is up; but don't lock up.
      ;
    }    
    resetI2C();
    startUp = (char **)default_startUp;
  }
}

void checkCameraCommands()
{
  if (i2cState == SESSION_CMDBODY_SENT ) { // Omni only
    // buf[0..6] can be modified anytime
    buf[0] = 2; buf[1] = EMULATED_ADDRESS; buf[2] = 1;
    i2cState = SESSION_RPLRQBUF_READY;
    SendBufToCamera(buf+1);
    return;
  }
  while (inputAvailable())  {
    static boolean shiftable;
    byte c = myRead();
    switch (c) {
      case ' ':
        shiftable = false;
        continue;
      case '\r':
      case '\n':
        serialfirst = false;
        if (bufp != 6) {
          if (i2cState != SESSION_IDLE) { // Omni only
            myUnread(c);
            return;
          }
          // buf[6..] has been set until now by inputting from the queue or the serial exclusively
          // buf[6:7] contains a two-letter command and buf[8..] is the arguments to the command
          // so let's reformat buf[0..7] now in order to send the camera
          buf[0] = bufp - 1;      // Omni only
          buf[1] = 5; buf[2] = 2; // Omni only
          buf[3] = bufp - 4;
          buf[4] = buf[6]; buf[5] = buf[7];
          buf[6] = ++session;
          buf[7] = buf[4] == 'Y' ? 6 : 4;
          bufp = 6;
          SendBufToCamera(buf + 1);
        }
        return;
      case '@': // power on
        bufp = 6;
        serialfirst = false;
        __debug(F("camera power on"));
        powerOn();
        __emptyInputBuffer();
        return;     
/*      case '&':
        bufp = 6;
        debug = !debug;
        serialfirst = false;
        __debug(F("debug messages on"));
        __emptyInputBuffer();
        return;
      case '!':
        bufp = 6;
        serialfirst = false;
        __debug(F("role change"));
        roleChange();
        __emptyInputBuffer();
        return; */
      default:
        if (bufp >= 8 && isxdigit(c)) {
          c -= '0';
          if (c >= 10) {
            c = (c & 0x0f) + 9;
          }    
        }
        if (bufp < 9) {
          shiftable = true;
          buf[bufp++] = c;
        } else {
          if (shiftable) {
            buf[bufp-1] = (buf[bufp-1] << 4) + c;
          } else {
            buf[bufp++] = c;
          }
          shiftable = !shiftable;      
        }
        break;
    }
  }
}
