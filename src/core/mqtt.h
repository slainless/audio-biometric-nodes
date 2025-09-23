#pragma once

#include <Arduino.h>
#include <MqttClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <cstddef>
#include <cstdint>
#include <functional>

#define handleError(code, message)                                 \
  if (code != 0)                                                   \
  {                                                                \
    Serial.printf("MQTT client error on %s: %d\n", message, code); \
    return code;                                                   \
  }

class Mqtt
{
public:
  std::unique_ptr<MqttClient> client;
  Mqtt(const char *identifier);

  bool connect(const char *host, uint16_t port, bool secure);
  void poll(std::function<void()> connectCallback = nullptr);

  int publishWill(const char *topic, const char *message);
  int publishMessage(const char *topic, const char *message);
  int publishFragmentHeader(const char *topic, const char *header);
  int publishFragmentBody(const char *topic, const uint8_t *body, size_t size);
  int publishFragmentTrailer(const char *topic);

  bool subscribe(const char *topic,
                 std::function<void(const char *message)> cb);

private:
  const char *identifier;
  uint8_t stampSize;

  WiFiClient insecureClient;
  WiFiClientSecure secureClient;

  int stamp(const char *protocol);
};

void setupMqtt(Mqtt &mqtt);
void configureMqtt(Mqtt &mqtt);
void reconnectMqtt(Mqtt &mqtt);