#include "core/record.h"
#include "core/mqtt.h"
#include "core/led.h"
#include "core/buffer.h"

#include "mqtt/protocol.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <vector>

#define WAV_FILE_PATH "/recording.wav"
#define RECORDER_BUFFER_SIZE 512
#define RECORDER_DURATION 5000

ESP_STATIC_ASSERT(
    RECORDER_BUFFER_SIZE % AudioConfig::bytesPerSample == 0,
    "Buffer size must be a multiple of bytes per sample");

struct MqttSenderTaskContext
{
  QueueHandle_t *queue;
  Mqtt *mqtt;
  const char *topic;
  MqttTransmissionResult *result;

  bool *finished;
  size_t bufferSize;
};

RecorderResult verifyToMqtt(Recorder &recorder, Mqtt &mqtt, uint8_t blinkingPin)
{
  RecorderResult result{RecorderCode::OK};

  if (!mqtt.client)
  {
    Serial.println("MQTT client is not initialized, please setup mqtt");
    result.code = RecorderCode::MQTT_NOT_READY;
    return result;
  }

  bool finished = false;
  MqttTransmissionResult mqttResult{0, 0};

  size_t actualBufferSize = RECORDER_BUFFER_SIZE * AudioConfig::validBytesPerSample / AudioConfig::bytesPerSample;
  size_t actualSize = calculateActualSizeFor(RECORDER_DURATION, RECORDER_SAMPLE_RATE);
  QueueHandle_t audioQueue = xQueueCreate(actualSize / actualBufferSize / 4, actualBufferSize);
  if (!audioQueue)
  {
    result.code = RecorderCode::AUDIO_QUEUE_ALLOC_FAILED;
    return result;
  }

  auto senderTask = [](void *param)
  {
    auto *ctx = static_cast<MqttSenderTaskContext *>(param);
    auto &queue = *ctx->queue;
    auto &finished = *ctx->finished;
    auto &mqtt = *ctx->mqtt;
    auto &result = *ctx->result;

    size_t packetNumber = 1;
    uint8_t buffer[ctx->bufferSize];
    while (!finished || uxQueueMessagesWaiting(queue) > 0)
    {
      if (xQueueReceive(queue, buffer, pdMS_TO_TICKS(100)) == pdTRUE)
      {
        Serial.println("Sending audio fragment via MQTT");
        auto res = mqtt.publishFragmentBody(ctx->topic, buffer, ctx->bufferSize);
        if (res != 0)
        {
          result.code = res;
          vTaskDelete(nullptr);
          return;
        }
      }
    }
    vTaskDelete(nullptr);
  };

  MqttSenderTaskContext ctx{&audioQueue, &mqtt, MqttTopic::RECORDER, &mqttResult, &finished, actualBufferSize};
  xTaskCreate(senderTask, "MqttSender", 4096, &ctx, 1, nullptr);

  auto blink = createBlinker(blinkingPin);
  recorder.readFor(
      RECORDER_DURATION,
      RECORDER_BUFFER_SIZE,
      [blink, actualBufferSize, &audioQueue](const int32_t *data)
      {
        blink(0);
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

        auto res = xQueueSend(audioQueue, buf, 0);
      });

  blink(1);

  finished = true;
  while (uxQueueMessagesWaiting(audioQueue) > 0)
  {
    if (mqttResult.code != 0)
    {
      result.code = RecorderCode::MQTT_TRANSMISSION_FAILED;
      result.mqttError.emplace(mqttResult);
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  vQueueDelete(audioQueue);
  return result;
}