#pragma once

#include <SPIFFS.h>
#include <driver/i2s.h>

namespace AudioConfig
{
  constexpr i2s_channel_t channelMode = I2S_CHANNEL_MONO;
  constexpr i2s_bits_per_sample_t bitsPerSample = I2S_BITS_PER_SAMPLE_32BIT;
  constexpr i2s_bits_per_sample_t hardwareBitsPerSample = I2S_BITS_PER_SAMPLE_24BIT;
  constexpr uint8_t bytesPerSample = bitsPerSample / 8 * channelMode; // (bitsPerSample / 8) * channelMode
  constexpr uint8_t validBytesPerSample = hardwareBitsPerSample / 8 * channelMode;
  constexpr bool isLeftJustified = true; // INMP441 is left-justified in 32-bit word
}

using RecordingCallback = std::function<void(const int32_t *data)>;

class Recorder
{
public:
  Recorder(i2s_port_t deviceIndex, int sdInPin, int sckPin, int wsPin);

  void begin(uint32_t sampleRate);
  void end();
  bool readFor(unsigned long durationMs, size_t bufferSize, RecordingCallback callback = nullptr);

private:
  i2s_port_t deviceIndex;
  uint32_t sampleRate;
  i2s_pin_config_t i2s_pin_config;

  void writeWavHeader(File &file, uint32_t totalSamples);
};

const size_t calculateActualSizeFor(const unsigned long durationMs, const uint32_t sampleRate);