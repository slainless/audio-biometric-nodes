from concurrent.futures import ThreadPoolExecutor
from threading import Lock
import logging

from .ffi import Protocol

logger = logging.getLogger(__name__)

type Partial = tuple[list[str], bytearray]


class MessageAssembler:
    _partials: dict[str, Partial] = {}

    def __init__(self):
        self._partials = {}
        self._lock = Lock()
        self._executor = ThreadPoolExecutor()

        def on_assembled(id: str, data: bytes):
            pass

        self.on_assembled = on_assembled

    def add_message(self, id: str, type: str, message: bytes):
        if type in [Protocol.MqttMessageType.MESSAGE]:
            logger.warning(f'Message type "{type}" should not be passed here')
            return

        with self._lock:
            types, buffer = self._partials.setdefault(id, ([], bytearray()))

            match type:
                case Protocol.MqttMessageType.FRAGMENT_HEADER:
                    if len(types) > 0:
                        logger.warning(
                            f"Attempting to add fragment header to non-empty partial for id: {id}. Discarding previous partial."
                        )
                        types.clear()
                        buffer.clear()

                    types.append(type)
                case Protocol.MqttMessageType.FRAGMENT_BODY:
                    if len(types) == 0:
                        logger.warning(
                            f"Attempting to add fragment body to empty partial for id: {id}. Discarding message."
                        )
                        return

                    types.append(type)
                    buffer.extend(message)
                case Protocol.MqttMessageType.FRAGMENT_TRAILER:
                    if len(types) == 0:
                        logger.warning(
                            f"Attempting to add fragment trailer to empty partial for id: {id}. Discarding message."
                        )
                        return

                    if types[-1] != Protocol.MqttMessageType.FRAGMENT_BODY:
                        logger.warning(
                            f"Attempting to add fragment trailer to partial for id: {id} that does not contain a fragment body. Discarding message."
                        )
                        return

                    types.append(type)
                    buffer.extend(message)
                    data = bytes(buffer)

                    types.clear()
                    buffer.clear()

                    self._assembledCallback(id, data)
                case _:
                    raise ValueError(f"Invalid message type: {type}")

    def _assembledCallback(self, id: str, data: bytes):
        self._executor.submit(self.on_assembled, id, data)
