#include "core/record.h"

#include <SPIFFS.h>

#define WAV_FILE_PATH "/recording.wav"
#define RECORDER_BUFFER_SIZE 512
#define RECORDER_DURATION 5000

void recordToMqtt(Recorder &recorder) {
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
}