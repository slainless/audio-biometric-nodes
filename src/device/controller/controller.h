#include "core/mqtt.h"

#define CONTROLLER_IDENTIFIER "controller"

void subscribeToCommand(Mqtt &mqtt, uint8_t lampSwitchPin, uint8_t fanSwitchPin);