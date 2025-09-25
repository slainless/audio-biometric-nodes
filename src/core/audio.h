#pragma once

#include <SPIFFS.h>
#include <driver/i2s.h>

using RecordingCallback = std::function<void(const int32_t *data)>;

class Recorder
{
public:
  Recorder(i2s_port_t deviceIndex, int sdInPin, int sckPin, int wsPin);

  void begin(uint32_t sampleRate);
  void end();
  bool readToFile(File &file, size_t bufferSize, unsigned long durationMs, RecordingCallback callback = nullptr);

private:
  i2s_port_t deviceIndex;
  uint32_t sampleRate;
  i2s_pin_config_t i2s_pin_config;

  void writeWavHeader(File &file, uint32_t totalSamples);
};