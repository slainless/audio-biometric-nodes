from concurrent.futures import ThreadPoolExecutor
from typing import Any

from .server import MqttServer
from .ffi import Protocol

from ...biometric import Verificator

import logging

logger = logging.getLogger(__name__)


class VerificationHandler:
    def __init__(
        self, verificator: Verificator, threshold: float = 0.5, stop_at_unverified=True
    ):
        self.threshold = threshold
        self.verificator = verificator
        self.stop_at_unverified = stop_at_unverified
        self._executor = ThreadPoolExecutor(4)

    def __call__(self, server: MqttServer, id: str, data: bytes) -> Any:
        logger.info(f"[{id}] Verifying audio...")
        result = self.verificator.verify(
            data, threshold=self.threshold, stop_at_unverified=self.stop_at_unverified
        )
        self._executor.submit(server.send_verification_result, "", result)
        logger.info(f"[{id}] Verification result:\n{result}")

        if not result.verified:
            logger.info(f"[{id}] Verification failed")
            return

        if result.command is None:
            logger.info(f"[{id}] No command found")
            return

        if result.command not in Protocol.MqttControllerCommand.Values:
            logger.info(f"[{id}] Unknown command '{result.command}'")
            return

        logger.info(f"[{id}] Sending command '{result.command}'")
        server.send_command("", result.command)


class SampleHandler:
    def __init__(self, verificator: Verificator):
        self.verificator = verificator

    def __call__(
        self, server: MqttServer, id: str, sample_name: str, data: bytes
    ) -> Any:
        logger.info(f"[{id} # {sample_name}] Storing audio sample...")
        self.verificator.embedder.set_reference(sample_name, data)
        logger.info(f"[{id} # {sample_name}] Audio sample stored")
