from concurrent.futures import ThreadPoolExecutor
from threading import Lock
from typing import TypedDict
import logging

from .ffi import Protocol

logger = logging.getLogger(__name__)


class AssembledMessage(TypedDict):
    data: bytearray
    type_sequence: list[str]
    header: str


class MessageAssembler:
    _partials: dict[str, AssembledMessage] = {}

    def __init__(self):
        self._partials = {}
        self._lock = Lock()
        self._executor = ThreadPoolExecutor()

        def on_assembled(id: str, header: str, data: bytearray):
            pass

        self.on_assembled = on_assembled

    def add_message(self, id: str, type: str, message: bytes):
        if type in [Protocol.MqttMessageType.MESSAGE]:
            logger.warning(f'Message type "{type}" should not be passed here')
            return

        with self._lock:
            assembled = self._partials.setdefault(
                id, AssembledMessage(data=bytearray(), type_sequence=[], header="")
            )

            match type:
                case Protocol.MqttMessageType.FRAGMENT_HEADER:
                    if len(assembled["type_sequence"]) > 0:
                        logger.warning(
                            f"Attempting to add fragment header to non-empty partial for id: {id}. Discarding previous partial."
                        )
                        assembled["type_sequence"].clear()
                        assembled["data"].clear()

                    assembled["header"] = str(message)
                    assembled["type_sequence"].append(type)
                case Protocol.MqttMessageType.FRAGMENT_BODY:
                    if len(assembled["type_sequence"]) == 0:
                        logger.warning(
                            f"Attempting to add fragment body to empty partial for id: {id}. Discarding message."
                        )
                        return

                    assembled["type_sequence"].append(type)
                    assembled["data"].extend(message)
                case Protocol.MqttMessageType.FRAGMENT_TRAILER:
                    if len(assembled["type_sequence"]) == 0:
                        logger.warning(
                            f"Attempting to add fragment trailer to empty partial for id: {id}. Discarding message."
                        )
                        return

                    if (
                        assembled["type_sequence"][-1]
                        != Protocol.MqttMessageType.FRAGMENT_BODY
                    ):
                        logger.warning(
                            f"Attempting to add fragment trailer to partial for id: {id} that does not contain a fragment body. Discarding message."
                        )
                        return

                    assembled["type_sequence"].append(type)
                    assembled["data"].extend(message)

                    data = bytearray(assembled["data"])
                    header = assembled["header"]

                    assembled["type_sequence"].clear()
                    assembled["data"].clear()
                    assembled["header"] = ""

                    self._assembledCallback(id, header, data)
                case _:
                    raise ValueError(f"Invalid message type: {type}")

    def _assembledCallback(self, id: str, header: str, data: bytearray):
        self._executor.submit(self.on_assembled, id, header, data)
