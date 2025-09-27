#include <functional>

std::function<void(bool)> createBlinker(uint8_t blinkingPin, unsigned long interval = 500);