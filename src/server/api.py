from fastapi import FastAPI, UploadFile, File

from ..biometric import Verificator


class ApiServer:
    def __init__(self, verificator: Verificator):
        self.verificator = verificator

    def attach(self, app: FastAPI):
        app.put("/{voice_name}")(self.register_voice)
        app.delete("/{voice_name}")(self.remove_voice)
        app.post("/verify")(self.verify_voice)

    async def register_voice(self, voice_name: str, file: UploadFile = File(...)):
        self.verificator.embedder.set_reference(voice_name, file.file)

    async def remove_voice(self, voice_name: str):
        self.verificator.embedder.remove_reference(voice_name)

    async def verify_voice(self, file: UploadFile = File(...)):
        return self.verificator.verify(file.file)
