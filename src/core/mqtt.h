#include <Arduino.h>
#include <MqttClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <functional>

class Mqtt {
public:
  std::unique_ptr<MqttClient> client;
  Mqtt();

  bool connect(const char *host, uint16_t port, bool secure);
  void poll(std::function<void()> connectCallback = nullptr);

private:
  WiFiClient insecureClient;
  WiFiClientSecure secureClient;
};

void configureMqtt(Mqtt &mqtt);
void reconnectMqtt(Mqtt &mqtt);