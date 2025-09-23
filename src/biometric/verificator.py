import io
from typing import Protocol

import numpy as np
from pydantic import BaseModel

type AudioInput = str | bytes | io.BytesIO


class CommandMatcher(Protocol):
    def predict_command(self, transcription: str) -> str | None: ...


class VoiceEmbedder(Protocol):
    def get_embeddings(self) -> dict[str, np.ndarray]: ...
    def embed(self, audio: AudioInput) -> np.ndarray: ...
    def calculate_similarity(self, emb1: np.ndarray, emb2: np.ndarray) -> float: ...


class Transcriber(Protocol):
    def transcribe(self, audio: AudioInput) -> str: ...


class VerificationResult(BaseModel):
    verified: bool
    similarity: float
    transcription: str
    command: str | None = None
    reference: str | None = None


class Verificator:
    def __init__(
        self,
        command_matcher: CommandMatcher,
        embedder: VoiceEmbedder,
        transcriber: Transcriber,
    ):
        self.command_matcher = command_matcher
        self.embedder = embedder
        self.transcriber = transcriber

    def verify(
        self,
        audio: AudioInput,
        # Inclusive
        threshold: float = 0.50,
        # Whether to stop verification on first successful embedding match
        stop_at_verified=False,
    ) -> VerificationResult:
        embeddings = self.embedder.get_embeddings()
        input = self.embedder.embed(audio)

        best_similarity = 0.0
        best_reference = None
        for key, embedding in embeddings.items():
            similarity = self.embedder.calculate_similarity(input, embedding)

            if similarity > best_similarity:
                best_similarity = similarity
                best_reference = key

            if stop_at_verified and similarity >= threshold:
                break

        verified = bool(best_similarity > threshold)
        if not verified:
            return VerificationResult(
                verified=False,
                similarity=float(best_similarity),
                transcription="",
                command=None,
                reference=None,
            )

        text = self.transcriber.transcribe(audio)
        command = self.command_matcher.predict_command(text)

        return VerificationResult(
            verified=verified,
            similarity=best_similarity,
            transcription=text,
            command=command,
            reference=best_reference,
        )
