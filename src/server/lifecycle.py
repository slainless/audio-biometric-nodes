from contextlib import asynccontextmanager
import logging

from fastapi import FastAPI

from ..mqtt import MqttServer
from .api import ApiServer

_logger = logging.getLogger(__name__)


class BiometricServerLifecycle:
    def __init__(self, mqtt_server: MqttServer, api_server: ApiServer):
        self.mqtt_server = mqtt_server
        self.api_server = api_server

    @asynccontextmanager
    async def lifespan(self, app: FastAPI):
        self.mqtt_server.start()
        _logger.info("Connected to MQTT server application")

        self.api_server.attach(app)

        yield

        self.mqtt_server.stop()
        _logger.info("Disconnected from MQTT server application")
