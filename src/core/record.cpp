#include "core/record.h"
#include "core/mqtt.h"
#include "core/led.h"
#include "core/buffer.h"
#include "core/utils.h"
#include "core/remotexy.h"

#include "mqtt/protocol.h"

#include <Arduino.h>
#include <vector>
#include <cstring>

#define WAV_FILE_PATH "/recording.wav"
#define RECORDER_BUFFER_SIZE 512
#define RECORDER_DURATION 5000

ESP_STATIC_ASSERT(
    RECORDER_BUFFER_SIZE % AudioConfig::bytesPerSample == 0,
    "Buffer size must be a multiple of bytes per sample");

createTag(RECORD);

struct MqttSenderTaskContext
{
  QueueHandle_t *queue;
  Mqtt *mqtt;
  const char *topic;
  MqttTransmissionResult *result;

  bool *finished;
  size_t bufferSize;
};

namespace Record
{
#define __assertMqttReady                                               \
  if (!mqtt.client)                                                     \
  {                                                                     \
    ESP_LOGI(TAG, "MQTT client is not initialized, please setup mqtt"); \
    RecorderResult result{RecorderCode::MQTT_NOT_READY};                \
    return result;                                                      \
  }

  RecorderResult start(Recorder &recorder, Mqtt &mqtt, uint8_t blinkingPin)
  {
    RecorderResult result{RecorderCode::OK};
    __assertMqttReady;

    RemoteXY.led_recorder = HIGH;
    sprintf(RemoteXY.value_recorder_status, "Recording...");
    RemoteXY_Handler();

    bool finished = false;
    MqttTransmissionResult mqttResult{0, 0};

    size_t actualBufferSize = RECORDER_BUFFER_SIZE * AudioConfig::validBytesPerSample / AudioConfig::bytesPerSample;
    size_t actualSize = calculateActualSizeFor(RECORDER_DURATION, RECORDER_SAMPLE_RATE);
    size_t totalPackets = actualSize / actualBufferSize;
    ESP_LOGI(TAG, "Free heap: %d, actual size: %d, buffer size: %d", xPortGetFreeHeapSize(), actualSize, actualBufferSize);

    uint8_t header[44];
    recorder.writeWavHeader(header, actualSize);
    auto res = mqtt.publishFragmentBody(MqttTopic::RECORDER, header, 44);
    if (res != ESP_OK)
    {
      mqttResult.code = res;
      mqttResult.packetNumber = 0;
      result.code = RecorderCode::MQTT_TRANSMISSION_FAILED;
      result.mqttError.emplace(mqttResult);
    }

    size_t packetNumber = 0;
    auto blink = createBlinker(blinkingPin);
    recorder.readFor(
        RECORDER_DURATION,
        RECORDER_BUFFER_SIZE,
        [blink, actualBufferSize, &mqtt, &mqttResult, &packetNumber, totalPackets](const int32_t *data)
        {
          blink(0);
          RemoteXY_Handler();

          size_t bytesWritten = 0;
          uint8_t buf[actualBufferSize];
          for (size_t i = 0; i < RECORDER_BUFFER_SIZE / AudioConfig::bytesPerSample; i++)
          {
            auto sample = data[i];
            if (AudioConfig::isLeftJustified)
            {
              sample = sample >> 8;
            }

            auto v = reinterpret_cast<uint8_t *>(&sample);
            buf[bytesWritten++] = v[0];
            buf[bytesWritten++] = v[1];
            buf[bytesWritten++] = v[2];
          }

          auto res = mqtt.publishFragmentBody(MqttTopic::RECORDER, buf, actualBufferSize);
          if (res != 0)
          {
            ESP_LOGE(TAG, "Fail to send packet with res: %d", res);
            mqttResult.code = res;
            mqttResult.packetNumber = packetNumber;
          }
          else
          {
            ESP_LOGI(TAG, "Fragment packet (%d/%d) sent", packetNumber + 1, totalPackets);
          }

          packetNumber++;
        });

    blink(1);

    RemoteXY.led_recorder = LOW;
    sprintf(RemoteXY.value_recorder_status, "Recording complete");
    RemoteXY_Handler();

    if (mqttResult.code != ESP_OK)
    {
      result.code = RecorderCode::MQTT_TRANSMISSION_FAILED;
      result.mqttError.emplace(mqttResult);
    }
    return result;
  };

#define __returnMqttError(mqttCode, status)                                 \
  if (res != ESP_OK)                                                        \
  {                                                                         \
    RemoteXY.led_recorder = LOW;                                            \
    ESP_LOGE(TAG, "Fail to transmit MQTT packet with error: %d", mqttCode); \
    sprintf(status, "MQTT Transmission Error: %d", res);                    \
    RecorderResult result{RecorderCode::MQTT_TRANSMISSION_FAILED};          \
    result.mqttError.emplace(MqttTransmissionResult{0, res});               \
    return result;                                                          \
  }

