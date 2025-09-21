#include "mqtt/protocol.h"

#include <cstring>

int main() { return 0; }

extern "C" {
const char *ffi_mqttProtocol(const char *protocolKey, const char *key) {
  if (!key)
    return nullptr;

#define _MQP(header, list)                                                     \
  if (strcmp(protocolKey, #header) == 0) {                                     \
    namespace protocol = header;                                               \
    list return nullptr;                                                       \
  }
#define _MQX(name, value)                                                      \
  if (strcmp(key, #name) == 0)                                                 \
    return protocol::name;

  MQTT_PROTOCOL

#undef _MQP
#undef _MQX

  return nullptr;
}

const char *const *ffi_mqttProtocolList(const char *protocolKey) {
  if (!protocolKey)
    return nullptr;

#define JOIN(a, b) a##b
#define LITERAL(name) #name
#define _MQP(header, list)                                                     \
  if (strcmp(protocolKey, LITERAL(header)) == 0) {                             \
    return JOIN(header, List);                                                 \
  }

  MQTT_PROTOCOL

#undef _MQP
#undef JOIN
#undef LITERAL

  return nullptr;
}
}