from fastapi import FastAPI, Response, UploadFile, File

from ..biometric import Verificator


class ApiServer:
    def __init__(self, verificator: Verificator):
        self.verificator = verificator

    def attach(self, app: FastAPI):
        app.put("/voice/{voice_name}")(self.register_voice)
        app.delete("/voice/{voice_name}")(self.remove_voice)
        app.post("/voice/verify")(self.verify_voice)
        app.post("/voice/clear")(self.clear)
        app.get("/voice")(self.list_voices)

    async def register_voice(self, voice_name: str, file: UploadFile = File(...)):
        self.verificator.embedder.set_reference(voice_name, file.file)

    async def remove_voice(self, voice_name: str):
        if self.verificator.embedder.remove_reference(voice_name):
            return

        return Response(status_code=404, content="Voice embedding not found")

    async def verify_voice(self, file: UploadFile = File(...)):
        return self.verificator.verify(file.file)

    async def clear(self):
        self.verificator.embedder.source.clear()
        return dict(removed_embeddings=self.verificator.embedder.source.all().keys())

    async def list_voices(self):
        return dict(registered_embeddings=self.verificator.embedder.source.all().keys())
