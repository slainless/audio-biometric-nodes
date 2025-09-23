#include "core/audio.h"
#include <cstdint>
#include <driver/i2s.h>
#include <vector>

constexpr i2s_channel_t channelMode = I2S_CHANNEL_MONO;
constexpr i2s_bits_per_sample_t bitsPerSample = I2S_BITS_PER_SAMPLE_32BIT;
constexpr i2s_bits_per_sample_t hardwareBitsPerSample = I2S_BITS_PER_SAMPLE_24BIT;
constexpr uint8_t bytesPerSample = bitsPerSample / 8 * channelMode; // (bitsPerSample / 8) * channelMode
constexpr uint8_t validBytesPerSample = hardwareBitsPerSample / 8 * channelMode;
constexpr bool samples_left_justified = true; // INMP441 is left-justified in 32-bit word

ESP_STATIC_ASSERT(hardwareBitsPerSample <= bitsPerSample, "Currently only supports hardwareBitsPerSample >= bitsPerSample");

Recorder::Recorder(i2s_port_t deviceIndex, int sdInPin, int sckPin, int wsPin)
    : i2s_pin_config({
          .bck_io_num = sckPin,
          .ws_io_num = wsPin,
          .data_out_num = I2S_PIN_NO_CHANGE,
          .data_in_num = sdInPin,
      }),
      deviceIndex(deviceIndex)
{
}

void Recorder::begin(uint32_t sampleRate)
{
  this->sampleRate = sampleRate;
  i2s_config_t i2s_config = {.mode =
                                 (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
                             .sample_rate = sampleRate,
                             .bits_per_sample = bitsPerSample,
                             .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
                             .communication_format = I2S_COMM_FORMAT_STAND_I2S,
                             .intr_alloc_flags = 0,
                             .dma_buf_count = 8,
                             .dma_buf_len = 1024,
                             .use_apll = true};

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_pin_config);
}

void Recorder::end() { i2s_driver_uninstall(I2S_NUM_0); }

bool Recorder::readToFile(File &file, size_t bufferSize,
                          unsigned long durationMs)
{
  if (bufferSize % bytesPerSample != 0)
  {
    Serial.println("[ERR] Buffer size must be a multiple of bytes per sample");
    return false;
  }

  size_t bytesRead = 0;
  size_t bytesWritten = 0;
  std::vector<int32_t> samplingBuffer(bufferSize / bytesPerSample);
  // Fit 341 samples of 3 bytes each
  uint8_t fsWriteBuffer[1023];

  const size_t totalSamples = (static_cast<size_t>(sampleRate) * durationMs) / 1000;
  const size_t loops = totalSamples * bytesPerSample / bufferSize;
  const size_t readTargetBytes = loops * bufferSize;
  const size_t actualTargetBytes = loops * bufferSize * validBytesPerSample / bytesPerSample;

  Serial.printf("Reading %u bytes (and saving %u bytes) in %u ms (Read loops: %u)\n",
                readTargetBytes, actualTargetBytes, durationMs, loops);

  writeWavHeader(file, actualTargetBytes);
  for (size_t i = 0; i < loops; i++)
  {
    esp_err_t r = i2s_read(deviceIndex, samplingBuffer.data(), bufferSize, &bytesRead,
                           portMAX_DELAY);
    if (r != ESP_OK)
    {
      Serial.printf("[ERR] i2s_read failed: %d\n", r);
      return false;
    }

    writeSamples(bytesRead, samplingBuffer.data(), bytesWritten,
                 fsWriteBuffer, sizeof(fsWriteBuffer), file);

    yield();
  }

  if (bytesWritten > 0)
  {
    file.write(fsWriteBuffer, bytesWritten);
    bytesWritten = 0;
  }

  return true;
}

void writeSamples(
    size_t bytesRead,
    const int32_t *samplingBuffer,
    size_t &bytesWritten, // caller-maintained write position
    uint8_t *fsBuffer,
    size_t fsBufferSize, // MUST be actual capacity of fsBuffer
    File &file)
{
  if (bytesPerSample == 0)
    return;

  size_t numOfSamples = bytesRead / bytesPerSample;
  if (numOfSamples == 0)
    return;

  for (size_t s = 0; s < numOfSamples; ++s)
  {
    // Ensure enough room, otherwise flush
    if (bytesWritten + validBytesPerSample > fsBufferSize)
    {
      file.write(fsBuffer, bytesWritten);
      bytesWritten = 0;
    }

    int32_t sample = samplingBuffer[s];
    uint32_t u;
    uint8_t b0, b1, b2;

    if (samples_left_justified)
    {
      u = (static_cast<uint32_t>(sample) >> 8) & 0x00FFFFFFu;
    }
    else
    {
      u = static_cast<uint32_t>(sample) & 0x00FFFFFFu;
    }

    // LSB-first for WAV 24-bit file (three bytes little-endian)
    b0 = static_cast<uint8_t>(u & 0xFFu);
    b1 = static_cast<uint8_t>((u >> 8) & 0xFFu);
    b2 = static_cast<uint8_t>((u >> 16) & 0xFFu);

    fsBuffer[bytesWritten++] = b0;
    fsBuffer[bytesWritten++] = b1;
    fsBuffer[bytesWritten++] = b2;
  }
}

void Recorder::writeWavHeader(File &file, size_t actualTargetBytes)
{
  uint8_t header[44];

  memcpy(header, "RIFF", 4);
  *(uint32_t *)(header + 4) = actualTargetBytes + 36;
  memcpy(header + 8, "WAVE", 4);

  memcpy(header + 12, "fmt ", 4);
  *(uint32_t *)(header + 16) = 16;
  *(uint16_t *)(header + 20) = 1;
  *(uint16_t *)(header + 22) = channelMode;
  *(uint32_t *)(header + 24) = sampleRate;
  *(uint32_t *)(header + 28) = sampleRate * validBytesPerSample;
  *(uint16_t *)(header + 32) = validBytesPerSample;
  *(uint16_t *)(header + 34) = hardwareBitsPerSample;

  memcpy(header + 36, "data", 4);
  *(uint32_t *)(header + 40) = actualTargetBytes;

  file.write(header, 44);
}
