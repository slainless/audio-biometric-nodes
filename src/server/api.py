from fastapi import FastAPI, UploadFile, File
from fastapi.responses import JSONResponse, PlainTextResponse

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

        return PlainTextResponse(status_code=404, content="Voice embedding not found")

    async def verify_voice(self, file: UploadFile = File(...)):
        return self.verificator.verify(file.file)

    async def clear(self):
        self.verificator.embedder.source.clear()
        return JSONResponse(
            dict(removed_embeddings=list(self.verificator.embedder.source.all().keys()))
        )

    async def list_voices(self):
        return JSONResponse(
            dict(
                registered_embeddings=list(
                    self.verificator.embedder.source.all().keys()
                )
            )
        )
