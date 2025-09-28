#include "core/wifi.h"
#include "core/mqtt.h"

namespace RemoteXYConfigurer
{
  enum class RemoteXYConfigCode
  {
    OK,
    MISSING_WIFI_SSID,
    MISSING_WIFI_PASSWORD,
    MISSING_MQTT_HOST,
    MISSING_MQTT_PORT,
    WIFI_FLASH_CONFIG_ERROR,
    MQTT_FLASH_CONFIG_ERROR,
    WIFI_ERROR,
    MQTT_ERROR
  };

  RemoteXYConfigCode storeConfig(WiFiConfig &wifiConfig, MqttConfig &mqttConfig);
  int updateConfigToRemote(WiFiConfig &wifiConfig, MqttConfig &mqttConfig);
  void configureNetwork(WiFiConfig &wifiConfig, MqttConfig &mqttConfig, Mqtt &mqtt);
}