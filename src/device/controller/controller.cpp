#include "device/controller/controller.h"

#include "mqtt/protocol.h"
#include "core/utils.h"

createTag(VERIFY_RESULT);

void subscribeToCommand(Mqtt &mqtt, uint8_t lampSwitchPin, uint8_t fanSwitchPin)
{
  mqtt.subscribe(
      MqttTopic::CONTROLLER,
      [lampSwitchPin, fanSwitchPin](const char *msg, size_t size)
      {
        if (strcmp(msg, MqttControllerCommand::LAMP_ON) == 0)
        {
          digitalWrite(lampSwitchPin, HIGH);
        }
        else if (strcmp(msg, MqttControllerCommand::LAMP_OFF) == 0)
        {
          digitalWrite(lampSwitchPin, LOW);
        }
        else if (strcmp(msg, MqttControllerCommand::FAN_ON) == 0)
        {
          digitalWrite(fanSwitchPin, HIGH);
        }
        else if (strcmp(msg, MqttControllerCommand::FAN_OFF) == 0)
        {
          digitalWrite(fanSwitchPin, LOW);
        }
      });
}