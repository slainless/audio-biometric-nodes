#include <SPIFFS.h>

bool store(const char *path, uint8_t *address, size_t size) {
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.printf("Failed to open file for writing: %s\n", path);
    return false;
  }

  file.write(address, size);
  file.close();
  return true;
};

bool load(const char *path, uint8_t *address, size_t size) {
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) {
    Serial.printf("Failed to open file for reading: %s\n", path);
    return false;
  }

  file.read(address, size);
  file.close();
  return true;
};