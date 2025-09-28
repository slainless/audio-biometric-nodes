#include "core/control.h"
#include "core/remotexy.h"
#include "core/utils.h"

namespace RemoteXYConfigurer
{
  RemoteXYConfigCode storeConfig(WiFiConfig &wifiConfig, MqttConfig &mqttConfig)
  {
    createTag(REMOTEXY);

    if (RemoteXY.input_wifi_ssid[0] == '\0')
    {
      return RemoteXYConfigCode::MISSING_WIFI_SSID;
    }

    if (RemoteXY.input_wifi_password[0] == '\0')
    {
      return RemoteXYConfigCode::MISSING_WIFI_PASSWORD;
    }

    if (RemoteXY.input_mqtt_host[0] == '\0')
    {
      return RemoteXYConfigCode::MISSING_MQTT_HOST;
    }

    if (RemoteXY.input_mqtt_port <= 0)
    {
      return RemoteXYConfigCode::MISSING_MQTT_PORT;
    }

    ESP_LOGI(TAG,
             "Received configuration:\n"
             "- WiFi SSID: %s\n"
             "- WiFi SSID: %s\n"
             "- MQTT Host: %s\n"
             "- MQTT Port: %d\n"
             "- MQTT Use SSL: %d\n",
             RemoteXY.input_wifi_ssid,
             RemoteXY.input_wifi_password,
             RemoteXY.input_mqtt_host,
             RemoteXY.input_mqtt_port,
             RemoteXY.input_mqtt_use_ssl);

    auto ssid = String(RemoteXY.input_wifi_ssid);
    auto password = String(RemoteXY.input_wifi_password);
    auto host = String(RemoteXY.input_mqtt_host);

    ssid.trim();
    password.trim();
    host.trim();

    sprintf(wifiConfig.ssid, ssid.c_str());
    sprintf(wifiConfig.password, password.c_str());

    sprintf(mqttConfig.host, host.c_str());
    mqttConfig.port = RemoteXY.input_mqtt_port;
    mqttConfig.useSsl = RemoteXY.input_mqtt_use_ssl;

    auto res = WiFiConfigurer::store(wifiConfig);
    if (res != ESP_OK)
    {
      return RemoteXYConfigCode::WIFI_FLASH_CONFIG_ERROR;
    }

    res = MqttConfigurer::store(mqttConfig);
    if (res != ESP_OK)
    {
      return RemoteXYConfigCode::MQTT_FLASH_CONFIG_ERROR;
    }

    return RemoteXYConfigCode::OK;
  }

  int updateConfigToRemote(WiFiConfig &wifiConfig, MqttConfig &mqttConfig)
  {
    if (wifiConfig.ssid[0] != '\0')
    {
      sprintf(RemoteXY.input_wifi_ssid, wifiConfig.ssid);
    }
    if (wifiConfig.password[0] != '\0')
    {
      sprintf(RemoteXY.input_wifi_password, wifiConfig.password);
    }
    if (mqttConfig.host[0] != '\0')
    {
      sprintf(RemoteXY.input_mqtt_host, mqttConfig.host);
    }
    if (mqttConfig.port != 0)
    {
      RemoteXY.input_mqtt_port = mqttConfig.port;
    }
    if (mqttConfig.useSsl)
    {
      RemoteXY.input_mqtt_use_ssl = 1;
    }

    return 0;
  }

  void configureNetwork(WiFiConfig &wifiConfig, MqttConfig &mqttConfig, Mqtt &mqtt)
  {
    createTag(REMOTEXY);

    sprintf(RemoteXY.value_config_status, "Configuring...");
    RemoteXY_Handler();

    ESP_LOGI(TAG, "Storing configuration");
    auto res = storeConfig(wifiConfig, mqttConfig);
    if (res == RemoteXYConfigCode::MISSING_WIFI_SSID)
    {
      sprintf(RemoteXY.value_config_status, "Missing WiFi SSID");
    }
    else if (res == RemoteXYConfigCode::MISSING_WIFI_PASSWORD)
    {
      sprintf(RemoteXY.value_config_status, "Missing WiFi Password");
    }
    else if (res == RemoteXYConfigCode::MISSING_MQTT_HOST)
    {
      sprintf(RemoteXY.value_config_status, "Missing MQTT Host");
    }
    else if (res == RemoteXYConfigCode::MISSING_MQTT_PORT)
    {
      sprintf(RemoteXY.value_config_status, "Missing MQTT Port");
    }

    if (res != RemoteXYConfigCode::OK)
    {
      ESP_LOGI(TAG, "Fail to store configuration, caused by: %d", res);
      return;
    }

    ESP_LOGI(TAG, "Configuration stored.");
    auto wifiError = WiFiConfigurer::reconnect(wifiConfig);
    if (wifiError != ESP_OK)
    {
      ESP_LOGI(TAG, "Fail to store configure WiFi, caused by: %d", wifiError);
      sprintf(RemoteXY.value_config_status, "WiFi Error. Check SSID/Password");
      return;
    }

    auto mqttError = MqttConfigurer::reconnect(mqttConfig, mqtt);
    if (mqttError != ESP_OK)
    {
      ESP_LOGI(TAG, "Fail to store configure MQTT, caused by: %d", mqttError);
      sprintf(RemoteXY.value_config_status, "MQTT Error. Check Host/Port");
      return;
    }

    sprintf(RemoteXY.value_config_status, "Connected");
    ESP_LOGI(TAG, "WiFi and MQTT successfully configured");
  }

  void resetVerifyResult()
  {
    sprintf(RemoteXY.value_recorder_command, "-");
    sprintf(RemoteXY.value_recorder_reference, "-");
    sprintf(RemoteXY.value_recorder_similarity_status, "-");
    sprintf(RemoteXY.value_recorder_transcription, "-");
    sprintf(RemoteXY.value_recorder_verified_status, "-");
  }
}