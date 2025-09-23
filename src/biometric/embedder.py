from typing import Protocol, BinaryIO
import io

import torch
import torchaudio
import numpy as np

from .types import AudioInput
from speechbrain.inference import EncoderClassifier


class VoiceEmbedder(Protocol):
    source: "EmbeddingSource"

    def get_embeddings(self) -> dict[str, np.ndarray]: ...
    def embed(self, audio: AudioInput) -> np.ndarray: ...
    def calculate_similarity(self, emb1: np.ndarray, emb2: np.ndarray) -> float: ...

    def get_reference(self, key: str) -> np.ndarray | None: ...
    def set_reference(self, key: str, audio: AudioInput): ...
    def remove_reference(self, key: str) -> bool: ...


class EmbeddingSource(Protocol):
    def all(self) -> dict[str, np.ndarray]: ...
    def get(self, key: str) -> np.ndarray | None: ...
    def set(self, key: str, value: np.ndarray): ...
    def remove(self, key: str) -> bool: ...
    def clear(self): ...


class SpeechbrainEmbedder(VoiceEmbedder):
    def __init__(
        self,
        source: EmbeddingSource,
        device="cuda" if torch.cuda.is_available() else "cpu",
    ):
        model = EncoderClassifier.from_hparams(
            source="speechbrain/spkrec-ecapa-voxceleb",
        )
        if not model:
            raise ValueError("Failed to load speaker embedding model")
        self.model = model.to(device)
        self.device = device

        self.source = source

    @staticmethod
    def _normalize_audio(audio_or_path: AudioInput) -> str | BinaryIO:
        if isinstance(audio_or_path, (bytes, bytearray, memoryview)):
            return io.BytesIO(audio_or_path)
        else:
            return audio_or_path

    def embed(self, audio: AudioInput) -> np.ndarray:
        """Extract speaker embedding from audio file"""

        signal, sr = torchaudio.load(self._normalize_audio(audio))
        if signal.shape[0] > 1:
            signal = signal.mean(dim=0, keepdim=True)

        if sr != 16000:
            transform = torchaudio.transforms.Resample(orig_freq=sr, new_freq=16000)
            signal = transform(signal)
        embedding = self.model.encode_batch(signal)
        embedding = embedding.squeeze().detach().cpu().numpy()
        return embedding

    def get_embeddings(self) -> dict[str, np.ndarray]:
        return self.source.all()

    def calculate_similarity(self, emb1: np.ndarray, emb2: np.ndarray) -> float:
        return np.dot(emb1, emb2) / (np.linalg.norm(emb1) * np.linalg.norm(emb2))

    def get_reference(self, key: str) -> np.ndarray | None:
        return self.source.get(key)

    def set_reference(self, key: str, audio: AudioInput):
        embedding = self.embed(audio)
        self.source.set(key, embedding)

    def remove_reference(self, key: str):
        return self.source.remove(key)
