#include "core/mqtt.h"
#include "mqtt/protocol.h"
#include "setup/wifi.h"

#include "device/controller/controller.h"

#include <SPIFFS.h>

Mqtt mqtt(CONTROLLER_IDENTIFIER);

void subscribeMqtt()
{
  mqtt.subscribe(MqttTopic::CONTROLLER, [](const char *msg)
                 { Serial.printf("Received message: %s\n", msg); });
}

void setup()
{
  Serial.begin(115200);

  if (!SPIFFS.begin(true))
  {
    while (true)
    {
      Serial.println("These components are missing:");
      Serial.println("- SPIFFS");
      Serial.println();
      delay(1000);
    }
  }

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
  else
    Serial.println("Unknown command, available commands are: wifi, mqtt");
}