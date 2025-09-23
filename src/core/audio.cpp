#include "core/audio.h"
#include <cstdint>
#include <driver/i2s.h>
#include <vector>

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
                             .bits_per_sample = I2S_BITS_PER_SAMPLE,
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
  size_t bytesRead;
  std::vector<uint8_t> buffer(bufferSize);

  size_t targetBytes = sampleRate * I2S_BYTES_PER_SAMPLE * durationMs / 1000;
  size_t loops = targetBytes / bufferSize;
  size_t normalizedTargetBytes = loops * bufferSize;

  Serial.printf("Reading %u bytes in %u ms (Read loops: %u)\n",
                normalizedTargetBytes, durationMs, loops);

  writeWavHeader(file, normalizedTargetBytes);
  for (size_t i = 0; i < loops; i++)
  {
    esp_err_t r = i2s_read(deviceIndex, buffer.data(), bufferSize, &bytesRead,
                           portMAX_DELAY);
    if (r != ESP_OK)
    {
      Serial.printf("[ERR] i2s_read failed: %d\n", r);
      return false;
    }
    file.write(buffer.data(), bytesRead);
    yield(); // let background tasks run
  }

  return true;
}

void Recorder::writeWavHeader(File &file, uint32_t totalSamples)
{
  uint32_t dataSize = totalSamples * I2S_BYTES_PER_SAMPLE;
  uint8_t header[44];

  memcpy(header, "RIFF", 4);
  *(uint32_t *)(header + 4) = dataSize + 36;
  memcpy(header + 8, "WAVE", 4);
  memcpy(header + 12, "fmt ", 4);
  *(uint32_t *)(header + 16) = 16;
  *(uint16_t *)(header + 20) = 1;
  *(uint16_t *)(header + 22) = I2S_CHANNEL_MODE;
  *(uint32_t *)(header + 24) = sampleRate;
  *(uint32_t *)(header + 28) = sampleRate * I2S_BYTES_PER_SAMPLE;
  *(uint16_t *)(header + 32) = I2S_BYTES_PER_SAMPLE;
  *(uint16_t *)(header + 34) = I2S_BITS_PER_SAMPLE;
  memcpy(header + 36, "data", 4);
  *(uint32_t *)(header + 40) = dataSize;

  file.write(header, 44);
}
