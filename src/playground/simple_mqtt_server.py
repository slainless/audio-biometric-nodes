#!/usr/bin/env python3

import sys
import os

from ..mqtt.core.server import BiometricMqttServer
from ..mqtt.core.ffi import Protocol
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

MQTT_BROKER_HOST = os.getenv("MQTT_BROKER_HOST") or "localhost"
MQTT_BROKER_PORT = int(os.getenv("MQTT_BROKER_PORT") or 1883)
MQTT_KEEPALIVE = int(os.getenv("MQTT_KEEPALIVE") or 60)

RECORDER_TOPIC = Protocol.MqttTopic.RECORDER


def main():
    """Main function to run the MQTT subscriber."""
    logger.info("Starting MQTT Subscriber")
    logger.info(f"Broker: {MQTT_BROKER_HOST}:{MQTT_BROKER_PORT}")
    logger.info(f"Topic: {RECORDER_TOPIC}")

    client = BiometricMqttServer(
        MQTT_BROKER_HOST, MQTT_BROKER_PORT, RECORDER_TOPIC, MQTT_KEEPALIVE
    )

    def on_verify(id: str, data: bytes):
        with open(f"verify_{id}.wav", "wb") as f:
            f.write(data)

    client.on_verify = on_verify

    try:
        client.start_forever()
    except KeyboardInterrupt:
        logger.info("\nInterrupted. Shutting down MQTT subscriber...")
        client.stop()
        sys.exit(0)
    except Exception as e:
        logger.info(f"Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
