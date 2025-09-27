#include <Arduino.h>

std::function<void(bool)> createBlinker(uint8_t blinkingPin, unsigned long interval)
{
  if (blinkingPin == 0)
  {
    return [](bool restore = false) {};
  }

  auto initialPinState = digitalRead(blinkingPin);
  auto pinState = initialPinState;
  auto lastMillis = millis();

  return [pinState, lastMillis, blinkingPin, interval, initialPinState](bool restore = false) mutable
  {
    if (restore)
    {
      digitalWrite(blinkingPin, initialPinState);
      return;
    }

    auto current = millis();
    if (current - lastMillis < interval)
    {
      return;
    }

    lastMillis = current;
    if (pinState == HIGH)
    {
      digitalWrite(blinkingPin, LOW);
      pinState = LOW;
    }
    else
    {
      digitalWrite(blinkingPin, HIGH);
      pinState = HIGH;
    }
  };
}