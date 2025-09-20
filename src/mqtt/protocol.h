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

namespace MqttHeader {
#define X(name, value) constexpr char name[] = value;
MQTT_HEADER
#undef X
} // namespace MqttHeader

namespace MqttMessageType {
#define X(name, value) constexpr char name[] = value;
MQTT_MESSAGE_TYPE
#undef X
} // namespace MqttMessageType

namespace MqttTopic {
#define X(name, value) constexpr char name[] = value;
MQTT_TOPIC
#undef X
} // namespace MqttTopic

extern "C" {
const char *ffi_mqttProtocol(const char *protocolKey, const char *key);
}