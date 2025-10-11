#pragma once

#include "core/audio.h"
#include "core/mqtt.h"
#include <optional>

#ifndef RECORDER_SAMPLE_RATE
#define RECORDER_SAMPLE_RATE 4000
#endif

#ifndef RECORDER_BUFFER_SIZE
#define RECORDER_BUFFER_SIZE 512
#endif

#ifndef RECORDER_AMP_THRESHOLD
#define RECORDER_AMP_THRESHOLD 0.005
#endif

#ifndef RECORDER_TIME_OFFSET
#define RECORDER_TIME_OFFSET 500 // ms
#endif

#ifndef RECORDER_MAX_RECORD_TIME
#define RECORDER_MAX_RECORD_TIME 4000 // ms
#endif

#define RECORDER_ACTUAL_BUFFER_SIZE RECORDER_BUFFER_SIZE *AudioConfig::validBytesPerSample / AudioConfig::bytesPerSample

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
#if USE_REALTIME_RECORDING == 0
  RecorderResult verify(Recorder &recorder, Mqtt &mqtt, uint8_t blinkingPin);
#endif
  RecorderResult sample(Recorder &recorder, Mqtt &mqtt, uint8_t blinkingPin, const char *sampleName);
  void normalizeSamples(const int32_t *data, const size_t dataSize, uint8_t *dest, NormalizationCallback cb = nullptr);
  RecorderResult poll(
      Recorder &recorder, Mqtt &mqtt,
      int32_t *buffer, size_t &bytesRead,
      u_long &lastPeakHit, u_long &lastRecordingStart,
      bool &isRecording,
      uint8_t indicatorPin);
}