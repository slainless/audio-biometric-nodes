import io
from typing import Protocol

import numpy as np
import torch
import torchaudio
from speechbrain.inference import EncoderClassifier
from transformers import VoxtralForConditionalGeneration, AutoProcessor
from pydantic import BaseModel


class CommandMatcher(Protocol):
    def predict_command(self, transcription: str) -> str | None: ...


class EmbeddingSource(Protocol):
    def all(self) -> dict[str, np.ndarray]: ...
    def get(self, key: str) -> np.ndarray | None: ...
    def set(self, key: str, value: np.ndarray): ...
    def remove(self, key: str): ...


type AudioInput = str | bytes | io.BytesIO


class VerificationResult(BaseModel):
    verified: bool
    similarity: float
    transcription: str
    command: str | None = None
    reference: str | None = None


class Verificator:
    def __init__(
        self,
        embedding_source: EmbeddingSource,
        command_matcher: CommandMatcher,
        device="cuda" if torch.cuda.is_available() else "cpu",
    ):
        embedder = EncoderClassifier.from_hparams(
            source="speechbrain/spkrec-ecapa-voxceleb",
        )
        if not embedder:
            raise ValueError("Failed to load speaker embedding model")
        self.embedder = embedder.to(device)
        self.device = device

        voxtral_repo_id = "mistralai/Voxtral-Mini-3B-2507"
        self.voxtral_repo_id = voxtral_repo_id
        self.audio_processor = AutoProcessor.from_pretrained(voxtral_repo_id)
        self.transcriber = VoxtralForConditionalGeneration.from_pretrained(
            voxtral_repo_id, torch_dtype=torch.bfloat16, device_map=device
        )

        self.embedding_source = embedding_source
        self.command_matcher = command_matcher

    @staticmethod
    def _normalize_audio(audio_or_path: AudioInput) -> str | io.BytesIO:
        if (
            isinstance(audio_or_path, bytes)
            or isinstance(audio_or_path, bytearray)
            or isinstance(audio_or_path, memoryview)
        ):
            return io.BytesIO(audio_or_path)
        else:
            return audio_or_path

    def extract_embedding(self, audio: AudioInput) -> np.ndarray:
        """Extract speaker embedding from audio file"""

        signal, sr = torchaudio.load(self._normalize_audio(audio))
        if sr != 16000:
            transform = torchaudio.transforms.Resample(orig_freq=sr, new_freq=16000)
            signal = transform(signal)
        embedding = self.embedder.encode_batch(signal)
        embedding = embedding.squeeze().detach().cpu().numpy()
        return embedding

    def transcribe_audio(self, audio: AudioInput) -> str:
        inputs = self.audio_processor.apply_transcription_request(
            language="en", audio=audio, model_id=self.voxtral_repo_id
        )
        print(type(inputs))
        inputs = inputs.to(self.device, dtype=torch.bfloat16)
        print(type(inputs))

        outputs = self.transcriber.generate(**inputs, max_new_tokens=500)
        decoded_outputs = self.audio_processor.batch_decode(
            outputs[:, inputs.input_ids.shape[1] :], skip_special_tokens=True
        )
        print(type(decoded_outputs))
        return " ".join(decoded_outputs).strip()

    def set_reference_audio(self, key: str, audio: AudioInput):
        """Register a new user with their voice embedding"""
        embedding = self.extract_embedding(audio)
        self.embedding_source.set(key, embedding)

    def verify_user(
        self,
        audio_path: str,
        # Inclusive
        threshold: float = 0.50,
        # Whether to stop verification on first successful embedding match
        stop_at_verified=False,
    ) -> VerificationResult:
        embeddings = self.embedding_source.all()
        test_embedding = self.extract_embedding(audio_path)

        best_similarity = 0.0
        best_reference = None
        for key, embedding in embeddings.items():
            similarity = np.dot(test_embedding, embedding) / (
                np.linalg.norm(test_embedding) * np.linalg.norm(embedding)
            )

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

        text = self.transcribe_audio(audio_path)
        command = self.command_matcher.predict_command(text)

        return VerificationResult(
            verified=verified,
            similarity=best_similarity,
            transcription=text,
            command=command,
            reference=best_reference,
        )
