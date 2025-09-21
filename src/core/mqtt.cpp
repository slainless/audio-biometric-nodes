#include "core/mqtt.h"
#include "core/flash.h"
#include "core/serial.h"
#include "mqtt/protocol.h"

#include <Arduino.h>
#include <MqttClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <cstring>
#include <functional>

#define MQTT_CONFIG_PATH "/mqtt.bin"

#define isClientReady                                                          \
  if (!client) {                                                               \
    Serial.println("MQTT client is not initialized, please setup mqtt");       \
    return false;                                                              \
  }

struct MqttConfig {
  char host[64];
  uint16_t port;
  bool useSsl;
};

MqttConfig mqttConfig;

Mqtt::Mqtt(const char *identifier)
    : identifier(identifier), stampSize(strlen(identifier)) {
  secureClient.setInsecure();
}

bool Mqtt::connect(const char *host, uint16_t port, bool secure) {
  if (secure) {
    client.reset(new MqttClient(secureClient));
  } else {
    client.reset(new MqttClient(insecureClient));
  }

  if (!client->connect(host, port)) {
    return false;
  }

  publishWill(MqttTopic::RECORDER, MqttHeader::WILL);
  return true;
}

void Mqtt::poll(std::function<void()> connectCallback) {
  if (client) {
    if (!client->connected()) {
      if (connectCallback) {
        Serial.println("MQTT client not connected, calling connect callback");
        connectCallback();
      }
    }

    client->poll();
  }
}

int Mqtt::publishWill(const char *topic, const char *message) {
  isClientReady;

  client->beginWill(topic, false, 0);
  stamp(MqttMessageType::MESSAGE);
  client->print(MqttHeader::WILL);
  client->endWill();

  return 0;
};

int Mqtt::publishMessage(const char *topic, const char *message) {
  isClientReady;

  client->beginMessage(topic);
  stamp(MqttMessageType::MESSAGE);
  client->print(message);
  client->endMessage();

  return 0;
};

int Mqtt::publishFragmentHeader(const char *topic, const char *header) {
  isClientReady;

  client->beginMessage(topic);
  stamp(MqttMessageType::FRAGMENT_HEADER);
  client->print(header);
  client->endMessage();

  return 0;
};

int Mqtt::publishFragmentBody(const char *topic, const uint8_t *body,
                              size_t size) {
  isClientReady;

  client->beginMessage(topic);
  stamp(MqttMessageType::FRAGMENT_BODY);
  client->write(body, size);
  client->endMessage();

  return 0;
};

int Mqtt::publishFragmentTrailer(const char *topic) {
  isClientReady;

  client->beginMessage(topic);
  stamp(MqttMessageType::FRAGMENT_TRAILER);
  client->endMessage();

  return 0;
};

int Mqtt::stamp(const char *protocol) {
  isClientReady;

  client->print(protocol);
  client->write(stampSize);
  client->write(reinterpret_cast<const uint8_t *>(identifier), stampSize);

  return 0;
};

void reconnectMqtt(Mqtt &mqtt) {
  if (mqttConfig.host[0] == '\0') {
    Serial.println("MQTT broker host is not set, skipping reconnect");
    return;
  }

  Serial.printf("Connecting to %s:%d with SSL: %b\n", mqttConfig.host,
                mqttConfig.port, mqttConfig.useSsl);
  if (!mqtt.connect(mqttConfig.host, mqttConfig.port, mqttConfig.useSsl)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqtt.client->connectError());
  }
};

void setupMqtt(Mqtt &mqtt) {
  Serial.println("Loading Mqtt configuration from flash");
  if (load(MQTT_CONFIG_PATH, reinterpret_cast<unsigned char *>(&mqttConfig),
           sizeof(MqttConfig))) {
    Serial.println("Mqtt configuration loaded");
    reconnectMqtt(mqtt);
  }
}

void configureMqtt(Mqtt &mqtt) {
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

  host.toCharArray(mqttConfig.host, sizeof(mqttConfig.host));
  mqttConfig.port = port.toInt();
  mqttConfig.useSsl = useSsl.equalsIgnoreCase("y");

  Serial.println("Saving Mqtt configuration to flash");
  if (store(MQTT_CONFIG_PATH, reinterpret_cast<unsigned char *>(&mqttConfig),
            sizeof(MqttConfig))) {
    Serial.println("Mqtt configuration saved");
  } else {
    Serial.println("Failed to save Mqtt configuration to flash");
  }

  reconnectMqtt(mqtt);
};