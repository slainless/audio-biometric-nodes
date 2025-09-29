#define RECORDER_SD_PIN 32
#define RECORDER_SCK_PIN 33
#define RECORDER_WS_PIN 25

#define BUILTIN_LED_PIN 2
#define PRINT_TASK_HIGHWATERMARK

#include <Arduino.h>
#include <esp_log.h>
#include <driver/i2s.h>

#define REMOTEXY_BLUETOOTH_NAME "ESP32Recorder"
#include "core/remotexy.h"

#include "core/mqtt.h"
#include "core/record.h"
#include "core/wifi.h"
#include "core/filesystem.h"
#include "core/utils.h"
#include "core/control.h"

#include "device/recorder/recorder.h"

WiFiConfig wifiConfig;
MqttConfig mqttConfig;

Mqtt mqtt(RECORDER_IDENTIFIER);
Recorder recorder(I2S_NUM_0, RECORDER_SD_PIN, RECORDER_SCK_PIN,
                  RECORDER_WS_PIN);

static SemaphoreHandle_t taskMutex = nullptr;
createTag(MAIN);

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
  subscribeToVerifyResult(mqtt);

  RemoteXYConfigurer::updateConfigToRemote(wifiConfig, mqttConfig);
  RemoteXYConfigurer::resetVerifyResult();

  taskMutex = xSemaphoreCreateMutex();
}

auto lastReconnectAttempt = millis();
auto lastConfig = millis();
auto lastRecording = millis();
auto lastSampling = millis();
auto lastSamplingTry = millis();

void loop()
{
  RemoteXY_Handler();
  if (RemoteXY.button_store_config != LOW)
  {
    controlledTask(taskMutex, lastConfig, 1000, {
      RemoteXYConfigurer::configureNetwork(wifiConfig, mqttConfig, mqtt);
    });
  }

  if (RemoteXY.button_recorder != LOW)
  {
    controlledTask(taskMutex, lastRecording, 6000, {
      RemoteXYConfigurer::resetVerifyResult();
      Record::verify(recorder, mqtt, BUILTIN_LED_PIN);
    });
  }

  if (RemoteXY.button_sampler != LOW)
  {
    timedFor(lastSamplingTry, 300, {
      auto sampleName = String(RemoteXY.input_voice_name);
      sampleName.trim();
      if (sampleName.charAt(0) != '\0')
      {
        controlledTask(taskMutex, lastSampling, 6000, {
          Record::sample(recorder, mqtt, BUILTIN_LED_PIN, sampleName.c_str());
        });
      }
      else
      {
        sprintf(RemoteXY.value_sampler_status, "Empty sample name");
      }
    });
  }

  mqtt.poll(
      []
      {
        controlledTask(taskMutex, lastReconnectAttempt, 1000, {
          MqttConfigurer::reconnect(mqttConfig, mqtt);
          subscribeToVerifyResult(mqtt);
        });
      });
}