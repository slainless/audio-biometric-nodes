#include "setup/wifi.h"
#include <Arduino.h>

void setup() { Serial.begin(115200); }

void loop() {
  auto cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.equalsIgnoreCase("wifi"))
    return setupWiFi();
}