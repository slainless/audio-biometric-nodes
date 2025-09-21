#!/usr/bin/env python3

import paho.mqtt.client as mqtt
from paho.mqtt.reasoncodes import ReasonCode
from paho.mqtt.client import ConnectFlags, DisconnectFlags, MQTTMessage
from paho.mqtt.properties import Properties
from paho.mqtt.enums import CallbackAPIVersion
from typing import Any
from textwrap import dedent

import json
import time
import sys
import os

import re
from pathlib import Path
from .core.ffi import Protocol
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)
current_dir = Path(__file__).parent

MQTT_BROKER_HOST = os.getenv("MQTT_BROKER_HOST") or "localhost"
MQTT_BROKER_PORT = int(os.getenv("MQTT_BROKER_PORT") or 1883)
MQTT_KEEPALIVE = int(os.getenv("MQTT_KEEPALIVE") or 60)

RECORDER_TOPIC = Protocol.MqttTopic.RECORDER


def on_connect(
    client: mqtt.Client,
    userdata: Any,
    flags: ConnectFlags,
    rc: ReasonCode,
    properties: Properties | None = None,
):
    if rc != 0:
        logger.info(f"Failed to connect to MQTT broker. Reason code: {rc}")
        return

    logger.info(
        dedent(f"""
            Connected to MQTT broker at {MQTT_BROKER_HOST}:{MQTT_BROKER_PORT}.
            - Reason code: {rc}
            - Flags: {flags}
            - Properties: {properties}
        """).strip()
    )
    logger.info(f"Subscribing to topic: {RECORDER_TOPIC}")
    client.subscribe(RECORDER_TOPIC)


def on_disconnect(
    client: mqtt.Client,
    userdata: Any,
    flags: DisconnectFlags,
    rc: ReasonCode,
    properties: Properties | None = None,
):
    """Callback for when the client disconnects from the broker."""
    logger.info(
        dedent(f"""
            Disconnected from MQTT broker.
            - Reason code: {rc}
            - Flags: {flags}
            - Properties: {properties}
        """).strip()
    )


def on_message(client: mqtt.Client, userdata: Any, msg: MQTTMessage):
    """Callback for when a message is received."""
    try:
        # Decode the message payload
        payload = msg.payload.decode("utf-8")

        # logger.info message details
        logger.info(f"\n--- Received Message ---")
        logger.info(f"Topic: {msg.topic}")
        logger.info(f"QoS: {msg.qos}")
        logger.info(f"Retain: {msg.retain}")
        logger.info(f"Payload: {payload}")

        # Try to parse as JSON for better formatting
        try:
            json_data = json.loads(payload)
            logger.info(f"JSON Data:")
            logger.info(json.dumps(json_data, indent=2))
        except json.JSONDecodeError:
            logger.info(f"Raw Data: {payload}")

        logger.info("--- End Message ---\n")

    except Exception as e:
        logger.info(f"Error processing message: {e}")
        logger.info(f"Raw payload: {msg.payload}")


def on_subscribe(
    client: mqtt.Client,
    userdata: Any,
    mid: int,
    reason_code_list: list[ReasonCode],
    properties: Properties | None = None,
):
    """Callback for when a subscription is successful."""
    logger.info(
        dedent(f"""
            Successfully subscribed to topic: {RECORDER_TOPIC}.
            - Reason code: {reason_code_list}
            - Message ID: {mid}
            - Properties: {properties}
        """).strip()
    )


def main():
    """Main function to run the MQTT subscriber."""
    logger.info("Starting MQTT Subscriber for ESP32 Audio Biometric")
    logger.info(f"Broker: {MQTT_BROKER_HOST}:{MQTT_BROKER_PORT}")
    logger.info(f"Topic: {RECORDER_TOPIC}")
    logger.info("Press Ctrl+C to stop\n")

    # Create MQTT client
    client = mqtt.Client(CallbackAPIVersion.VERSION2)

    # Set callbacks
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message
    client.on_subscribe = on_subscribe

    try:
        # Connect to broker
        client.connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT, MQTT_KEEPALIVE)

        # Start the loop
        client.loop_forever()

    except KeyboardInterrupt:
        logger.info("\nShutting down MQTT subscriber...")
        client.disconnect()
        sys.exit(0)
    except Exception as e:
        logger.info(f"Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
