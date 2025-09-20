#include "mqtt/protocol.h"

#include <cstring>

int main() { return 0; }

extern "C" {
const char *ffi_mqttProtocol(const char *protocolKey, const char *key) {
  if (!key)
    return nullptr;

#define X(name, value)                                                         \
  if (strcmp(key, #name) == 0)                                                 \
    return PX::name;

  if (strcmp(protocolKey, "MqttHeader") == 0) {
#define PX MqttHeader
    MQTT_HEADER
#undef PX
  } else if (strcmp(protocolKey, "MqttMessageType") == 0) {
#define PX MqttMessageType
    MQTT_MESSAGE_TYPE
#undef PX
  } else if (strcmp(protocolKey, "MqttTopic") == 0) {
#define PX MqttTopic
    MQTT_TOPIC
#undef PX
  }
  return nullptr;
}
}