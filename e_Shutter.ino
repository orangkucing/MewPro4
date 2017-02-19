// Interface to CANON Timer Remote Controller TC-80N3
// Also works with similar remote shutters.
// Note: Remote shutters are different from simple mechanical switches that bounce or chatter when turning on/off.
#ifdef USE_SHUTTER

// ATtiny1634 has only one external interrupt pin; Don't edit this line
const int SHUTTER_PIN      = 11;   // INT0             Interrupt pin w/o software debounce

void shutterHandler()
{
  if (digitalRead(SHUTTER_PIN) == LOW) {
    startRecording();
  } else {
    stopRecording();
  }
}

void setupShutter()
{
  pinMode(SHUTTER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SHUTTER_PIN), shutterHandler, CHANGE); 
}

#else

void setupShutter()
{
}

#endif /* USE_SHUTTER */
