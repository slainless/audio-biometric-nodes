import paho.mqtt.client as mqtt
from paho.mqtt.reasoncodes import ReasonCode
from paho.mqtt.client import ConnectFlags, DisconnectFlags, MQTTMessage
from paho.mqtt.properties import Properties
from paho.mqtt.enums import CallbackAPIVersion
from typing import Any
from textwrap import dedent
import logging

from .message import MessageAssembler
from .ffi import Protocol

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

        self._message_assembler.on_assembled = self._on_verify
        self.on_verify = self._on_verify

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

        if len(msg.payload) < 6:
            logger.error(f"Payload is too short: {len(msg.payload)}")
            return

        type = msg.payload[0:4].decode()
        if type not in Protocol.MqttMessageType.Values:
            logger.error(f"Invalid message type: {type}")
            return

        id_size = msg.payload[4]
        if len(msg.payload) < 5 + id_size:
            logger.error(
                f"Payload is too short, expecting at minimum: {5 + id_size}, got: {len(msg.payload)}"
            )
            return

        id = msg.payload[5 : 5 + id_size].decode()
        data = msg.payload[5 + id_size :]

        if type == Protocol.MqttMessageType.MESSAGE:
            logger.info(f"Message received from {id}:\n{data.decode()}")
            return

        if type == Protocol.MqttMessageType.FRAGMENT_HEADER:
            header = data.decode()
            if header not in Protocol.MqttHeader.Values:
                logger.error(f"Invalid header type received: {header}")
                return

            self._message_assembler.add_message(id, type, bytes())

        logger.info(f"Message received from {id} with type: {type}")
        self._message_assembler.add_message(id, type, data)

    def _on_verify(self, id: str, data: bytes):
        """Callback for when a message is assembled."""
        logger.info(f"Message assembled:\n{data}")
