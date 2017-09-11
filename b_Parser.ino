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

char tmp[20];

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
      Serial.println(F(" ** collision detected: print old buf")); Serial.print(' ');
      int buflen = tmp[0];
      int i = 1;
      while (i <= buflen) {
        printHex(tmp[i], false);
        Serial.print(' ');
        i++;
      }
      Serial.println("");
    } 
  } else {
    // a short delay is necessary here
    // if collisions occur while the delay then newly received data must be used
    delay(3);
  }
}

void SendToBastet()
{
  int buflen = RECV(0);
  if (RECV(2) == 0) { 
    // received packet is the reply for the previous command. Dual Hero only
    return;
  }
  Serial.write(RECV(1));
  Serial.write(RECV(2));
  // no need to send following two bytes
  // RECV(3) : session #
  // RECV(4) : constant 6 for YY or 4 for ZZ
  for (int i = 5; i <= buflen; i++) {
    printHex(RECV(i), false);
  }
  Serial.write('\n');
}

// parse command/reply that is received from camera through the herobus
boolean parseI2C_R()
{
  boolean resend = false;
  int base = isOmni() ? 2 : 0;
  _printInput();
  switch (RECV(4)) {
    case 0: // received packet is the reply for the previous ZZ command
      resend = ZZcommand_R(base);
      break;
    case 2: // received packet is the reply for the previous YY command
      resend = YYcommand_R(base);
      break;
    case 4:
      ZZcommand_R(0); // ZZ command received. Dual Hero only
      break;
    case 6:
      SendToBastet();
      YYcommand_R(0); // YY command received. Dual Hero only
      break;
  }
  return resend;
}

boolean YYcommand_R(int base)
{
  boolean resend = false;
  switch (RECV(5 + base)) {
    default:
      resend = extendedYYcommand_R(base);
      break;
  }
  return resend;
}

boolean ZZcommand_R(int base)
{
  boolean resend = false;
  switch (RECV(5 + base)) {
    case 1:
      switch (RECV(6 + base)) {
        case 0:
          if (RECV(4) == 4) { // ZZ received
            // Dual Hero only
            RECV(2) = 11;
            memcpy_P((byte *)&(RECV(3)), F("\x00\x00\x00\x00\x00\x00\x01\x03\x01\x00\x00"), 11);
            i2cState = SESSION_IDLE;
            SendBufToCamera((byte *)&(RECV(0)));
            startupSession = 0; queueState = QUEUE_EMPTY; // start startup sequences
            startUp = (char **)default_startUp;
          }
          break;
      }
      break;
    case 2: // camera power
      switch (RECV(6 + base)) {
        case 1:
          __debug(F("power on"));
          if (RECV(4) == 4) { // ZZ received
            // Dual Hero only
            RECV(2) = 8;
            memcpy_P((byte *)&(RECV(3)), F("\x00\x00\x00\x00\x00\x00\x02\x00"), 8);
            i2cState = SESSION_IDLE;
            SendBufToCamera((byte *)&(RECV(0)));                      
            // send '@' command to Bastet
            Serial.write('\n');
            Serial.write('@');
            Serial.write('\n');                     
          }
          break;
      }
      break;
    case 3: // get camera version
      if (debug) {
        Serial.print(F("version: "));
        Serial.println((char *)&RECV(7 + base));
      }
      break;
    case 0: // extended command
      resend = extendedZZcommand_R(base);
      break;
  }
  return resend;
}

