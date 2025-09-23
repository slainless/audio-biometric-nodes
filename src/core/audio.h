#pragma once

#include <SPIFFS.h>
#include <driver/i2s.h>

#define I2S_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define I2S_CHANNEL_MODE I2S_CHANNEL_MONO
#define I2S_BYTES_PER_SAMPLE (I2S_BITS_PER_SAMPLE / 8) * I2S_CHANNEL_MODE

class Recorder
{
public:
  Recorder(i2s_port_t deviceIndex, int sdInPin, int sckPin, int wsPin);

  void begin(uint32_t sampleRate);
  void end();
  bool readToFile(File &file, size_t bufferSize, unsigned long durationMs);

private:
  i2s_port_t deviceIndex;
  uint32_t sampleRate;
  i2s_pin_config_t i2s_pin_config;

  void writeWavHeader(File &file, uint32_t totalSamples);
};
