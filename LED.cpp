#include <Arduino.h>

boolean ledState;

const int LED_OUT          = 13; // Arduino onboard LED; HIGH (= ON) while recording

void ledOff()
{
  digitalWrite(LED_OUT, LOW);
  ledState = false;
}

void ledOn()
{
  digitalWrite(LED_OUT, HIGH);
  ledState = true;
}

void setupLED()
{
  pinMode(LED_OUT, OUTPUT);
  ledOff();
}
