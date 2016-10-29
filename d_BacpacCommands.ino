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

void emulateDetachBacpac()
{
  // to exit AR mode, emulate detach and attach bacpac
  //
  // detach bacpac
  pinMode(BPRDY, INPUT);
  delay(1000);
  // attach bacpac again
  pinMode(BPRDY, OUTPUT);
  digitalWrite(BPRDY, LOW);
}

boolean bacpacCommand()
{
  boolean resend = false;
  int reply = isOmni() ? 2 : 0;
  switch (RECV(4)) {
    case 0: // received packet is the reply for the previous ZZ command
      resend = ZZcommand(reply);
      break;
    case 2: // received packet is the reply for the previous YY command
      resend = YYcommand(reply);
      break;
    case 4:
      ZZcommand(0); // ZZ command received. Dual Hero only
      break;
    case 6:
      YYcommand(0); // YY command received. Dual Hero only
      break;
  default:
      __debug(F("unknown packet received"));
      break;
  }
  return resend;
}

boolean YYcommand(int reply)
{
  boolean resend = false;
  switch (RECV(5 + reply)) {
    default:
      resend = extendedYYcommand(reply);
      break;
  }
  return resend;
}

boolean ZZcommand(int reply)
{
  boolean resend = false;
  switch (RECV(5 + reply)) {
    case 1: // bacpac capabilities? Dual Hero only
      switch (RECV(6 + reply)) {
        case 0:
          __debug(F("capabilities?"));
          if (!reply) {
            // Dual Hero only
            buf[0] = 14;
            RECV(0) = 0; RECV(1) = 1;
            RECV(2) = 11;
            memcpy_P((byte *)&(RECV(3)), F("\x00\x00\x00\x00\x00\x00\x01\x03\x03\x00\x00"), 11);
            i2cState = SESSION_IDLE;
            SendBufToCamera((byte *)&(RECV(0)));
            startupSession = 0; queueState = QUEUE_EMPTY; // start startup sequences       
          }
          break;
      }
      break;
    case 2: // camera power
      switch (RECV(6 + reply)) {
        case 1:
          __debug(F("power on"));
          if (!reply) {
            // Dual Hero only
            buf[0] = 11;
            RECV(0) = 0; RECV(1) = 1;
            RECV(2) = 8;
            memcpy_P((byte *)&(RECV(3)), F("\x00\x00\x00\x00\x00\x00\x02\x00"), 8);
            i2cState = SESSION_IDLE;
            SendBufToCamera((byte *)&(RECV(0)));                      
          }
          break;
      }
      break;
    case 3: // get camera version
      if (debug) {
        Serial.print(F("version: "));
        Serial.println((char *)&RECV(7 + reply));
      }
      break;
    case 0: // extended command
      resend = extendedZZcommand(reply);
      break;
    default:
      __debug(F("unknown ZZ"));
      break;
  }
  return resend;
}

