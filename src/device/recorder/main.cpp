#define RECORDER_SD_PIN 32
#define RECORDER_SCK_PIN 33
#define RECORDER_WS_PIN 25

#define BUILTIN_LED_PIN 2

#include "core/mqtt.h"
#include "core/record.h"
#include "core/wifi.h"
#include "core/filesystem.h"
#include "core/utils.h"

#include "device/recorder/recorder.h"
#include "device/recorder/control.h"

#include <Arduino.h>
#include <esp_log.h>
#include <driver/i2s.h>

WiFiConfig wifiConfig;
MqttConfig mqttConfig;

Mqtt mqtt(RECORDER_IDENTIFIER);
Recorder recorder(I2S_NUM_0, RECORDER_SD_PIN, RECORDER_SCK_PIN,
                  RECORDER_WS_PIN);

void readinessNotifierHandle(void *pvParameters)
{
  while (true)
  {
    if (!mqtt.client)
    {
      Serial.println("MQTT client is not initialized, please setup mqtt");
    }

    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WiFi is not connected, please setup wifi");
    }

    __printHighWaterMark;
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void mqttPollerHandle(void *pvParameters)
{
  auto lastReconnectAttempt = millis();
  auto lastHWCheck = millis();
  while (true)
  {
    mqtt.poll(
        [&lastReconnectAttempt]
        {
          if (millis() - lastReconnectAttempt >= 1000)
          {
            MqttConfigurer::reconnect(mqttConfig, mqtt);
            lastReconnectAttempt = millis();
          }
        });

    if (millis() - lastHWCheck >= 1000)
    {
      __printHighWaterMark;
      lastHWCheck = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup()
{
  esp_log_level_set("*", ESP_LOG_DEBUG);

  RemoteXY_Init();
  pinMode(BUILTIN_LED_PIN, OUTPUT);
  Serial.begin(115200);
  recorder.begin(RECORDER_SAMPLE_RATE);

  int code;
  ensureSetup(code, FileSystem::setup(), "SPIFFS");
  ensureSetup(code, WiFiConfigurer::setup(wifiConfig), "WiFi");
  ensureSetup(code, MqttConfigurer::setup(mqttConfig, mqtt), "MQTT");

  xTaskCreate(readinessNotifierHandle, "ReadinessNotifier", 1024, nullptr, 1, nullptr);
  xTaskCreate(mqttPollerHandle, "MqttPoller", 2048, nullptr, 1, nullptr);
}

void loop()
{
  RemoteXY_Handler();
}