#pragma once

#include "core/audio.h"
#include "core/mqtt.h"

void recordToMqtt(Recorder &recorder, Mqtt &mqtt, uint8_t blinkingPin);