#pragma once

#include <cstddef>
#include <cstdint>

namespace FileSystem
{
  int setup();
  bool store(const char *path, uint8_t *address, size_t size);
  bool load(const char *path, uint8_t *address, size_t size);
}