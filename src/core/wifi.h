#pragma once

struct WiFiConfig
{
  char ssid[33];
  char password[65];
};

namespace WiFiConfigurer
{
  int store(WiFiConfig &config);
  int setup(WiFiConfig &config);
  int serialPrompt(WiFiConfig &config);
}