boolean extendedYYcommand(int reply)
{
  boolean resend = false;
  switch (RECV(6 + reply)) {
    case 1: // mode change
      switch (RECV(7 + reply)) {
        case 1: // set main mode
          mode = 0; // TODO parser should be impremented: RECV(10 + reply) is always zero
          break;
        case 5: // set sub mode
          mode = 0; // TODO parser should be impremented: RECV(10 + reply) is always zero
          break;
      }
      break;
    case 2: // video
      switch (RECV(7 + reply)) {
        case 1: // default sub mode
          __debug(F("default sub mode: "));
          switch (RECV(10 + reply)) {
            case 0:
              __debug(F("video"));
              break;
            case 1:
              __debug(F("timelapse + video"));
              break;
            case 2:
              __debug(F("video + photo"));
              break;
            case 3:
              __debug(F("looping"));
              break;
          }
          break;
        case 27: // shutter button depressed. start
          if (reply) {
            // modify the reply to make a new command. DON'T USE buf[7..] as it is already filled by using the queue or the serial
            buf[0] = 15;
            RECV(0) = 5; RECV(1) = 2;
            RECV(2) = 12;
            RECV(3) = RECV(4) = 'Z';
            RECV(5) = ++session;
            RECV(14) = 0; RECV(13) = second; RECV(12) = minute; RECV(11) = hour;
            RECV(13)++;
            if (RECV(13) == 60) {
              RECV(13) = 0; RECV(12)++;
              if (RECV(12) == 60) {
                RECV(12) = 0; RECV(11)++;
                if (RECV(11) == 24) {
                  RECV(11) = 0;
                }
              }
            }
            memcpy_P((byte *)&(RECV(6)), F("\x04\x00\x01\x01\x01"), 5);
            i2cState = SESSION_IDLE;
            SendBufToCamera((byte *)&(RECV(0)));
          }
          break;
        case 28: // sync stop
          if (reply) {
            if (RECV(6) != STATUS_BUSY) {
              resend = true;
              break;
            }
            // modify the reply to make a new command. DON'T USE buf[7..] as it is already filled by using the queue or the serial
            buf[0] = 11;
            RECV(0) = 5; RECV(1) = 2;
            RECV(2) = 8;
            RECV(3) = RECV(4) = 'Z';
            RECV(5) = ++session;
            memcpy_P((byte *)&(RECV(6)), F("\x04\x00\x01\x01\x00"), 5);        
            i2cState = SESSION_IDLE;
            SendBufToCamera((byte *)&(RECV(0)));
          }
          break;
        case 38: // all video settings
          break;
      }
      break;
    case 3: // photo
      switch (RECV(7 + reply)) {
        case 1: // default sub mode
          __debug(F("default sub mode: "));
          switch (RECV(10 + reply)) {
            case 0:
              __debug(F("single"));
              break;
            case 1:
              __debug(F("continuous"));
              break;
            case 2:
              __debug(F("night"));
              break;
          }
          break;
        case 23: // shutter button depressed. start
          if (reply) {
            if (RECV(6) != STATUS_RECORDING) {
              resend = true;
            }
          }
          break;
        case 27: // all photo settings
          break;
      }
      break;
    case 4: // multi-shot
      switch (RECV(7 + reply)) {
        case 1: // default sub mode
          __debug(F("default sub mode: "));
          switch (RECV(10 + reply)) {
            case 0:
              __debug(F("burst"));
              break;
            case 1:
              __debug(F("timelapse"));
              break;
            case 2:
              __debug(F("night lapse"));
              break;
          }
          break;
        case 27: // shutter button depressed. start    
          break;
        case 32: // all multi-shot settings
          break;
      }
      break;
    case 7: // global settings
      switch (RECV(7 + reply)) {
        case 9: // orientation
          __debug(F("orientation: "));
          // there's a bug in v4.0.0 firmware that prevents from setting orientation to AUTO
          switch (RECV(10 + reply)) {
            case 0:
              __debug(F("Auto"));
              break;
            case 1:
              __debug(F("Up"));
              break;
            case 2:
              __debug(F("Down"));
              break;
          }
          break;
        case 11: // default mode
          __debug(F("default mode: "));
          switch (RECV(10 + reply)) {
            case 0:
              __debug(F("video"));
              break;
            case 1:
              __debug(F("photo"));
              break;
            case 2:
              __debug(F("multi-shot"));
              break;
          }
          break;
        case 13: // quick capture
          __debug(F("quick capture"));
          switch (RECV(10 + reply)) {
            case 0:
              __debug(F("off"));
              break;
            case 1:
              __debug(F("on"));
              break;
          }
          break;
        case 15: // LEDs
          __debug(F("LEDs: "));
          switch (RECV(10 + reply)) {
            case 0:
              __debug(F("off"));
              break;
            case 1:
              __debug(F("2"));
              break;
            case 2:
              __debug(F("4"));
              break;
          }
          break;
        case 17: // beeps
          __debug(F("beeps: "));
          switch (RECV(10 + reply)) {
            case 0:
              __debug(F("100%"));
              break;
            case 1:
              __debug(F("70%"));
              break;
            case 2:
              __debug(F("off"));
              break;
          }
          break;
        case 19: // video format
          switch (RECV(10 + reply)) {
            case 0:
              __debug(F("NTSC"));
              break;
            case 1:
              __debug(F("PAL"));
              break;
          }
          break;
        case 21: // OSD
          __debug(F("OSD"));
          break;
        case 23: // auto power down
          __debug(F("auto power down"));
          break;
        case 27: // date/time
          break;
        case 32: // language
          break;
        case 33: // all global settings
          // mode = RECV(36 + reply); // default_app_mode
          break;
      }
      break;
    case 9: // delete
      switch (RECV(7 + reply)) {
        case 9:
          __debug(F("delete last"));
          break;
        case 10:
          __debug(F("delete all/format"));
          break;
      }
      break;
    default:
      // unknown
      break;
  }

  return resend;
}

boolean extendedZZcommand(int reply)
{
  boolean resend = false;
  switch (RECV(6 + reply)) {
    case 0: // protocol revision
      // current protocol revison is 1 0 0
      break;
    case 1: // sync
      switch (RECV(7 + reply)) {
        case 1: // status request
          switch (RECV(8 + reply)) {
            case 0:
              if (reply) {
                if (mode != MODE_PHOTO) {
                  // video
                  if (RECV(6) != STATUS_RECORDING && RECV(6) != STATUS_IDLE) {
                    resend = true;
                  }
                } else {
                  // photo
                  // modify the reply to make a new command. DON'T USE buf[7..] as it is already filled by using the queue or the serial
                  buf[0] = 12;
                  RECV(0) = 5; RECV(1) = 2;
                  RECV(2) = 9;
                  RECV(3) = RECV(4) = 'Z';
                  RECV(5) = ++session;
                  memcpy_P(&(RECV(6)), F("\x04\x00\x01\x02\x02\x01"), 6);
                  i2cState = SESSION_IDLE;
                  SendBufToCamera(&(RECV(0)));
                }
              }
              break;
            case 1:
              if (reply) {
                if (RECV(6) != STATUS_RECORDING) {
                  resend = true;
                }
              }
              break;
            default:
              break;
          }
          break;
        case 2: // block until writing to microSD complete
          if (reply) {
            if (RECV(6) != STATUS_IDLE) {
              resend = true;
            }
          }
          break;
        default:
          break;
      }
      break;
    case 2: // Heartbeat
      if (!reply) {
        buf[0] = 12;
        RECV(0) = 0; RECV(1) = 1;
        RECV(2) = 9;
        RECV(3) = RECV(4) = 0;
        RECV(5) = ++session;
        memcpy_P((byte *)&(RECV(6)), F("\x00\x00\x00\x00\x02\x00"), 6);
        i2cState = SESSION_IDLE;
        SendBufToCamera((byte *)&(RECV(0)));    
      }
      break;
    case 3: // power off
      // never reach here
      break;
    case 5: // bacpac firmware version
      break;
    case 6: // bacpac serial number
      break;
    default:
      break;
  }
  return resend;
}

void checkBacpacCommands()
{
  boolean resend;
  if (recvc) {
    _printInput();
  
    resend = bacpacCommand();

    if (resend) {
      i2cState = SESSION_CMDBODY_SENT; // send reply request again
    } else {
      i2cState = SESSION_IDLE;
      if (queueState != QUEUE_HALT) {
        queueState = QUEUE_EMPTY;
      }
    }    
    recvc = 0;
  }
}
