#include "core/mqtt.h"
#include "core/serial.h"
#include "mqtt/protocol.h"

#include <Arduino.h>
#include <MqttClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <cstring>
#include <functional>

#define isClientReady                                                          \
  if (!client) {                                                               \
    Serial.println("MQTT client is not initialized, please setup mqtt");       \
    return false;                                                              \
  }

char brokerHost[64];
uint16_t brokerPort;
bool brokerUseSsl;

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

  auto res = client->beginWill(topic, false, 0);
  handleError(res, "starting will writing");

  res = stamp(MqttMessageType::MESSAGE);
  handleError(res, "stamping will message");

  res = client->print(MqttHeader::WILL);
  handleError(res, "writing will message");

  res = client->endWill();
  handleError(res, "sending will");

  return 0;
};

int Mqtt::publishMessage(const char *topic, const char *message) {
  isClientReady;

  auto res = client->beginMessage(topic);
  handleError(res, "starting message writing");

  res = stamp(MqttMessageType::MESSAGE);
  handleError(res, "stamping will message");

  res = client->print(message);
  handleError(res, "writing message");

  res = client->endMessage();
  handleError(res, "sending message");

  return 0;
};

int Mqtt::publishFragmentHeader(const char *topic, const char *header) {
  isClientReady;

  auto res = client->beginMessage(topic);
  handleError(res, "starting fragment header writing");

  res = stamp(MqttMessageType::FRAGMENT_HEADER);
  handleError(res, "stamping fragment header");

  res = client->print(header);
  handleError(res, "writing fragment header");

  res = client->endMessage();
  handleError(res, "sending fragment header");

  return 0;
};

int Mqtt::publishFragmentBody(const char *topic, const uint8_t *body,
                              size_t size) {
  isClientReady;

  auto res = client->beginMessage(topic);
  handleError(res, "starting fragment body writing");

  res = stamp(MqttMessageType::FRAGMENT_BODY);
  handleError(res, "stamping fragment body");

  res = client->write(body, size);
  handleError(res, "writing fragment body");

  res = client->endMessage();
  handleError(res, "sending fragment body");

  return 0;
};

int Mqtt::publishFragmentTrailer(const char *topic) {
  isClientReady;

  auto res = client->beginMessage(topic);
  handleError(res, "starting fragment trailer writing");

  res = stamp(MqttMessageType::FRAGMENT_TRAILER);
  handleError(res, "stamping fragment trailer");

  res = client->endMessage();
  handleError(res, "sending fragment trailer");

  return 0;
};

int Mqtt::stamp(const char *protocol) {
  isClientReady;

  auto res = client->print(protocol);
  handleError(res, "stamping protocol");

  res = client->write(stampSize);
  handleError(res, "stamping identifier size");

  res = client->write(reinterpret_cast<const uint8_t *>(identifier), stampSize);
  handleError(res, "stamping identifier");

  return 0;
};

void configureMqtt(Mqtt &mqtt) {
  Serial.print("Broker host: ");
  auto host = blockingReadStringUntil('\n');
  Serial.print(host);

  Serial.print("Broker port: ");
  auto port = blockingReadStringUntil('\n');
  Serial.print(port);

  Serial.print("Use SSL?: (y/n) ");
  auto useSsl = blockingReadStringUntil('\n');
  Serial.print(useSsl);

  host.trim();
  port.trim();
  useSsl.trim();

  host.toCharArray(brokerHost, sizeof(brokerHost));
  brokerPort = port.toInt();
  brokerUseSsl = useSsl.equalsIgnoreCase("y");
};

void reconnectMqtt(Mqtt &mqtt) {
  if (brokerHost[0] == '\0') {
    Serial.println("MQTT broker host is not set, skipping reconnect");
    return;
  }

  Serial.printf("Connecting to %s:%d with SSL: %b\n", brokerHost, brokerPort,
                brokerUseSsl);
  if (!mqtt.connect(brokerHost, brokerPort, brokerUseSsl)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqtt.client->connectError());
  }
};