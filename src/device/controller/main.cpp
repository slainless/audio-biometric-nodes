#include <Arduino.h>

#include "core/mqtt.h"
#include "mqtt/protocol.h"
#include "setup/wifi.h"
#include "setup/spiffs.h"

#include "device/controller/controller.h"

#include <SPIFFS.h>

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

void setup()
{
  Serial.begin(115200);
  pinMode(SWITCH_PIN, OUTPUT);

  setupSPIFFS();
  setupWiFi();
  setupMqtt(mqtt);
  subscribeMqtt();
}

void reconnectHandler()
{
  reconnectMqtt(mqtt);
  subscribeMqtt();
}

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
  {
    configureMqtt(mqtt);
    subscribeMqtt();
  }
}