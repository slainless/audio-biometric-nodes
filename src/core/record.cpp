#include "core/record.h"
#include "core/mqtt.h"
#include "mqtt/protocol.h"

#include <SPIFFS.h>
#include <vector>

#define WAV_FILE_PATH "/recording.wav"
#define RECORDER_BUFFER_SIZE 512
#define RECORDER_DURATION 5000

int publishRecordingToMqtt(File &file, Mqtt &mqtt, uint32_t bufferSize) {
  size_t bytesRead;
  std::vector<uint8_t> buffer(bufferSize);

  auto res =
      mqtt.publishFragmentHeader(MqttTopic::RECORDER, MqttHeader::VERIFY);
  handleError(res, "publishing recording fragment header");

  while (file.available()) {
    file.read(buffer.data(), bufferSize);
    res = mqtt.publishFragmentBody(MqttTopic::RECORDER, buffer.data(),
                                   bufferSize);
    handleError(res, "publishing recording fragment body");
  }

  res = mqtt.publishFragmentTrailer(MqttTopic::RECORDER);
  handleError(res, "publishing recording fragment trailer");

  return 0;
}

void recordToMqtt(Recorder &recorder, Mqtt &mqtt) {
  if (!mqtt.client) {
    Serial.println("MQTT client is not initialized, please setup mqtt");
    return;
  }

  if (SPIFFS.exists(WAV_FILE_PATH)) {
    SPIFFS.remove(WAV_FILE_PATH);
    delay(50);
  }

  auto write = SPIFFS.open(WAV_FILE_PATH, FILE_WRITE);
  if (!write) {
    Serial.printf("Fail to open recording file: %s\n", WAV_FILE_PATH);
    return;
  }

  Serial.println("Recording started for 5 seconds...");
  recorder.readToFile(write, RECORDER_BUFFER_SIZE, RECORDER_DURATION);
  write.close();

  auto read = SPIFFS.open(WAV_FILE_PATH, FILE_READ);
  if (!read) {
    Serial.printf("Fail to open recording result: %s\n", WAV_FILE_PATH);
    return;
  }

  Serial.printf("Recording saved at: %s (size=%u bytes)\n", WAV_FILE_PATH,
                (unsigned)read.size());

  Serial.println("Publishing recording to MQTT...");
  auto res = publishRecordingToMqtt(read, mqtt, RECORDER_BUFFER_SIZE);
  if (res != 0) {
    Serial.printf("Fail to publish recording to MQTT: %d\n", res);
    read.close();
    return;
  }

  Serial.println("Recording published to MQTT");
  read.close();
}