boolean extendedYYcommand_R(int base)
{
  boolean resend = false;
  switch (RECV(6 + base)) {
    case 1: // mode change
      switch (RECV(7 + base)) {
        case 0: // get main mode
          mode = RECV(11 + base);
          break;
        case 1: // set main mode
          mode = RECV(10 + base);
          break;
        case 5: // set sub mode. argc = 2; main, sub
          mode = RECV(10 + base);
          break;
      }
      break;
    case 2: // video
      switch (RECV(7 + base)) {
        case 1: // default sub mode
          break;
        case 27: // shutter button depressed. start
          if (isOmni()) {
            // Omni only
            // modify the reply to make a new command. DON'T USE buf[7..] as it is already filled by using the queue or the serial
            buf[0] = 15;
            RECV(0) = 5; RECV(1) = 2;
            RECV(2) = 12;
            RECV(3) = RECV(4) = 'Z';
            RECV(5) = ++session;
            RECV(14) = 0; RECV(13) = omni_second; RECV(12) = omni_minute; RECV(11) = omni_hour;
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
          if (isOmni()) {
            // Omni only
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
        case 38: // bulk transfer video settings
          break;
      }
      break;
    case 3: // photo
      switch (RECV(7 + base)) {
        case 1: // default sub mode
          break;
        case 23: // shutter button depressed. start
          if (isOmni()) {
            // Omni only
            if (RECV(6) != STATUS_RECORDING) {
              resend = true;
            }
          }
          break;
        case 27: // bulk transfer photo settings
          break;
      }
      break;
    case 4: // multi-shot
      switch (RECV(7 + base)) {
        case 1: // default sub mode
          break;
        case 27: // shutter button depressed. start
          break;
        case 32: // bulk transfer multi_shot settings
          break;
      }
      break;
    case 7: // global settings
      switch (RECV(7 + base)) {
        case 26: // get datetime
          _setTime(base);
          break;
        case 33: // bulk transfer
          break;
      }
      break;
    case 9: // delete
      break;
  }
  return resend;
}

boolean extendedZZcommand_R(int base)
{
  boolean resend = false;
  switch (RECV(6 + base)) {
    case 0: // init external sync
      break;
    case 1: // sync
      switch (RECV(7 + base)) {
        case 1: // status request
          switch (RECV(8 + base)) {
            case 0:
              if (isOmni()) {
                // Omni only
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
              } else if (RECV(4) == 4) { // ZZ received
                // Dual Hero only
                RECV(2) = 10;
                RECV(3) = RECV(4) = 0;
                RECV(5) = 0;
                memcpy_P((byte *)&(RECV(6)), F("\x00\x00\x00\x00\x01\x01\x00"), 7);
                i2cState = SESSION_IDLE;
                SendBufToCamera((byte *)&(RECV(0)));                                    
              }
              break;
            case 1:
              if (isOmni()) {
                // Omni only
                if (RECV(6) != STATUS_RECORDING) {
                  resend = true;
                }
              } else if (RECV(4) == 4) { // ZZ received
                // Dual Hero only
                RECV(2) = 10;
                RECV(3) = RECV(4) = 0;
                RECV(5) = 0;
                memcpy_P((byte *)&(RECV(6)), F("\x00\x00\x00\x00\x01\x01\x01"), 7);
                i2cState = SESSION_IDLE;
                SendBufToCamera((byte *)&(RECV(0)));                    
              }
              break;
          }
          break;
        case 2: // block until writing to microSD complete
          if (isOmni()) {
            // Omni only
            if (RECV(6) != STATUS_IDLE) {
              resend = true;
            }
          }
          break;
        case 7: // sync signal request (ID_MASTER only)
          // after depressing shutter camera sends parameters to Dual Hero
          // however, received parameter is buggy and completely unusable
          // RECV(9+base:10+base)  : fps * 100
          // RECV(11+base:12+base) : number of HSYNC pulses in one frame
          // RECV(13+base:14+base) : ditto
          break;
      }
      break;
    case 2: // Heartbeat
      if (RECV(4) == 4) { // ZZ received
        // Dual Hero only
        RECV(2) = 9;
        RECV(3) = RECV(4) = 0;
        RECV(5) = 0;
        memcpy_P((byte *)&(RECV(6)), F("\x00\x00\x00\x00\x02\x00"), 6);
        i2cState = SESSION_IDLE;
        SendBufToCamera((byte *)&(RECV(0)));    
      }
      break;
    case 3: // power off
      SendToBastet();
      break;
    case 5: // bacpac firmware version
      break;
    case 6: // bacpac serial number
      break;
  }
  return resend;
}

// parse command that is sent to camera through the herobus
void parseI2C_W(byte *p)
{
  switch ((p[3] << 8) + p[4]) {
    case ('Y' << 8) + 'Y':
      YYcommand_W(p);
      break;
    case ('Z' << 8) + 'Z':
      ZZcommand_W(p);
      break;
  }
}

void YYcommand_W(byte *p)
{
  switch (p[7]) {
    default:
      extendedYYcommand_W(p);
      break;
  }
  p[7] = bootID;
}

void ZZcommand_W(byte *p)
{
  switch (p[7]) {
    case 1: // Dual Hero only
      break;
    case 2: // camera power
      break;
    case 3: // get camera version
      break;
    case 0: // extended command
      extendedZZcommand_W(p);
      break;
  }
}

void extendedYYcommand_W(byte *p)
{
  switch (p[8]) {
    case 1: // mode change
      switch (p[9]) {
        case 1: // set main mode
          mode = p[12];
          break;
        case 5: // set sub mode. argc = 2: main, sub
          mode = p[12];
          break;
      }
      break;
    case 2: // video
      switch (p[9]) {
        case 1: // default sub mode
          break;
        case 3: // argc = 3: resolution, fps, fov
          break;
        case 5: // piv. Note: not generated by ID_MASTER (default firmware's bug)
          break;
        case 7: // looping
          break;
        case 9: // low_light. Note: not generated by ID_MASTER (default firmware's bug)
          break;
        case 11: // spot meter
          break;
        case 13: // timelapse_rate
          break;
        case 15: // protune
          break;
        case 17: // protune_white_balance
          break;
        case 19: // protune_color
          break;
        case 21: // protune_sharpness
          break;
        case 23: // protune_iso
          break;
        case 25: // protune_ev
          break;
        case 26: // reset protune. argc = 0
          break;
        case 27: // shutter button depressed. start
          if (isOmni()) {
            // keep current time for future reference
            omni_hour = p[13]; omni_minute = p[14]; omni_second = p[15];
          } else {
            // no need to send the current time. truncate the packet
            p[11] = 0; p[2] = 9;
          }
          break;
        case 28: // sync stop
          break;
        case 38: // bulk transfer video settings
          break;
        case 40: // exposure_time
          break;
        case 42: // protune_iso_mode
          break;
      }
      break;
    case 3: // photo
      switch (p[9]) {
        case 1: // default sub mode
          break;
        case 3: // resolution
          break;
        case 5: // continuous_rate
          break;
        case 7: // spot meter
          break;
        case 9: // exposure_time
          break;
        case 11: // protune
          break; 
        case 13: // protune_white_balance
          break;
        case 15: // protune_color
          break;
        case 17: // protune_sharpness
          break;
        case 19: // protune_iso
          break;
        case 21: // protune_ev
          break;
        case 22: // reset protune. argc = 0
          break;
        case 23: // shutter button depressed. start
          break;
        case 24: // stop (continuous sub mode only)
          break;
        case 27: // bulk transfer photo settings
          break;
        case 29: // protune_iso_min
          break;
      }
      break;
    case 4: // multi-shot
      switch (p[9]) {
        case 1: // default sub mode
          break;
        //
        // ID_MASTER doesn't generate any of these setting commands (default firmware's bug)
        case 3: // resolution
          break;
        case 5: // burst_rate
          break;
        case 7: // timelapse_rate
          break;
        case 9: // nightlapse_rate
          break;
        case 11: // spot_meter
          break;
        case 13: // exposure_time
          break;
        case 15: // protune
          break;
        case 17: // protune_white_balance
          break;
        case 19: // protune_color
          break;
        case 21: // protune_sharpness
          break;
        case 23: // protune_iso
          break;
        case 25: // protune_ev
          break;
        case 26: // reset protune. arg = 0
          break;
        //
        //
        case 27: // shutter button depressed. start 
          break;
        case 28: // stop
          break;
        case 32: // bulk transfer multi-shot settings
          break;
        // ID_MASTER doesn't generate the following setting command (default firmware's bug)
        case 34: // protune_iso_min (v4)
          break;
      }
      break;
    case 7: // global settings
      switch (p[9]) {
        case 1: // LCD brightness. Note: not generated by ID_MASTER (default firmware's bug)
          break;
        case 3: // LCD sleep. Note: not generated by ID_MASTER (default firmware's bug)
          break;
        case 5: // LCD lock. Note: not generated by ID_MASTER (default firmware's bug)
          break;
        case 7: // LCD power. Note: not generated by ID_MASTER (default firmware's bug)
          break;
        case 9: // orientation
                // ID_MASTER generates "UP" or "DOWN", which is the SAME to its orientation, even when it is actually set to "AUTO" (default firmware's bug)
#if defined(DUAL_HERO_ORIENTATION) && !defined(BASTET_MASTER)
          p[12] = 2; // force orientation DOWN
#endif
          break;
        case 11: // default_app_mode
          break;
        case 13: // quick_capture
          break;
        case 15: // led
          break;
        case 17: // beep_volume
          break;
        case 19: // video_format
          break;
        case 21: // osd
          break;
        case 23: // auto_power_down
          break;
        case 27: // argc = 7: year_high, year_low, month, date, hour, minute, second
          break;
        case 32: // language
          break;
        case 33: // bulk transfer global settings (due to the gopro's firmware bug camera might ignore some settings including orientation)
          break;
      }
      break;
    case 9: // delete
      switch(p[9]) {
        case 9: // delete last. argc = 0
          break;
        case 10: // delete all. argc = 0
          break;
      }
      break;
  }
}

void extendedZZcommand_W(byte *p)
{
  switch (p[8]) {
    case 0:
      //                                         p[9]  p[10] p[11]
      // external sync signal for video/photo if 1     0     *
      // standalone                           otherwise
      break;
    case 1: // sync
      break;
    case 2: // Heartbeat
      break;
    case 3: // power off
      break;
    case 5: // bacpac firmware version
      break;
    case 6: // bacpac serial number
      break;
  }
}

