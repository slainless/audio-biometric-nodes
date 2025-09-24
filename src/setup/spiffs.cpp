#include <SPIFFS.h>

void setupSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    while (true)
    {
      Serial.println("Fail to load SPIFFS. It is required.");
      delay(1000);
    }
  }
}