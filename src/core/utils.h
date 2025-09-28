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

#define timedFor(timeContainer, durationMs, codeBlock) \
  if (millis() - timeContainer >= durationMs)          \
  {                                                    \
    codeBlock;                                         \
    timeContainer = millis();                          \
  }

#define controlledTask(semaphore, timeContainer, throttleMs, whenActive)                \
  if (millis() - timeContainer >= throttleMs && xSemaphoreTake(semaphore, 0) == pdTRUE) \
  {                                                                                     \
    timeContainer = millis();                                                           \
    auto __semaphore__ = semaphore;                                                     \
    whenActive;                                                                         \
    xSemaphoreGive(semaphore);                                                          \
  }

#define controlledTaskWhenBusy(semaphore, whenBusy) \
  if (xSemaphoreTake(semaphore, 0) == pdTRUE)       \
  {                                                 \
    xSemaphoreGive(semaphore);                      \
  }                                                 \
  else                                              \
  {                                                 \
    whenBusy;                                       \
  }

#define controlledReturn         \
  xSemaphoreGive(__semaphore__); \
  return;

#define assignString(a, b)                  \
  ESP_STATIC_ASSERT(sizeof(b) < sizeof(a)); \
  strncpy(a, b, sizeof(a));                 \
  a[sizeof(a) - 1] = '\0';

#ifdef PRINT_TASK_HIGHWATERMARK
#define __printHighWaterMark                                                           \
  {                                                                                    \
    UBaseType_t highWater = uxTaskGetStackHighWaterMark(NULL);                         \
    ESP_LOGE("RTOS_TASK_HIGHWATERMARK", "Task1 remaining stack: %u words", highWater); \
  }
#else
#define __printHighWaterMark \
  {                          \
  }
#endif

#define createTag(tagName) static const char *TAG = #tagName;