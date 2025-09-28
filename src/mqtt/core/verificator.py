from typing import Any

from .server import MqttServer
from .ffi import Protocol

from ...biometric import Verificator

import logging

_logger = logging.getLogger(__name__)


class VerificationHandler:
    def __init__(self, verificator: Verificator, threshold: float = 0.5):
        self.threshold = threshold
        self.verificator = verificator

    def __call__(self, server: MqttServer, id: str, data: bytes) -> Any:
        _logger.info(f"[{id}] Verifying audio...")
        result = self.verificator.verify(data, threshold=self.threshold)
        _logger.info(f"[{id}] Verification result:\n{result}")

        if not result.verified:
            _logger.info(f"[{id}] Verification failed")
            return

        if result.command is None:
            _logger.info(f"[{id}] No command found")
            return

        if result.command not in Protocol.MqttControllerCommand.Values:
            _logger.info(f"[{id}] Unknown command '{result.command}'")
            return

        _logger.info(f"[{id}] Sending command '{result.command}'")
        server.send_command("", result.command)


class SampleHandler:
    def __init__(self, verificator: Verificator):
        self.verificator = verificator

    def __call__(
        self, server: MqttServer, id: str, sample_name: str, data: bytes
    ) -> Any:
        _logger.info(f"[{id} # {sample_name}] Storing audio sample...")
        self.verificator.embedder.set_reference(sample_name, data)
        _logger.info(f"[{id} # {sample_name}] Audio sample stored")
