#pragma once

extern "C"
{
  const char *ffi_mqttProtocol(const char *protocolKey, const char *key);
  const char *const *ffi_mqttProtocolList(const char *protocolKey);
}

/* -------------------------------------------------------------------------- */
/*                             Protocol Definition                            */
/* -------------------------------------------------------------------------- */

#define MQTT_PROTOCOL          \
  _MQEXPAND(MQTT_HEADER)       \
  _MQEXPAND(MQTT_MESSAGE_TYPE) \
  _MQEXPAND(MQTT_TOPIC)        \
  _MQEXPAND(MQTT_CONTROLLER_COMMAND)

/* ------------------------------ Protocol Key ------------------------------ */

#define MQTT_HEADER_KEY MqttHeader
#define MQTT_MESSAGE_TYPE_KEY MqttMessageType
#define MQTT_TOPIC_KEY MqttTopic
#define MQTT_CONTROLLER_COMMAND_KEY MqttControllerCommand

/* ------------------------------ Protocol List ----------------------------- */

#define MQTT_HEADER_LIST \
  _MQX(WILL, "ded")      \
  _MQX(VERIFY, "verify")

#define MQTT_MESSAGE_TYPE_LIST  \
  _MQX(MESSAGE, "msg ")         \
  _MQX(FRAGMENT_HEADER, "head") \
  _MQX(FRAGMENT_BODY, "frag")   \
  _MQX(FRAGMENT_TRAILER, "end ")

#define MQTT_TOPIC_LIST                                       \
  _MQX(RECORDER, "audio_biometric/slainless/device/recorder") \
  _MQX(CONTROLLER, "audio_biometric/slainless/device/controller")

#define MQTT_CONTROLLER_COMMAND_LIST \
  _MQX(ON, "on")                     \
  _MQX(OFF, "off")

/* -------------------------------------------------------------------------- */
/*                              End of Definition                             */
/* -------------------------------------------------------------------------- */

#define _MQEXPAND(header) _MQP(header##_KEY, header##_LIST)
#define LITERAL(name, ...) JOIN(name, __VA_ARGS__)
#define JOIN(name, suffix) name##suffix

/* --------------------------- Protocol namespace --------------------------- */

#define _MQP(header, list)  \
  namespace LITERAL(header) \
  {                         \
    list                    \
  }
#define _MQX(name, value) constexpr char name[] = value;

MQTT_PROTOCOL

#undef _MQX
#undef _MQP

/* --------------------------- Protocol value list -------------------------- */

#define _MQP(header, list) \
  static const char *const LITERAL(header, List)[] = {list nullptr};
#define _MQX(name, value) value,

MQTT_PROTOCOL

#undef _MQX
#undef _MQP

/* --------------------------------- Cleanup -------------------------------- */

#undef LITERAL
#undef JOIN