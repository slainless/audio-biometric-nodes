#include <Arduino.h>
#include <SPIFFS.h>

#include "core/mqtt.h"
#include "mqtt/protocol.h"
#include "setup/wifi.h"
#include "setup/spiffs.h"

#include "device/controller/controller.h"

#define SWITCH_PIN 15

Mqtt mqtt(CONTROLLER_IDENTIFIER);

void subscribeMqtt()
{
  mqtt.subscribe(
      MqttTopic::CONTROLLER,
      [](const char *msg)
      {
        if (strcmp(msg, MqttControllerCommand::ON) == 0)
        {
          digitalWrite(SWITCH_PIN, HIGH);
        }
        else if (strcmp(msg, MqttControllerCommand::OFF) == 0)
        {
          digitalWrite(SWITCH_PIN, LOW);
        }
      });
}

void mqttReconnectHandler()
{
  reconnectMqtt(mqtt);
  subscribeMqtt();
}

void configurerTask(void *pvParameters)
{
  while (1)
  {
    if (!mqtt.client)
    {
      Serial.println("MQTT client is not initialized, please setup mqtt");
    }

    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WiFi is not connected, please setup wifi");
    }

    auto cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("wifi"))
      return configureWiFi();
    if (cmd.equalsIgnoreCase("mqtt"))
      return mqttReconnectHandler();
  }
}

void mqttPoolerTask(void *pvParameters)
{
  while (1)
  {
    mqtt.poll(mqttReconnectHandler);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(SWITCH_PIN, OUTPUT);

  setupSPIFFS();
  setupWiFi();
  setupMqtt(mqtt);
  subscribeMqtt();

  xTaskCreate(configurerTask, "Configurer", 2048, nullptr, 1, nullptr);
  xTaskCreate(mqttPoolerTask, "MqttPooler", 2048, nullptr, 1, nullptr);
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(portMAX_DELAY));
}