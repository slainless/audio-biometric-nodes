#include "core/audio.h"
#include <cstdint>
#include <driver/i2s.h>
#include <vector>

ESP_STATIC_ASSERT(
    AudioConfig::hardwareBitsPerSample <= AudioConfig::bitsPerSample,
    "Currently only supports hardwareBitsPerSample >= bitsPerSample");

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
                             .bits_per_sample = AudioConfig::bitsPerSample,
                             // bro this is stupid, PIO framework library for arduino-esp32 is outdated!!!
                             // this is a bug from older i2s driver:
                             // https://github.com/espressif/esp-idf/issues/6625
                             .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
                             .communication_format = I2S_COMM_FORMAT_STAND_I2S,
                             .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
                             .dma_buf_count = 8,
                             .dma_buf_len = 1024,
                             .use_apll = true};

  i2s_driver_install(deviceIndex, &i2s_config, 0, NULL);
  i2s_set_pin(deviceIndex, &i2s_pin_config);
}

void Recorder::end() { i2s_driver_uninstall(deviceIndex); }

void writeSamples(
    size_t bytesRead,
    const int32_t *samplingBuffer,
    size_t &bytesWritten, // caller-maintained write position
    uint8_t *fsBuffer,
    size_t fsBufferSize, // MUST be actual capacity of fsBuffer
    File &file)
{
  if (AudioConfig::bytesPerSample == 0)
    return;

  size_t numOfSamples = bytesRead / AudioConfig::bytesPerSample;
  if (numOfSamples == 0)
    return;

  for (size_t s = 0; s < numOfSamples; ++s)
  {
    // Ensure enough room, otherwise flush
    if (bytesWritten + AudioConfig::validBytesPerSample > fsBufferSize)
    {
      file.write(fsBuffer, bytesWritten);
      bytesWritten = 0;
    }

    // Work with the raw unsigned bit-pattern to avoid sign-extension issues
    // when shifting. The microphone's 24 valid bits live either left-justified
    // (top 24 bits of the 32-bit word) or right-justified; shift accordingly.
    uint32_t raw = static_cast<uint32_t>(samplingBuffer[s]);
    uint32_t u;
    uint8_t b0, b1, b2;

    if (AudioConfig::isLeftJustified)
    {
      // left-justified: valid 24 bits are in bits [31:8]
      u = (raw >> 8) & 0x00FFFFFFu;
    }
    else
    {
      // right-justified: valid 24 bits are in bits [23:0]
      u = raw & 0x00FFFFFFu;
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

bool Recorder::readFor(unsigned long durationMs, size_t bufferSize, RecordingCallback callback)
{
  if (bufferSize % AudioConfig::bytesPerSample != 0)
  {
    Serial.println("[ERR] Buffer size must be a multiple of bytes per sample");
    return false;
  }

  size_t bytesRead = 0;
  std::vector<int32_t> samplingBuffer(bufferSize / AudioConfig::bytesPerSample);

  const size_t totalSamples = (static_cast<size_t>(sampleRate) * durationMs) / 1000;
  const size_t loops = totalSamples * AudioConfig::bytesPerSample / bufferSize;
  const size_t readTargetBytes = loops * bufferSize;
  const size_t actualTargetBytes = loops * bufferSize * AudioConfig::validBytesPerSample / AudioConfig::bytesPerSample;

  Serial.printf("Reading %u bytes (and saving %u bytes) in %u ms (Read loops: %u)\n",
                readTargetBytes, actualTargetBytes, durationMs, loops);

  for (size_t i = 0; i < loops; i++)
  {
    esp_err_t r = i2s_read(deviceIndex, samplingBuffer.data(), bufferSize, &bytesRead,
                           portMAX_DELAY);
    if (r != ESP_OK)
    {
      Serial.printf("[ERR] i2s_read failed: %d\n", r);
      return false;
    }

    if (callback)
    {
      callback(samplingBuffer.data());
    }

    yield();
  }

  return true;
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
  // Num channels
  *(uint16_t *)(header + 22) = AudioConfig::channelMode;
  // Sample rate
  *(uint32_t *)(header + 24) = sampleRate;
  // Byte rate = SampleRate * BlockAlign
  *(uint32_t *)(header + 28) = sampleRate * AudioConfig::validBytesPerSample;
  // Block align (bytes per sample frame)
  *(uint16_t *)(header + 32) = AudioConfig::validBytesPerSample;
  // Bits per sample (per channel)
  *(uint16_t *)(header + 34) = AudioConfig::hardwareBitsPerSample;

  memcpy(header + 36, "data", 4);
  *(uint32_t *)(header + 40) = actualTargetBytes;

  file.write(header, 44);
}

const size_t calculateActualSizeFor(const unsigned long durationMs, const uint32_t sampleRate)
{
  const size_t totalSamples = (static_cast<size_t>(sampleRate) * durationMs) / 1000;
  return totalSamples * AudioConfig::validBytesPerSample;
}