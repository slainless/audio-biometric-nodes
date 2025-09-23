import logging
import os
from pathlib import Path

from fastapi import FastAPI

from .lifecycle import BiometricServerLifecycle

from ..biometric import (
    SpeechbrainEmbedder,
    VoxtralTranscriber,
    KaldiIndonesianTranscriber,
    DiffCommandMatcher,
    FileEmbeddingSource,
    Verificator,
)
from ..mqtt import MqttServer, Protocol, VerificationHandler
from .api import ApiServer

current_dir = Path(__file__).parent
default_embedding_file = current_dir / ".." / ".." / ".data" / "embeddings.npz"

EMBEDDING_FILE_PATH = Path(os.getenv("EMBEDDING_FILE_PATH") or default_embedding_file)

MQTT_BROKER_HOST = os.getenv("MQTT_BROKER_HOST") or "localhost"
MQTT_BROKER_PORT = int(os.getenv("MQTT_BROKER_PORT") or 1883)
MQTT_KEEPALIVE = int(os.getenv("MQTT_KEEPALIVE") or 60)

RECORDER_TOPIC = Protocol.MqttTopic.RECORDER

_logger = logging.getLogger(__name__)

logging.basicConfig(level=logging.DEBUG)

embedding_source = FileEmbeddingSource(EMBEDDING_FILE_PATH)
command_matcher = DiffCommandMatcher(
    {
        "nyalakan lampu": "LAMP_ON",
        "matikan lampu": "LAMP_OFF",
        "hidupkan lampu": "LAMP_ON",
        "tutup lampu": "LAMP_OFF",
        "buka lampu": "LAMP_ON",
        "lampu hidup": "LAMP_ON",
        "lampu mati": "LAMP_OFF",
    }
)
verificator = Verificator(
    command_matcher, SpeechbrainEmbedder(embedding_source), KaldiIndonesianTranscriber()
)

api_server = ApiServer(verificator)
mqtt_server = MqttServer(
    MQTT_BROKER_HOST, MQTT_BROKER_PORT, RECORDER_TOPIC, MQTT_KEEPALIVE
)
mqtt_server.on_verify = VerificationHandler(verificator, threshold=0.35)

app = FastAPI(lifespan=BiometricServerLifecycle(mqtt_server, api_server).lifespan())
