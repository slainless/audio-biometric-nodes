from .verificator import Transcriber, AudioInput
import torch
from transformers import AutoProcessor, VoxtralForConditionalGeneration


class VoxtralTranscriber(Transcriber):
    def __init__(self, device="cuda" if torch.cuda.is_available() else "cpu"):
        voxtral_repo_id = "mistralai/Voxtral-Mini-3B-2507"
        self.voxtral_repo_id = voxtral_repo_id
        self.audio_processor = AutoProcessor.from_pretrained(voxtral_repo_id)
        self.transcriber = VoxtralForConditionalGeneration.from_pretrained(
            voxtral_repo_id, torch_dtype=torch.bfloat16, device_map=device
        )
        self.device = device

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
