#include <Arduino.h>

String blockingReadStringUntil(char terminator) {
  String result = "";
  while (true) {
    while (Serial.available() == 0) {
      yield();
    }
    char c = Serial.read();
    if (c == terminator)
      break;
    result += c;
  }
  return result;
}