#include "core/wifi.h"
#include "core/filesystem.h"
#include "core/serial.h"

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_log.h>

#define WAITING_TIME_BEFORE_GIVE_UP 5000
#define WIFI_CONFIG_PATH "/spiffs/wifi.bin"

static const char *TAG = "WIFI";

void connectWiFi(String ssid, String password)
{
  WiFi.disconnect(true);
  WiFi.begin(ssid, password);

  ESP_LOGI(TAG, "Connecting to Wi-Fi %s", ssid);

  const int wait_delay = 300;
  int wait_time = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");

    delay(wait_delay);
    wait_time += wait_delay;

    if (wait_time >= WAITING_TIME_BEFORE_GIVE_UP)
    {
      Serial.println();
      ESP_LOGI(TAG, "Cannot connect to WiFi. Check the inputted password.");
      WiFi.disconnect(true);
      return;
    }
  }

  Serial.println();
  ESP_LOGI(TAG, "Connected with IP: %s", WiFi.localIP());
};

void connectWiFi(WiFiConfig config)
{
  connectWiFi(String(config.ssid), String(config.password));
};

namespace WiFiConfigurer
{
  int setup(WiFiConfig &config)
  {
    ESP_LOGI(TAG, "Loading WiFi configuration from flash");
    if (FileSystem::load(WIFI_CONFIG_PATH, reinterpret_cast<unsigned char *>(&config),
                         sizeof(WiFiConfig)))
    {
      ESP_LOGI(TAG, "WiFi configuration loaded");
      connectWiFi(config);
    }

    return ESP_OK;
  }

  int serialPrompt(WiFiConfig &config)
  {
    Serial.print("Access point SSID: ");
    auto ssid = blockingReadStringUntil();
    Serial.print(ssid);
    Serial.println();

    Serial.print("Access point password: ");
    auto password = blockingReadStringUntil();
    Serial.print(password);
    Serial.println();

    ssid.trim();
    password.trim();

    strncpy(config.ssid, ssid.c_str(), sizeof(config.ssid) - 1);
    strncpy(config.password, password.c_str(), sizeof(config.password) - 1);

    ESP_LOGI(TAG, "Saving WiFi configuration to flash");
    if (FileSystem::store(WIFI_CONFIG_PATH, reinterpret_cast<unsigned char *>(&config),
                          sizeof(WiFiConfig)))
    {
      ESP_LOGI(TAG, "WiFi configuration saved");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to save WiFi configuration to flash");
      return 1;
    }

    connectWiFi(config);
    return ESP_OK;
  }
}