  RecorderResult verify(Recorder &recorder, Mqtt &mqtt, uint8_t blinkingPin)
  {
    __assertMqttReady;

    auto res = mqtt.publishFragmentHeader(MqttTopic::RECORDER, MqttHeader::VERIFY);
    __returnMqttError(res, RemoteXY.value_recorder_status);

    auto recordingResult = start(recorder, mqtt, blinkingPin);
    if (recordingResult.code != RecorderCode::OK)
    {
      if (recordingResult.code == RecorderCode::MQTT_TRANSMISSION_FAILED && recordingResult.mqttError.has_value())
      {
        ESP_LOGE(
            TAG,
            "Fail when transmitting with error code: %d, at packet number: %d",
            recordingResult.mqttError->code,
            recordingResult.mqttError->packetNumber);
        sprintf(
            RemoteXY.value_recorder_status,
            "MQTT Transmission error: %d - %d",
            recordingResult.mqttError->code,
            recordingResult.mqttError->packetNumber);
        return recordingResult;
      }
      else
      {
        ESP_LOGE(TAG, "Fail when recording with error code: %d", recordingResult.code);
        sprintf(RemoteXY.value_recorder_status, "Recording error: %d", recordingResult.code);
      }
    }

    res = mqtt.publishFragmentTrailer(MqttTopic::RECORDER);
    __returnMqttError(res, RemoteXY.value_recorder_status);

    return RecorderResult{RecorderCode::OK};
  };

  RecorderResult sample(Recorder &recorder, Mqtt &mqtt, uint8_t blinkingPin, const char *sampleName)
  {
    __assertMqttReady;

    auto res = mqtt.publishFragmentHeader(MqttTopic::RECORDER, MqttHeader::SAMPLE);
    __returnMqttError(res, RemoteXY.value_sampler_status);

    res = mqtt.publishFragmentBody(MqttTopic::RECORDER, reinterpret_cast<const uint8_t *>(sampleName), std::strlen(sampleName) + 1);
    __returnMqttError(res, RemoteXY.value_sampler_status);

    auto recordingResult = start(recorder, mqtt, blinkingPin);
    if (recordingResult.code != RecorderCode::OK)
    {
      if (recordingResult.code == RecorderCode::MQTT_TRANSMISSION_FAILED && recordingResult.mqttError.has_value())
      {
        ESP_LOGE(
            TAG,
            "Fail when transmitting with error code: %d, at packet number: %d",
            recordingResult.mqttError->code,
            recordingResult.mqttError->packetNumber);
        sprintf(
            RemoteXY.value_sampler_status,
            "MQTT Transmission error: %d - %d",
            recordingResult.mqttError->code,
            recordingResult.mqttError->packetNumber);
        return recordingResult;
      }
      else
      {
        ESP_LOGE(TAG, "Fail when recording with error code: %d", recordingResult.code);
        sprintf(RemoteXY.value_sampler_status, "Recording error: %d", recordingResult.code);
      }
    }

    res = mqtt.publishFragmentTrailer(MqttTopic::RECORDER);
    __returnMqttError(res, RemoteXY.value_sampler_status);

    return RecorderResult{RecorderCode::OK};
  };

#undef __returnMqttError
#undef __assertMqttReady
}