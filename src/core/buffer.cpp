#include "core/buffer.h"

CircularBuffer::CircularBuffer(uint8_t *buf, size_t capacity) : buffer(buf), capacity(capacity), head(0), tail(0)
{
  mutex = xSemaphoreCreateMutex();
}

CircularBuffer::~CircularBuffer()
{
  if (mutex)
    vSemaphoreDelete(mutex);
}

size_t CircularBuffer::write(const uint8_t *data, size_t bytes)
{
  size_t written = 0;
  xSemaphoreTake(mutex, portMAX_DELAY);
  for (size_t i = 0; i < bytes; ++i)
  {
    size_t next = (head + 1) % capacity;
    if (next == tail)
      break; // buffer full
    buffer[head] = data[i];
    head = next;
    ++written;
  }
  xSemaphoreGive(mutex);
  return written;
}

// Read bytes from buffer, returns number of bytes actually read
size_t CircularBuffer::read(uint8_t *out, size_t bytes)
{
  size_t readBytes = 0;
  xSemaphoreTake(mutex, portMAX_DELAY);
  for (size_t i = 0; i < bytes; ++i)
  {
    if (tail == head)
      break; // buffer empty
    out[i] = buffer[tail];
    tail = (tail + 1) % capacity;
    ++readBytes;
  }
  xSemaphoreGive(mutex);
  return readBytes;
}

// Number of bytes currently in buffer
size_t CircularBuffer::size() const
{
  if (head >= tail)
    return head - tail;
  return capacity - tail + head;
}

bool CircularBuffer::isEmpty() const { return head == tail; }
bool CircularBuffer::isFull() const { return ((head + 1) % capacity) == tail; }