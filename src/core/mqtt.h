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

struct MqttConfig
{
  char host[64];
  uint16_t port;
  bool useSsl;
};

class Mqtt
{
public:
  std::unique_ptr<MqttClient> client;
  Mqtt(const char *identifier);

  bool connect(MqttConfig &config);
  bool connect(const char *host, uint16_t port, bool secure);
  void poll(std::function<void()> connectCallback = nullptr);

  int publishWill(const char *topic, const char *message);
  int publishMessage(const char *topic, const char *message);
  int publishFragmentHeader(const char *topic, const char *header);
  int publishFragmentBody(const char *topic, const uint8_t *body, size_t size);
  int publishFragmentTrailer(const char *topic);

  int subscribe(const char *topic,
                std::function<void(const char *message)> cb);

private:
  const char *identifier;
  uint8_t stampSize;

  WiFiClient insecureClient;
  WiFiClientSecure secureClient;

  int stamp(const char *protocol);
};

namespace MqttConfigurer
{
  int setup(MqttConfig &mqttConfig, Mqtt &mqtt);
  int serialPrompt(MqttConfig &mqttConfig, Mqtt &mqtt);
  int reconnect(MqttConfig &mqttConfig, Mqtt &mqtt);
}