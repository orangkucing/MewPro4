boolean inputAvailable()
{
  if (queueidx != -1 || Serial.available()) {
    return true;
  }
  return false;
}

int unreadBuf = -1;
void myUnread(char c)
{
  unreadBuf = c & 0xFF;
}

char myRead()
{
  char c;
  if (unreadBuf != -1) {
    c = (char)unreadBuf;
    unreadBuf = -1;
    return c;
  }
  if (serialfirst && Serial.available()) {
    return Serial.read();
  }
  if (queueidx != -1) {
    c = (char)pgm_read_byte(queueadr + queueidx++);
    if (!c) {
      queueidx = -1;
      c = '\n';
    }
    serialfirst = false;
    return c;
  }
  serialfirst = true;
  return Serial.read();
}

// Utility functions
void queueIn(char *p)
{
  queueadr = p;
  queueidx = 0;
}

void __emptyInputBuffer()
{
  while (inputAvailable()) {
    if (myRead() == '\n') {
      return;
    }
  }
}

void emptyQueue()
{
  queueidx = -1;
  recvc = 0;
  bufp = 6;
  serialfirst = false;
  while (Serial.available()) { // clear serial buffer
    Serial.read();
  }
}
