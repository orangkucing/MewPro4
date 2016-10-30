// queue state
const int QUEUE_HALT  = -1; // initial state or startup sequence finished
const int QUEUE_BUSY  = 0;  // a command has been queued
const int QUEUE_EMPTY = 1;  // queue empty
volatile char queueState = QUEUE_HALT;
char startupSession;
char **startUp; // pointer to startup sequence array

// start up sequence for Omni firmware
const char omni_session_0[] PROGMEM = "ZZ0000010000";
const char omni_session_1[] PROGMEM = "ZZ00050300000500";

const char* const omni_startUp[] PROGMEM = {
  omni_session_0, omni_session_1, NULL
};

// start up sequence for camera's default firmware
const char default_session_0[] PROGMEM = "ZZ0000010000";

const char* const default_startUp[] PROGMEM = {
  default_session_0, NULL
};
