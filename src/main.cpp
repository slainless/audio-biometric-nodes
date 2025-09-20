#define RECORDER_SAMPLE_RATE 16000

#define RECORDER_SD_PIN 32
#define RECORDER_SCK_PIN 33
#define RECORDER_FS_PIN 25

#include "core/audio.h"
#include "core/mqtt.h"
#include "core/serial.h"
#include "setup/wifi.h"

#include <Arduino.h>
#include <I2S.h>

Mqtt mqtt;
Recorder recorder(esp_i2s::I2S_NUM_0, RECORDER_SD_PIN, RECORDER_SCK_PIN,
                  RECORDER_FS_PIN);

void setup() {
  Serial.begin(115200);
  recorder.begin(RECORDER_SAMPLE_RATE);
}

void reconnectHandler() { reconnectMqtt(mqtt); }

void loop() {
  mqtt.poll(reconnectHandler);
  auto cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.equalsIgnoreCase("wifi"))
    return setupWiFi();
  if (cmd.equalsIgnoreCase("mqtt"))
    return configureMqtt(mqtt);
}