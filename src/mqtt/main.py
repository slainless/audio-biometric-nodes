#!/usr/bin/env python3
"""
MQTT subscriber for ESP32 Audio Biometric project.
Subscribes to the recorder topic and prints all received messages.
Reads topic definition directly from C protocol.h file.
"""

# import paho.mqtt.client as mqtt
# import json
# import time
# import sys
import os

# import re
from pathlib import Path
from .ffi import build_protocol_accessor

current_dir = Path(__file__).parent

MQTT_BROKER_HOST = os.getenv("MQTT_BROKER_HOST") or "localhost"
MQTT_BROKER_PORT = int(os.getenv("MQTT_BROKER_PORT") or 1883)
MQTT_KEEPALIVE = int(os.getenv("MQTT_KEEPALIVE") or 60)

protocol = build_protocol_accessor()


# def on_connect(client, userdata, flags, rc):
#     """Callback for when the client connects to the broker."""
#     if rc == 0:
#         print(f"Connected to MQTT broker at {MQTT_BROKER_HOST}:{MQTT_BROKER_PORT}")
#         print(f"Subscribing to topic: {RECORDER_TOPIC}")
#         client.subscribe(RECORDER_TOPIC)
#     else:
#         print(f"Failed to connect to MQTT broker. Return code: {rc}")


# def on_disconnect(client, userdata, rc):
#     """Callback for when the client disconnects from the broker."""
#     print(f"Disconnected from MQTT broker. Return code: {rc}")


# def on_message(client, userdata, msg):
#     """Callback for when a message is received."""
#     try:
#         # Decode the message payload
#         payload = msg.payload.decode("utf-8")

#         # Print message details
#         print(f"\n--- Received Message ---")
#         print(f"Topic: {msg.topic}")
#         print(f"QoS: {msg.qos}")
#         print(f"Retain: {msg.retain}")
#         print(f"Payload: {payload}")

#         # Try to parse as JSON for better formatting
#         try:
#             json_data = json.loads(payload)
#             print(f"JSON Data:")
#             print(json.dumps(json_data, indent=2))
#         except json.JSONDecodeError:
#             print(f"Raw Data: {payload}")

#         print("--- End Message ---\n")

#     except Exception as e:
#         print(f"Error processing message: {e}")
#         print(f"Raw payload: {msg.payload}")


# def on_subscribe(client, userdata, mid, granted_qos):
#     """Callback for when a subscription is successful."""
#     print(f"Successfully subscribed to topic: {RECORDER_TOPIC}")
#     print(f"Granted QoS: {granted_qos}")


# def main():
#     """Main function to run the MQTT subscriber."""
#     print("Starting MQTT Subscriber for ESP32 Audio Biometric")
#     print(f"Broker: {MQTT_BROKER_HOST}:{MQTT_BROKER_PORT}")
#     print(f"Topic: {RECORDER_TOPIC}")
#     print("Press Ctrl+C to stop\n")

#     # Create MQTT client
#     client = mqtt.Client()

#     # Set callbacks
#     client.on_connect = on_connect
#     client.on_disconnect = on_disconnect
#     client.on_message = on_message
#     client.on_subscribe = on_subscribe

#     try:
#         # Connect to broker
#         client.connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT, MQTT_KEEPALIVE)

#         # Start the loop
#         client.loop_forever()

#     except KeyboardInterrupt:
#         print("\nShutting down MQTT subscriber...")
#         client.disconnect()
#         sys.exit(0)
#     except Exception as e:
#         print(f"Error: {e}")
#         sys.exit(1)


# if __name__ == "__main__":
#     main()
