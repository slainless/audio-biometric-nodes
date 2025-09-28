#pragma once

#include "core/audio.h"
#include "core/mqtt.h"
#include <optional>

#define RECORDER_SAMPLE_RATE 8000

struct MqttTransmissionResult
{
  size_t packetNumber;
  int code;
};

enum class RecorderCode
{
  OK,
  MQTT_NOT_READY,
  AUDIO_QUEUE_ALLOC_FAILED,
  MQTT_TRANSMISSION_FAILED
};

struct RecorderResult
{
  RecorderCode code;
  std::optional<MqttTransmissionResult> mqttError;
};

namespace Record
{
  RecorderResult verify(Recorder &recorder, Mqtt &mqtt, uint8_t blinkingPin);
  RecorderResult sample(Recorder &recorder, Mqtt &mqtt, uint8_t blinkingPin);
}