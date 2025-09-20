#pragma once

#define MQTT_HEADER                                                            \
  X(WILL, "ded")                                                               \
  X(VERIFY, "verify")

#define MQTT_MESSAGE_TYPE                                                      \
  X(MESSAGE, "msg")                                                            \
  X(FRAGMENT_HEADER, "head")                                                   \
  X(FRAGMENT_BODY, "frag")                                                     \
  X(FRAGMENT_TRAILER, "end")

#define MQTT_TOPIC                                                             \
  X(RECORDER, "audio_biometric/slainless/device/recorder")                     \
  X(CONTROLLER, "audio_biometric/slainless/device/controller")

#define X(name, value) constexpr char name[] = value;
namespace MqttHeader {
MQTT_HEADER
} // namespace MqttHeader

namespace MqttMessageType {
MQTT_MESSAGE_TYPE
} // namespace MqttMessageType

namespace MqttTopic {
MQTT_TOPIC
} // namespace MqttTopic
#undef X

extern "C" {
const char *ffi_mqttProtocol(const char *protocolKey, const char *key);
}