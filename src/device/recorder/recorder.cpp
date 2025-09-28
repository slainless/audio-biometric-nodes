#include "core/mqtt.h"
#include "core/utils.h"
#include "core/remotexy.h"

#include "mqtt/protocol.h"

#include "device/recorder/recorder.h"

createTag(VERIFY_RESULT);

void subscribeToVerifyResult(Mqtt &mqtt)
{
  mqtt.subscribe(
      MqttTopic::VERIFY_RESULT,
      [](auto msg, auto size)
      {
        if (size < 11)
        {
          ESP_LOGI(TAG, "Message size is too short: %d", size);
          return;
        }

        size_t index = 0;

        bool verified = msg[index++];
        float similarity;
        memcpy(&similarity, &msg[index], 4);
        index += 4;

        uint8_t referenceSize = msg[index++];
        char reference[referenceSize + 1] = {0};
        memcpy(&reference, &msg[index], referenceSize);
        index += referenceSize;

        uint8_t transcriptionSize = msg[index++];
        char transcription[transcriptionSize + 1] = {0};
        memcpy(&transcription, &msg[index], transcriptionSize);
        index += transcriptionSize;

        uint8_t commandSize = msg[index++];
        char command[commandSize + 1] = {0};
        memcpy(&command, &msg[index], commandSize);

        ESP_LOGI(
            TAG,
            "Received verify result:\n"
            "- Verified: %d\n"
            "- Similarity: %f\n"
            "- Reference: %s\n"
            "- Transcription: %s\n"
            "- Command: %s",
            verified,
            similarity,
            reference,
            transcription,
            command);

        if (verified)
        {
          sprintf(RemoteXY.value_recorder_verified_status, "Terverifikasi");
        }
        else
        {
          sprintf(RemoteXY.value_recorder_verified_status, "Tidak terverifikasi");
        }

        sprintf(RemoteXY.value_recorder_similarity_status, "%f%", similarity * 100);
        sprintf(RemoteXY.value_recorder_reference, reference);
        sprintf(RemoteXY.value_recorder_transcription, transcription);
        sprintf(RemoteXY.value_recorder_command, command);
      });
}