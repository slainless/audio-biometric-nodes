#include "WiFi.h"
#include "setup/serial.h"
#include <Arduino.h>

#define WAITING_TIME_BEFORE_GIVE_UP 5000

void setupWiFi() {
  Serial.print("Access point SSID: ");
  auto ssid = blockingReadStringUntil('\n');
  Serial.print(ssid);
  Serial.println();

  Serial.print("Access point password: ");
  auto password = blockingReadStringUntil('\n');
  Serial.print(password);
  Serial.println();

  ssid.trim();
  password.trim();

  WiFi.disconnect(true);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to Wi-Fi");

  const int wait_delay = 300;
  int wait_time = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");

    delay(wait_delay);
    wait_time += wait_delay;

    if (wait_time >= WAITING_TIME_BEFORE_GIVE_UP) {
      Serial.println("\nCannot connect to WiFi. Check the inputted password.");
      WiFi.disconnect(true);
      return;
    }
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}
