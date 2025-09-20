#pragma once

#include <cstddef>
#include <cstdint>

bool store(const char *path, uint8_t *address, size_t size);
bool load(const char *path, uint8_t *address, size_t size);