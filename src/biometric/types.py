from pydantic import BaseModel
from typing import BinaryIO

type AudioInput = str | bytes | BinaryIO


class VerificationResult(BaseModel):
    verified: bool
    similarity: float
    transcription: str
    command: str | None = None
    reference: str | None = None
