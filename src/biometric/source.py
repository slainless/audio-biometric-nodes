from pathlib import Path

from .embedder import EmbeddingSource
import numpy as np


class FileEmbeddingSource(EmbeddingSource):
    def __init__(self, file: Path):
        self.store: dict[str, np.ndarray] = {}
        self.file = file
        self.read()

    def write(self):
        np.savez_compressed(self.file, allow_pickle=True, **self.store)

    def read(self):
        if self.file.exists():
            data = np.load(self.file)
            self.store = {key: data[key] for key in data}
        else:
            self.store = {}

    def all(self) -> dict[str, np.ndarray]:
        return self.store

    def get(self, key: str) -> np.ndarray | None:
        return self.store.get(key)

    def set(self, key: str, value: np.ndarray):
        self.store[key] = value
        self.write()

    def remove(self, key: str):
        if key in self.store:
            del self.store[key]
            self.write()
