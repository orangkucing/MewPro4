// queue state
const int QUEUE_HALT  = -1; // initial state or startup sequence finished
const int QUEUE_BUSY  = 0;  // a command has been queued
const int QUEUE_EMPTY = 1;  // queue empty
volatile char queueState = QUEUE_HALT;
char startupSession;
char **startUp; // pointer to startup sequence array

// start up sequence for Omni firmware
const char omni_session_0[] PROGMEM = "ZZ0000010000"; // external sync signal required to shoot video/photo
const char omni_session_1[] PROGMEM = "ZZ00050300000500";

const char* const omni_startUp[] PROGMEM = {
  omni_session_0, omni_session_1, NULL
};

// start up sequence for camera's default firmware
const char set_lowlight_off[]  PROGMEM = "YY000209000100"; // lowlight off
const char default_session_0[] PROGMEM = "ZZ0000010000"; // external sync
const char default_session_1[] PROGMEM = "ZZ0000010100"; // standalone
const char default_session_2[] PROGMEM = "YY0001000000"; // get current mode
const char default_session_3[] PROGMEM = "YY00071a0000"; // get datetime

const char* const default_startUp[] PROGMEM = {
#ifdef USE_GENLOCK
  set_lowlight_off,
  default_session_0,
#else
  default_session_1,
  default_session_2,
#  ifdef USE_TIME_ALARMS
  default_session_3,
#  endif /* USE_TIME_ALARMS */
#endif /* USE_GENLOCK */
  NULL
};
