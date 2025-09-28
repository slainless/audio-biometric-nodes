#define RECORDER_SD_PIN 32
#define RECORDER_SCK_PIN 33
#define RECORDER_WS_PIN 25

#define BUILTIN_LED_PIN 2
#define PRINT_TASK_HIGHWATERMARK

#include <Arduino.h>
#include <esp_log.h>
#include <driver/i2s.h>

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

void readinessNotifierHandle(void *pvParameters)
{
  while (true)
  {
    controlledTaskWhenBusy(taskMutex, {
      vTaskDelay(pdMS_TO_TICKS(2000));
      return;
    });

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
          controlledTask(taskMutex, lastReconnectAttempt, 1000, {
            MqttConfigurer::reconnect(mqttConfig, mqtt);
          });
        });

    timedFor(lastHWCheck, 1000, {
      __printHighWaterMark;
    });

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

BaseType_t readinessNotifier;
BaseType_t mqttPoller;
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

  updateConfigToRemote(wifiConfig, mqttConfig);

  taskMutex = xSemaphoreCreateMutex();
  // readinessNotifier = xTaskCreate(readinessNotifierHandle, "ReadinessNotifier", 2048, nullptr, 1, nullptr);
  // mqttPoller = xTaskCreate(mqttPollerHandle, "MqttPoller", 2048, nullptr, 1, nullptr);
}

auto lastConfig = millis();
void loop()
{
  RemoteXY_Handler();
  if (RemoteXY.button_store_config != LOW)
  {
    controlledTask(taskMutex, lastConfig, 200, {
      ESP_LOGI(TAG, "Storing configuration");
      auto res = storeConfig(wifiConfig, mqttConfig);
      if (res == RemoteXYConfigCode::MISSING_WIFI_SSID)
      {
        sprintf(RemoteXY.value_config_status, "Missing WiFi SSID");
      }
      else if (res == RemoteXYConfigCode::MISSING_WIFI_PASSWORD)
      {
        sprintf(RemoteXY.value_config_status, "Missing WiFi Password");
      }
      else if (res == RemoteXYConfigCode::MISSING_MQTT_HOST)
      {
        sprintf(RemoteXY.value_config_status, "Missing MQTT Host");
      }
      else if (res == RemoteXYConfigCode::MISSING_MQTT_PORT)
      {
        sprintf(RemoteXY.value_config_status, "Missing MQTT Port");
      }

      if (res != RemoteXYConfigCode::OK)
      {
        ESP_LOGI(TAG, "Fail to store configuration, caused by: %d", res);
        controlledReturn;
      }

      ESP_LOGI(TAG, "Configuration stored.");
      auto wifiError = WiFiConfigurer::reconnect(wifiConfig);
      if (wifiError != ESP_OK)
      {
        ESP_LOGI(TAG, "Fail to store configure WiFi, caused by: %d", wifiError);
        sprintf(RemoteXY.value_config_status, "WiFi Error. Check SSID/Password");
        controlledReturn;
      }

      auto mqttError = MqttConfigurer::reconnect(mqttConfig, mqtt);
      if (mqttError != ESP_OK)
      {
        ESP_LOGI(TAG, "Fail to store configure MQTT, caused by: %d", mqttError);
        sprintf(RemoteXY.value_config_status, "MQTT Error. Check Host/Port");
        controlledReturn;
      }

      sprintf(RemoteXY.value_config_status, "Connected");
      ESP_LOGI(TAG, "WiFi and MQTT successfully configured");
    });
  }
}