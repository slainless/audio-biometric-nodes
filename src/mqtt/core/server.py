import paho.mqtt.client as mqtt
from paho.mqtt.reasoncodes import ReasonCode
from paho.mqtt.client import ConnectFlags, DisconnectFlags, MQTTMessage
from paho.mqtt.properties import Properties
from paho.mqtt.enums import CallbackAPIVersion
from typing import Any
from textwrap import dedent
import logging

from .message import MessageAssembler

logger = logging.getLogger(__name__)


class BiometricMqttServer:
    def __init__(
        self,
        broker_host: str,
        broker_port: int,
        recorder_topic: str,
        keepalive: int = 60,
    ):
        self._client = mqtt.Client(CallbackAPIVersion.VERSION2)

        self._message_assembler = MessageAssembler()

        self._broker_host = broker_host
        self._broker_port = broker_port
        self._keepalive = keepalive

        self._recorder_topic = recorder_topic

        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_subscribe = self._on_subscribe
        self._client.on_message = self._on_message

    def start_forever(self):
        self._client.connect(self._broker_host, self._broker_port, self._keepalive)
        self._client.loop_forever(retry_first_connection=True)

    def stop(self):
        self._client.disconnect()

    def _on_connect(
        self,
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
                Connected to MQTT broker at {self._broker_host}:{self._broker_port}.
                - Reason code: {rc}
                - Flags: {flags}
                - Properties: {properties}
            """).strip()
        )
        logger.info(f"Subscribing to topic: {self._recorder_topic}")
        client.subscribe(self._recorder_topic)

    def _on_disconnect(
        self,
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

    def _on_subscribe(
        self,
        client: mqtt.Client,
        userdata: Any,
        mid: int,
        reason_code_list: list[ReasonCode],
        properties: Properties | None = None,
    ):
        """Callback for when a subscription is successful."""
        logger.info(
            dedent(f"""
                Successfully subscribed to topic: {self._recorder_topic}.
                - Reason code: {reason_code_list}
                - Message ID: {mid}
                - Properties: {properties}
            """).strip()
        )

    def _on_message(self, client: mqtt.Client, userdata: Any, msg: MQTTMessage):
        """Callback for when a message is received."""

        logger.info(f"Payload received:\n{msg.payload}")
