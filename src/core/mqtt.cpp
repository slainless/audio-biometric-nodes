#include "core/mqtt.h"
#include "core/serial.h"

#include <Arduino.h>
#include <MqttClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <functional>

char brokerHost[64];
uint16_t brokerPort;
bool brokerUseSsl;

Mqtt::Mqtt() { secureClient.setInsecure(); }

bool Mqtt::connect(const char *host, uint16_t port, bool secure) {
  if (secure) {
    client.reset(new MqttClient(secureClient));
    return client->connect(host, port);
  } else {
    client.reset(new MqttClient(insecureClient));
    return client->connect(host, port);
  }
}

void Mqtt::poll(std::function<void()> connectCallback) {
  if (!client->connected()) {
    connectCallback();
  }

  if (client) {
    client->poll();
  }
}

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