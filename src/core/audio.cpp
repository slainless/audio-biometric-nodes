#include "core/audio.h"
#include "hal/i2s_types.h"
#include <vector>

Recorder::Recorder(uint8_t deviceIndex, uint8_t sdInPin, uint8_t sckPin,
                   uint8_t fsPin)
    : i2s(deviceIndex, 0, sdInPin, sckPin, fsPin) {}

void Recorder::begin(uint32_t sampleRate) {
  this->sampleRate = sampleRate;
  i2s.begin(esp_i2s::I2S_MODE_MASTER, sampleRate, I2S_SAMPLE_PER_BIT);
}

void Recorder::end() { i2s.end(); }

void Recorder::readToFile(File &file, size_t bufferSize,
                          unsigned long durationMs) {
  std::vector<uint8_t> buffer(bufferSize);

  size_t targetBytes = sampleRate * I2S_BYTES_PER_SAMPLE * (durationMs / 1000);
  size_t loops = targetBytes / bufferSize;

  writeWavHeader(file, targetBytes);
  for (size_t i = 0; i < loops; i++) {
    auto bytesRead = read(buffer.data(), bufferSize);
    file.write(buffer.data(), bytesRead);
    yield(); // let background tasks run
  }
}

size_t Recorder::read(uint8_t *buffer, size_t bufferSize) {
  size_t availableBytes = i2s.available();
  if (availableBytes < bufferSize) {
    return i2s.readBytes(reinterpret_cast<char *>(buffer), availableBytes);
  } else {
    return i2s.readBytes(reinterpret_cast<char *>(buffer), bufferSize);
  }
}

void Recorder::writeWavHeader(File &file, uint32_t totalSamples) {
  uint32_t dataSize = totalSamples * I2S_BYTES_PER_SAMPLE;
  uint8_t header[44];

  memcpy(header, "RIFF", 4);
  *(uint32_t *)(header + 4) = dataSize + 36;
  memcpy(header + 8, "WAVE", 4);
  memcpy(header + 12, "fmt ", 4);
  *(uint32_t *)(header + 16) = 16;
  *(uint16_t *)(header + 20) = 1;
  *(uint16_t *)(header + 22) = I2S_AUDIO_MODE;
  *(uint32_t *)(header + 24) = sampleRate;
  *(uint32_t *)(header + 28) = sampleRate * I2S_BYTES_PER_SAMPLE;
  *(uint16_t *)(header + 32) = I2S_BYTES_PER_SAMPLE;
  *(uint16_t *)(header + 34) = I2S_SAMPLE_PER_BIT;
  memcpy(header + 36, "data", 4);
  *(uint32_t *)(header + 40) = dataSize;

  file.write(header, 44);
}
