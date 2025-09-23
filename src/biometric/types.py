from pydantic import BaseModel
from typing import BinaryIO, runtime_checkable, Protocol


@runtime_checkable
class Seekable(Protocol):
    def seek(self, offset: int, whence: int = 0) -> int: ...


type AudioInput = str | bytes | BinaryIO


class VerificationResult(BaseModel):
    verified: bool
    similarity: float
    transcription: str
    command: str | None = None
    reference: str | None = None
