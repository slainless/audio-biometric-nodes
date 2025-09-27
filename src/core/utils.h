#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#define ensureSetup(code, setup, message)                 \
  code = setup;                                           \
  if (code != ESP_OK)                                     \
  {                                                       \
    while (true)                                          \
    {                                                     \
      ESP_LOGE("SETUP", "Failed setting up %s", message); \
      delay(2000);                                        \
    }                                                     \
  }

#define __printHighWaterMark                                                           \
  {                                                                                    \
    UBaseType_t highWater = uxTaskGetStackHighWaterMark(NULL);                         \
    ESP_LOGD("RTOS_TASK_HIGHWATERMARK", "Task1 remaining stack: %u words", highWater); \
  }