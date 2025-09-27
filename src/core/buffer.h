#include <cstdint>
#include <cstddef>
#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class CircularBuffer
{
public:
  CircularBuffer(uint8_t *buf, size_t capacity);
  ~CircularBuffer();

  size_t write(const uint8_t *data, size_t bytes);
  size_t read(uint8_t *out, size_t bytes);
  size_t size() const;
  bool isEmpty() const;
  bool isFull() const;

private:
  uint8_t *buffer;
  size_t capacity;
  size_t head;
  size_t tail;
  SemaphoreHandle_t mutex;
};
