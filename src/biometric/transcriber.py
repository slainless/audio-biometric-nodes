from typing import Protocol
from pathlib import Path
import io
import wave

from .types import AudioInput
from transformers import AutoProcessor, VoxtralForConditionalGeneration
import tempfile

import sherpa_ncnn
import numpy as np

import torch
from scipy.signal import resample_poly

import whisper

current_dir = Path(__file__).parent
k2_indonesian_model = (
    current_dir
    / ".."
    / ".."
    / ".models"
    / "sherpa-ncnn-pruned-transducer-stateless7-streaming-id"
)


class Transcriber(Protocol):
    def transcribe(self, audio: AudioInput) -> str: ...


class WhisperTranscriber(Transcriber):
    def __init__(self, model_type: str = "base"):
        self.model = whisper.load_model(model_type)

    def transcribe(self, audio: AudioInput) -> str:
        if not isinstance(audio, (str, Path)):
            with tempfile.NamedTemporaryFile(delete=False) as f:
                f.write(audio)
                audio = f.name

        result = self.model.transcribe(audio, language="id")
        return result["text"].strip()


class KaldiIndonesianTranscriber(Transcriber):
    @staticmethod
    def read_wave_to_float32(audio: AudioInput, target_sample_rate: int) -> np.ndarray:
        """
        Read WAV from path / bytes / file-like, return mono np.float32 samples [-1, 1]
        at target_sample_rate. Raises ValueError for unsupported formats or if scipy
        is required for resampling but not available.
        """
        if isinstance(audio, (bytes, bytearray, memoryview)):
            fobj = io.BytesIO(audio)
        else:
            fobj = audio

        with wave.open(fobj, "rb") as wf:
            sr = wf.getframerate()
            nch = wf.getnchannels()
            sw = wf.getsampwidth()  # bytes per sample
            nframes = wf.getnframes()
            frames = wf.readframes(nframes)

        # decode raw bytes into int array
        if sw == 1:  # 8-bit PCM unsigned
            samples = np.frombuffer(frames, dtype=np.uint8).astype(np.float32)
            # convert unsigned 0..255 to -1..1
            samples = (samples - 128.0) / 128.0
        elif sw == 2:  # 16-bit signed
            samples = np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0
        elif sw == 3:  # 24-bit signed (little-endian)
            # convert 3-byte chunks to int32 then to float
            a = np.frombuffer(frames, dtype=np.uint8)
            if a.size % 3 != 0:
                raise ValueError("24-bit WAV has incomplete frames")
            a = a.reshape(-1, 3)
            # little endian -> construct int32
            vals = (
                a[:, 0].astype(np.int32)
                | (a[:, 1].astype(np.int32) << 8)
                | (a[:, 2].astype(np.int32) << 16)
            )
            # sign extend
            sign_mask = 1 << 23
            vals = np.where(vals & sign_mask, vals - (1 << 24), vals)
            samples = vals.astype(np.float32) / float(1 << 23)
        elif sw == 4:  # 32-bit signed
            samples = (
                np.frombuffer(frames, dtype=np.int32).astype(np.float32) / 2147483648.0
            )
        else:
            raise ValueError(f"Unsupported sample width: {sw} bytes")

        # handle multi-channel -> mono (average channels)
        if nch > 1:
            samples = samples.reshape(-1, nch).mean(axis=1)

        # resample if needed
        if sr != target_sample_rate:
            if resample_poly is None:
                raise ValueError(
                    f"Input sample rate {sr} != target {target_sample_rate}. "
                    "Install scipy (scipy.signal.resample_poly) or provide audio at the target rate."
                )
            # compute integer resample factors
            # use resample_poly for good quality: up/down factors as integers
            g = np.gcd(sr, target_sample_rate)
            up = target_sample_rate // g
            down = sr // g
            samples = resample_poly(samples, up, down).astype(np.float32)

        # ensure dtype float32
        return samples.astype(np.float32)

    def transcribe(self, audio: AudioInput) -> str:
        recognizer = sherpa_ncnn.Recognizer(
            tokens=f"{k2_indonesian_model}/tokens.txt",
            encoder_param=f"{k2_indonesian_model}/encoder_jit_trace-pnnx.ncnn.param",
            encoder_bin=f"{k2_indonesian_model}/encoder_jit_trace-pnnx.ncnn.bin",
            decoder_param=f"{k2_indonesian_model}/decoder_jit_trace-pnnx.ncnn.param",
            decoder_bin=f"{k2_indonesian_model}/decoder_jit_trace-pnnx.ncnn.bin",
            joiner_param=f"{k2_indonesian_model}/joiner_jit_trace-pnnx.ncnn.param",
            joiner_bin=f"{k2_indonesian_model}/joiner_jit_trace-pnnx.ncnn.bin",
            num_threads=4,
        )
        samples = self.read_wave_to_float32(audio, recognizer.sample_rate)

        recognizer.accept_waveform(recognizer.sample_rate, samples)

        # tail padding to flush partial tokens (0.5s)
        tail_paddings = np.zeros(int(recognizer.sample_rate * 0.5), dtype=np.float32)
        recognizer.accept_waveform(recognizer.sample_rate, tail_paddings)

        recognizer.input_finished()
        return recognizer.text


class VoxtralTranscriber(Transcriber):
    def __init__(self, device="cuda" if torch.cuda.is_available() else "cpu"):
        voxtral_repo_id = "mistralai/Voxtral-Mini-3B-2507"
        self.voxtral_repo_id = voxtral_repo_id
        self.audio_processor = AutoProcessor.from_pretrained(voxtral_repo_id)
        self.transcriber = VoxtralForConditionalGeneration.from_pretrained(
            voxtral_repo_id, torch_dtype=torch.bfloat16, device_map=device
        )
        self.device = device

    def transcribe(self, audio: AudioInput) -> str:
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
