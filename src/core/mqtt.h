#include <Arduino.h>
#include <MqttClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <cstddef>
#include <cstdint>
#include <functional>

class Mqtt {
public:
  std::unique_ptr<MqttClient> client;
  Mqtt(const char *identifier);

  bool connect(const char *host, uint16_t port, bool secure);
  void poll(std::function<void()> connectCallback = nullptr);

  bool publishWill(const char *topic, const char *message);
  bool publishMessage(const char *topic, const char *message);
  bool publishFragmentHeader(const char *topic, const char *header);
  bool publishFragmentBody(const char *topic, const uint8_t *body, size_t size);
  bool publishFragmentTrailer(const char *topic);

private:
  const char *identifier;
  uint8_t stampSize;

  WiFiClient insecureClient;
  WiFiClientSecure secureClient;

  bool stamp(const char *protocol);
};

void configureMqtt(Mqtt &mqtt);
void reconnectMqtt(Mqtt &mqtt);