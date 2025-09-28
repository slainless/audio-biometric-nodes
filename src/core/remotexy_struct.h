#pragma once

#include <cstdint>

#pragma pack(push, 1)
// this structure defines all the variables and events of your control interface
struct RemoteXY_t
{
  // input variables
  uint8_t button_recorder;      // =1 if button pressed, else =0, from 0 to 1
  char input_voice_name[21];    // string UTF8 end zero
  uint8_t button_sampler;       // =1 if button pressed, else =0, from 0 to 1
  char input_mqtt_host[33];     // string UTF8 end zero
  uint8_t input_mqtt_use_ssl;   // =1 if switch ON and =0 if OFF, from 0 to 1
  int16_t input_mqtt_port;      // -32768 .. +32767
  char input_wifi_ssid[33];     // string UTF8 end zero
  char input_wifi_password[65]; // string UTF8 end zero
  uint8_t button_store_config;  // =1 if button pressed, else =0, from 0 to 1

  // output variables
  uint8_t led_recorder;                      // from 0 to 1
  char value_recorder_status[65];            // string UTF8 end zero
  char value_recorder_command[65];           // string UTF8 end zero
  char value_recorder_transcription[33];     // string UTF8 end zero
  char value_recorder_similarity_status[11]; // string UTF8 end zero
  char value_recorder_verified_status[33];   // string UTF8 end zero
  uint8_t led_sampler;                       // from 0 to 1
  char value_sampler_status[65];             // string UTF8 end zero
  char value_config_status[65];              // string UTF8 end zero

  // other variable
  uint8_t connect_flag; // =1 if wire connected, else =0
};
#pragma pack(pop)

extern RemoteXY_t RemoteXY;