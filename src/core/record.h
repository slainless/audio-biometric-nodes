#pragma once

#include "core/audio.h"
#include "core/mqtt.h"
#include <optional>

#define RECORDER_SAMPLE_RATE 8000
#define RECORDER_BUFFER_SIZE 512

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

using NormalizationCallback = std::function<void(int32_t sample)>;

namespace Record
{
  RecorderResult verify(Recorder &recorder, Mqtt &mqtt, uint8_t blinkingPin);
  RecorderResult sample(Recorder &recorder, Mqtt &mqtt, uint8_t blinkingPin, const char *sampleName);
  void normalizeSamples(const int32_t *data, const size_t dataSize, uint8_t *dest, NormalizationCallback cb = nullptr);
}