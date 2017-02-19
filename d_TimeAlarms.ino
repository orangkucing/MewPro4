// Time alarm video/photo driven by MewPro

#ifdef USE_TIME_ALARMS

boolean alarmEntered = false;   // in order to avoid timeAdjust() or alarmPowerOn() functions called twice
const int syncDelay = 600; // more than 480 seconds (= 8 minutes). workaround for time drift on Arduino
const int powerOnDelay = 30;             // delay in second after camera power on 
const int powerOffDelay = 5;             // delay in second before camera power off

// Since the time measured by Arduino's Time library gains or loses maximum 20 seconds per hour, we need to sync the time with GoPro's RTC at least once a day.
void timeAdjust()
{
  if (!alarmEntered) {
    alarmEntered = true;
    queueIn((char *)F("@")); // power on camera
  }
}

void alarmFlagClear()
{
  alarmEntered = false;
}

// Alarm tasks
void alarmPowerOn()
{
  queueIn((char *)F("@")); // power on camera
  Alarm.timerOnce(powerOnDelay, alarmStartRecording);
}

void alarmStartRecording()
{
  startRecording();
/***********************
Example alarm: Shooting duration in seconds
 *** EDIT NEXT LINE ****/
  Alarm.timerOnce(5 * 60, alarmStopRecording);  // 5 minutes
/***********************/
}

void alarmStopRecording()
{
  stopRecording();
  Alarm.timerOnce(5, alarmSuspend);  // power off at powerOffDelay seconds after stop
}

void alarmSuspend()
{
  queueIn((char *)F("ZZ00030101")); // power off camera
  // alarmEntered == true for a long enough time > 8 minute (> 20 * 24 seconds).
  if (alarmEntered) {
    Alarm.timerOnce(syncDelay, alarmFlagClear);
  }
}

void _setTime(int base) // This function is called every time just after the camera power on
{
  TimeElements tm;
  tm.Year = (RECV(11 + base) * 256 + RECV(12 + base)) % 100; tm.Month = RECV(13 + base); tm.Day = RECV(14 + base);
  tm.Hour = RECV(15 + base); tm.Minute = RECV(16 + base); tm.Second = RECV(17 + base);
  setTime(makeTime(tm));
  for (uint8_t id = 0; id < dtNBR_ALARMS; id++) {
    // delete all alarms
    if (Alarm.isAlarm(id)) {
      Alarm.free(id);
    }
  }

/***********************
Example alarm: Shooting video from 09:00
 *** EDIT NEXT LINE ****/
  tm.Hour = 9; tm.Minute = 0; tm.Second = 0; // 09:00:00  Recording start time
/***********************/

  time_t t = makeTime(tm), s;
  // Arduino's time is adjusted whenever GoPro power on.
  // Sync the time once a day. Take care the time before power-on has drifted a maximum of 8 minute!
  s = t - 2 * syncDelay - 3 * powerOnDelay;
  Alarm.alarmRepeat(hour(s), minute(s), second(s), timeAdjust);
  //
  // just after sync with GoPro, the time is accurate enough. let's power on again and shoot video.
  t -= powerOnDelay; // powerOnDelay seconds before recording start
/***********************
 *** EDIT NEXT AND CHOOSE EITHER ALARM 1 or 2 or 3 ****/
/***********************
Example alarm 1: Shooting video everyday */
  Alarm.alarmRepeat(hour(t), minute(t), second(t), alarmPowerOn);
/***********************
Example alarm 2: Shooting video weekly on Tuesday */
//  Alarm.alarmRepeat(dowTuesday, hour(t), minute(t), second(t), alarmPowerOn);
/***********************
Example alarm 3: Shooting video on Monday and Wednesday and Friday */
//  Alarm.alarmRepeat(dowMonday, hour(t), minute(t), second(t), alarmPowerOn);
//  Alarm.alarmRepeat(dowWednesday, hour(t), minute(t), second(t), alarmPowerOn);
//  Alarm.alarmRepeat(dowFriday, hour(t), minute(t), second(t), alarmPowerOn);
/***********************/
  if (alarmEntered) {
    Alarm.timerOnce(powerOnDelay, alarmSuspend);
  }
}

void checkTimeAlarms()
{
  Alarm.delay(0); // Don't delete. Alarms are serviced from here.
}

#else

void _setTime(int base)
{
  
}

void checkTimeAlarms()
{
  
}

#endif /* USE_TIME_ALARMS */
