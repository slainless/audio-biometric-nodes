#define LAMP_SWITCH_PIN 15
#define FAN_SWITCH_PIN 18

#include <Arduino.h>
#include <esp_log.h>

#include "core/remotexy.h"

#include "core/mqtt.h"
#include "core/wifi.h"
#include "mqtt/protocol.h"
#include "core/filesystem.h"
#include "core/utils.h"
#include "core/control.h"

#include "device/controller/controller.h"

WiFiConfig wifiConfig;
MqttConfig mqttConfig;

Mqtt mqtt(CONTROLLER_IDENTIFIER);

static SemaphoreHandle_t taskMutex = nullptr;
createTag(MAIN);

void setup()
{
  esp_log_level_set("*", ESP_LOG_DEBUG);

  RemoteXY_Init();
  Serial.begin(115200);
  pinMode(LAMP_SWITCH_PIN, OUTPUT);
  pinMode(FAN_SWITCH_PIN, OUTPUT);

  int code;
  ensureSetup(code, FileSystem::setup(), "SPIFFS");
  ensureSetup(code, WiFiConfigurer::setup(wifiConfig), "WiFi");
  ensureSetup(code, MqttConfigurer::setup(mqttConfig, mqtt), "MQTT");
  subscribeToCommand(mqtt, LAMP_SWITCH_PIN, FAN_SWITCH_PIN);

  RemoteXYConfigurer::updateConfigToRemote(wifiConfig, mqttConfig);

  taskMutex = xSemaphoreCreateMutex();
}

auto lastReconnectAttempt = millis();
auto lastConfig = millis();

void loop()
{
  RemoteXY_Handler();
  if (RemoteXY.button_store_config != LOW)
  {
    controlledTask(taskMutex, lastConfig, 1000, {
      RemoteXYConfigurer::configureNetwork(wifiConfig, mqttConfig, mqtt);
    });
  }

  mqtt.poll(
      []
      {
        controlledTask(taskMutex, lastReconnectAttempt, 1000, {
          MqttConfigurer::reconnect(mqttConfig, mqtt);
          subscribeToCommand(mqtt, LAMP_SWITCH_PIN, FAN_SWITCH_PIN);
        });
      });
}