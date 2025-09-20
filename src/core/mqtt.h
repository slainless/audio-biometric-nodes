#include <Arduino.h>
#include <MqttClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

class Mqtt {
public:
  std::unique_ptr<MqttClient> client;
  Mqtt();

  bool connect(const char *host, uint16_t port, bool secure);

private:
  WiFiClient insecureClient;
  WiFiClientSecure secureClient;
};

void configureMqtt(Mqtt &mqtt);
void reconnectMqtt(Mqtt &mqtt);