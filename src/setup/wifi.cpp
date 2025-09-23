#include "core/flash.h"
#include "core/serial.h"

#include <Arduino.h>
#include <WiFi.h>

#define WAITING_TIME_BEFORE_GIVE_UP 5000
#define WIFI_CONFIG_PATH "/wifi.bin"

struct WiFiConfig
{
  char ssid[33];
  char password[65];
};

void connectWiFi(String ssid, String password)
{
  WiFi.disconnect(true);
  WiFi.begin(ssid, password);

  Serial.printf("Connecting to Wi-Fi %s\n", ssid);

  const int wait_delay = 300;
  int wait_time = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");

    delay(wait_delay);
    wait_time += wait_delay;

    if (wait_time >= WAITING_TIME_BEFORE_GIVE_UP)
    {
      Serial.println("\nCannot connect to WiFi. Check the inputted password.");
      WiFi.disconnect(true);
      return;
    }
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
};

void connectWiFi(WiFiConfig config)
{
  connectWiFi(String(config.ssid), String(config.password));
};

void setupWiFi()
{
  WiFiConfig config;
  Serial.println("Loading WiFi configuration from flash");
  if (load(WIFI_CONFIG_PATH, reinterpret_cast<unsigned char *>(&config),
           sizeof(WiFiConfig)))
  {
    Serial.println("WiFi configuration loaded");
    connectWiFi(config);
  }
}

void configureWiFi()
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

  WiFiConfig config;
  strncpy(config.ssid, ssid.c_str(), sizeof(config.ssid) - 1);
  strncpy(config.password, password.c_str(), sizeof(config.password) - 1);

  Serial.println("Saving WiFi configuration to flash");
  if (store(WIFI_CONFIG_PATH, reinterpret_cast<unsigned char *>(&config),
            sizeof(WiFiConfig)))
  {
    Serial.println("WiFi configuration saved");
  }
  else
  {
    Serial.println("Failed to save WiFi configuration to flash");
  }

  connectWiFi(config);
}
