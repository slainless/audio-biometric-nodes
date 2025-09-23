from typing import Protocol

from fastapi import FastAPI


class FastAPIAttachment(Protocol):
    def attach(self, app: FastAPI): ...
