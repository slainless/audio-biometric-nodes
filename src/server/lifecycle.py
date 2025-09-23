from contextlib import asynccontextmanager
import logging

from fastapi import FastAPI

from .fastapi import FastAPIAttachment

from ..mqtt import MqttServer

_logger = logging.getLogger(__name__)


class BiometricServerLifecycle:
    def __init__(self, mqtt_server: MqttServer, *attachments: FastAPIAttachment):
        self.mqtt_server = mqtt_server
        self.attachments = attachments

    @asynccontextmanager
    async def _lifespan(self, app: FastAPI):
        self.mqtt_server.start()
        _logger.info("Connected to MQTT server application")

        for attachment in self.attachments:
            attachment.attach(app)

        yield

        self.mqtt_server.stop()
        _logger.info("Disconnected from MQTT server application")

    def lifespan(self):
        return self._lifespan
