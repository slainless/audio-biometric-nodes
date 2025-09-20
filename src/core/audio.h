#include <I2S.h>
#include <SPIFFS.h>

#define I2S_SAMPLE_PER_BIT 16
#define I2S_AUDIO_MODE 1
#define I2S_BYTES_PER_SAMPLE (I2S_SAMPLE_PER_BIT / 8) * I2S_AUDIO_MODE

class Recorder {
public:
  Recorder(uint8_t deviceIndex, uint8_t sdInPin, uint8_t sckPin, uint8_t fsPin);

  void begin(uint32_t sampleRate);
  void end();
  void readToFile(File &file, size_t bufferSize, unsigned long durationMs);

private:
  I2SClass i2s;
  uint32_t sampleRate;

  size_t read(uint8_t *buffer, size_t bufferSize);
  void writeWavHeader(File &file, uint32_t totalSamples);
};
