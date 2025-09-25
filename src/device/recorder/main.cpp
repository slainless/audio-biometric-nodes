#define RECORDER_SAMPLE_RATE 8000

#define RECORDER_SD_PIN 32
#define RECORDER_SCK_PIN 33
#define RECORDER_WS_PIN 25

#define BUILTIN_LED_PIN 2

#include "core/mqtt.h"
#include "core/record.h"
#include "setup/wifi.h"
#include "setup/spiffs.h"

#include "device/recorder/recorder.h"

#include <Arduino.h>
#include <SPIFFS.h>
#include <driver/i2s.h>

Mqtt mqtt(RECORDER_IDENTIFIER);
Recorder recorder(I2S_NUM_0, RECORDER_SD_PIN, RECORDER_SCK_PIN,
                  RECORDER_WS_PIN);

void setup()
{
  pinMode(BUILTIN_LED_PIN, OUTPUT);
  Serial.begin(115200);
  recorder.begin(RECORDER_SAMPLE_RATE);

  setupSPIFFS();
  setupWiFi();
  setupMqtt(mqtt);
}

void reconnectHandler() { reconnectMqtt(mqtt); }

void loop()
{
  if (!mqtt.client)
  {
    Serial.println("MQTT client is not initialized, please setup mqtt");
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi is not connected, please setup wifi");
  }

  mqtt.poll(reconnectHandler);

  auto cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.equalsIgnoreCase("wifi"))
    return configureWiFi();
  if (cmd.equalsIgnoreCase("mqtt"))
    return configureMqtt(mqtt);
  if (cmd.equalsIgnoreCase("record"))
    return recordToMqtt(recorder, mqtt, BUILTIN_LED_PIN);
}