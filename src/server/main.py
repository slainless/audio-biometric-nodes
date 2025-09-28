import logging
import os
from pathlib import Path

from fastapi import FastAPI

from ..biometric import (
    SpeakerWavLMEmbedder,
    WhisperTranscriber,
    DiffCommandMatcher,
    FileEmbeddingSource,
    Verificator,
)
from ..mqtt import MqttServer, Protocol, VerificationHandler, SampleHandler
from .api import ApiAttachment
from .debug import DebugAttachment
from .lifecycle import BiometricServerLifecycle

current_dir = Path(__file__).parent
default_embedding_file = current_dir / ".." / ".." / ".data" / "embeddings.npz"

EMBEDDING_FILE_PATH = Path(os.getenv("EMBEDDING_FILE_PATH") or default_embedding_file)

MQTT_BROKER_HOST = os.getenv("MQTT_BROKER_HOST") or "localhost"
MQTT_BROKER_PORT = int(os.getenv("MQTT_BROKER_PORT") or 1883)
MQTT_KEEPALIVE = int(os.getenv("MQTT_KEEPALIVE") or 60)

RECORDER_TOPIC = Protocol.MqttTopic.RECORDER

logger = logging.getLogger(__name__)

logging.basicConfig(level=logging.DEBUG)

embedding_source = FileEmbeddingSource(EMBEDDING_FILE_PATH)
# embedder = SpeechbrainEmbedder(embedding_source)
embedder = SpeakerWavLMEmbedder(embedding_source)
command_matcher = DiffCommandMatcher(
    {
        "nyalakan lampu": Protocol.MqttControllerCommand.LAMP_ON,
        "hidupkan lampu": Protocol.MqttControllerCommand.LAMP_ON,
        "lampu hidup": Protocol.MqttControllerCommand.LAMP_ON,
        "matikan lampu": Protocol.MqttControllerCommand.LAMP_OFF,
        "lampu mati": Protocol.MqttControllerCommand.LAMP_OFF,
        "nyalakan kipas": Protocol.MqttControllerCommand.FAN_ON,
        "hidupkan kipas": Protocol.MqttControllerCommand.FAN_ON,
        "kipas hidup": Protocol.MqttControllerCommand.FAN_ON,
        "matikan kipas": Protocol.MqttControllerCommand.FAN_OFF,
        "kipas mati": Protocol.MqttControllerCommand.FAN_OFF,
    }
)
verificator = Verificator(command_matcher, embedder, WhisperTranscriber())

mqtt_server = MqttServer(
    MQTT_BROKER_HOST, MQTT_BROKER_PORT, RECORDER_TOPIC, MQTT_KEEPALIVE
)
mqtt_server.on_verify = VerificationHandler(verificator, threshold=0.35)
mqtt_server.on_sample = SampleHandler(verificator)

api = ApiAttachment(verificator)
debug = DebugAttachment(mqtt_server)
lifecycle = BiometricServerLifecycle(mqtt_server, api, debug)

app = FastAPI(lifespan=lifecycle.lifespan())
