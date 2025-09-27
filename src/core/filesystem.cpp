#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_spiffs.h>

static const char *TAG = "SPIFFS";

namespace FileSystem
{
  int setup()
  {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
      if (ret == ESP_FAIL)
      {
        ESP_LOGE(TAG, "Failed to mount or format filesystem");
      }
      else if (ret == ESP_ERR_NOT_FOUND)
      {
        ESP_LOGE(TAG, "Failed to find SPIFFS partition");
      }
      else
      {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
      }
      return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to get SPIFFS info (%s)", esp_err_to_name(ret));
      return ret;
    }

    ESP_LOGI(TAG, "SPIFFS total: %d, used: %d", total, used);
    return ESP_OK;
  }

  bool store(const char *path, uint8_t *address, size_t size)
  {
    FILE *file = fopen(path, "w");
    if (!file)
    {
      ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
      return false;
    }

    size_t written = fwrite(address, 1, size, file);
    if (written != size)
    {
      ESP_LOGE(TAG, "Failed to write all bytes to file: %s", path);
      fclose(file);
      return false;
    }

    fclose(file);
    return true;
  };

  bool load(const char *path, uint8_t *address, size_t size)
  {
    FILE *file = fopen(path, "rb"); // open for reading in binary mode
    if (!file)
    {
      ESP_LOGE(TAG, "Failed to open file for reading: %s\n", path);
      return false;
    }

    size_t read = fread(address, 1, size, file);
    if (read != size)
    {
      ESP_LOGE(TAG, "Failed to read all bytes from file: %s\n", path);
      fclose(file);
      return false;
    }

    fclose(file);
    return true;
  };
}
