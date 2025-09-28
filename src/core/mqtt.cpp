#include "core/mqtt.h"
#include "core/filesystem.h"
#include "core/serial.h"
#include "mqtt/protocol.h"

#include <Arduino.h>
#include <MqttClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_log.h>

#include <cstring>
#include <functional>

#define MQTT_CONFIG_PATH "/spiffs/mqtt.bin"

static const char *TAG = "MQTT";

#define isClientReady                                                    \
  if (!client)                                                           \
  {                                                                      \
    Serial.println("MQTT client is not initialized, please setup mqtt"); \
    return false;                                                        \
  }

Mqtt::Mqtt(const char *identifier)
    : identifier(identifier), stampSize(strlen(identifier))
{
  secureClient.setInsecure();
}

bool Mqtt::connect(const char *host, uint16_t port, bool secure)
{
  if (secure)
  {
    client.reset(new MqttClient(secureClient));
  }
  else
  {
    client.reset(new MqttClient(insecureClient));
  }

  if (!client->connect(host, port))
  {
    return false;
  }

  publishWill(MqttTopic::RECORDER, MqttHeader::WILL);
  return true;
}

bool Mqtt::connect(MqttConfig &config)
{
  return connect(config.host, config.port, config.useSsl);
}

void Mqtt::poll(std::function<void()> connectCallback)
{
  if (client)
  {
    if (!client->connected())
    {
      if (connectCallback)
      {
        ESP_LOGI(TAG, "MQTT client not connected, calling connect callback");
        connectCallback();
      }
    }

    client->poll();
  }
}

int Mqtt::publishWill(const char *topic, const char *message)
{
  isClientReady;

  client->beginWill(topic, false, 0);
  stamp(MqttMessageType::MESSAGE);
  client->print(MqttHeader::WILL);
  client->endWill();

  return 0;
};

int Mqtt::publishMessage(const char *topic, const char *message)
{
  isClientReady;

  client->beginMessage(topic);
  stamp(MqttMessageType::MESSAGE);
  client->print(message);
  client->endMessage();

  return 0;
};

int Mqtt::publishFragmentHeader(const char *topic, const char *header)
{
  isClientReady;

  client->beginMessage(topic);
  stamp(MqttMessageType::FRAGMENT_HEADER);
  client->print(header);
  client->endMessage();

  return 0;
};

int Mqtt::publishFragmentBody(const char *topic, const uint8_t *body,
                              size_t size)
{
  isClientReady;

  client->beginMessage(topic);
  stamp(MqttMessageType::FRAGMENT_BODY);
  client->write(body, size);
  client->endMessage();

  return 0;
};

int Mqtt::publishFragmentTrailer(const char *topic)
{
  isClientReady;

  client->beginMessage(topic);
  stamp(MqttMessageType::FRAGMENT_TRAILER);
  client->endMessage();

  return 0;
};

int Mqtt::stamp(const char *protocol)
{
  isClientReady;

  client->print(protocol);
  client->write(stampSize);
  client->write(reinterpret_cast<const uint8_t *>(identifier), stampSize);

  return 0;
};

int Mqtt::subscribe(const char *topic,
                    std::function<void(const char *message)> cb)
{
  isClientReady;

  auto res = client->subscribe(topic, 0);
  if (res != 1)
  {
    ESP_LOGE(TAG, "Failed to subscribe to MQTT server (topic: %s) with code: %d", topic, res);
    return res;
  }

  client->onMessage(
      [cb](MqttClient *mqttClient, int messageSize)
      {
#define __assert_read(into, size)                                                 \
  read = mqttClient->read(into, size);                                            \
  if (read != size)                                                               \
  {                                                                               \
    Serial.printf("EOF: Expecting message to be of length %d, instead got: %d\n", \
                  size, read);                                                    \
    return;                                                                       \
  }
        int read;

        if (messageSize < 6)
        {
          Serial.println("Received MQTT message that is too short");
          return;
        }

        if (messageSize == 6)
        {
          Serial.println("Receiving empty MQTT message");
          cb("");
          return;
        }

        char messageType[5] = {0};
        __assert_read(reinterpret_cast<uint8_t *>(messageType), 4);

        if (strncmp(messageType, MqttMessageType::MESSAGE, 4) != 0)
        {
          Serial.printf("Receiving non-message type MQTT message: %s\n", messageType);
          return;
        }

        uint8_t idSize;
        __assert_read(&idSize, 1);

        char id[idSize + 1] = {0};
        __assert_read(reinterpret_cast<uint8_t *>(id), idSize);

        if (strncmp(id, MqttIdentifier::SERVER, idSize) != 0)
        {
          Serial.printf("Received MQTT message from source other than server: %s\n", id);
          return;
        }

        size_t payloadSize = messageSize - 5 - idSize;
        char payload[payloadSize + 1] = {0};
        __assert_read(reinterpret_cast<uint8_t *>(payload), payloadSize);

        Serial.printf("Receiving MQTT message of size %d from %s with content:\n%s\n", payloadSize, id, payload);
        cb(payload);

#undef __assert_read
      });

  return true;
}

namespace MqttConfigurer
{
  int store(MqttConfig &config)
  {
    ESP_LOGI(TAG, "Saving Mqtt configuration to flash");
    if (FileSystem::store(MQTT_CONFIG_PATH, reinterpret_cast<unsigned char *>(&config),
                          sizeof(config)))
    {
      ESP_LOGI(TAG, "Mqtt configuration saved");
      return 0;
    }
    else
    {
      ESP_LOGE(TAG, "Failed to save Mqtt configuration to flash");
      return 1;
    }
  }

  int reconnect(MqttConfig &config, Mqtt &mqtt)
  {
    if (config.host[0] == '\0')
    {
      ESP_LOGI(TAG, "MQTT broker host is not set, skipping reconnect");
      return 1;
    }

    ESP_LOGI(TAG, "Connecting to %s:%d with SSL: %d", config.host,
             config.port, (int)config.useSsl);
    if (!mqtt.connect(config.host, config.port, config.useSsl))
    {
      auto code = mqtt.client->connectError();
      ESP_LOGE(TAG, "MQTT connection failed with code: %d", code);
      return code;
    }

    return ESP_OK;
  };

  int setup(MqttConfig &config, Mqtt &mqtt)
  {
    ESP_LOGI(TAG, "Loading Mqtt configuration from flash");
    if (FileSystem::load(MQTT_CONFIG_PATH, reinterpret_cast<unsigned char *>(&config),
                         sizeof(MqttConfig)))
    {
      ESP_LOGI(TAG, "Mqtt configuration loaded");
      MqttConfigurer::reconnect(config, mqtt);
    }

    return ESP_OK;
  }

  int serialPrompt(MqttConfig &config, Mqtt &mqtt)
  {
    Serial.print("Broker host: ");
    auto host = blockingReadStringUntil('\n');
    Serial.println(host);

    Serial.print("Broker port: ");
    auto port = blockingReadStringUntil('\n');
    Serial.println(port);

    Serial.print("Use SSL?: (y/n) ");
    auto useSsl = blockingReadStringUntil('\n');
    Serial.println(useSsl);

    host.trim();
    port.trim();
    useSsl.trim();

    host.toCharArray(config.host, sizeof(config.host));
    config.port = port.toInt();
    config.useSsl = useSsl.equalsIgnoreCase("y");

    auto res = store(config);
    if (res != ESP_OK)
    {
      return res;
    }

    reconnect(config, mqtt);
    return ESP_OK;
  }
}
