from fastapi import FastAPI
from pydantic import BaseModel

from ..mqtt import MqttServer
from .fastapi import FastAPIAttachment


class SendMessagePayload(BaseModel):
    message: str


class DebugAttachment(FastAPIAttachment):
    def __init__(self, mqtt_server: MqttServer) -> None:
        self.mqtt_server = mqtt_server

    def attach(self, app: FastAPI):
        app.post("/DEBUG/send_message")(self.send_message)

    def send_message(self, payload: SendMessagePayload):
        self.mqtt_server.send_command("", payload.message)
