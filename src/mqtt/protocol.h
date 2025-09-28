#pragma once

extern "C"
{
  const char *ffi_mqttProtocol(const char *protocolKey, const char *key);
  const char *const *ffi_mqttProtocolList(const char *protocolKey);
}

/* -------------------------------------------------------------------------- */
/*                             Protocol Definition                            */
/* -------------------------------------------------------------------------- */

#define MQTT_PROTOCOL                \
  _MQEXPAND(MQTT_HEADER)             \
  _MQEXPAND(MQTT_MESSAGE_TYPE)       \
  _MQEXPAND(MQTT_TOPIC)              \
  _MQEXPAND(MQTT_CONTROLLER_COMMAND) \
  _MQEXPAND(MQTT_IDENTIFIER)

/* ------------------------------ Protocol Key ------------------------------ */

#define MQTT_HEADER_KEY MqttHeader
#define MQTT_MESSAGE_TYPE_KEY MqttMessageType
#define MQTT_TOPIC_KEY MqttTopic
#define MQTT_CONTROLLER_COMMAND_KEY MqttControllerCommand
#define MQTT_IDENTIFIER_KEY MqttIdentifier

/* ------------------------------ Protocol List ----------------------------- */

#define MQTT_HEADER_LIST \
  _MQX(WILL, "ded")      \
  _MQX(VERIFY, "verify") \
  _MQX(SAMPLE, "sample")

#define MQTT_MESSAGE_TYPE_LIST  \
  _MQX(MESSAGE, "msg ")         \
  _MQX(FRAGMENT_HEADER, "head") \
  _MQX(FRAGMENT_BODY, "frag")   \
  _MQX(FRAGMENT_TRAILER, "end ")

#define MQTT_TOPIC_LIST                                                   \
  _MQX(RECORDER, "audio_biometric/slainless/device/recorder")             \
  _MQX(VERIFY_RESULT, "audio_biometric/slainless/device/recorder/verify") \
  _MQX(CONTROLLER, "audio_biometric/slainless/device/controller")

#define MQTT_CONTROLLER_COMMAND_LIST \
  _MQX(LAMP_ON, "lamp_on")           \
  _MQX(LAMP_OFF, "lamp_off")         \
  _MQX(FAN_ON, "fan_on")             \
  _MQX(FAN_OFF, "fan_off")

#define MQTT_IDENTIFIER_LIST \
  _MQX(SERVER, "biometric-server")

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