from difflib import SequenceMatcher
from .verificator import CommandMatcher


class DiffCommandMatcher(CommandMatcher):
    def __init__(self, commands: dict[str, str]):
        self.commands = commands

    def predict_command(self, transcription: str) -> str | None:
        """Match transcribed text to voice commands"""
        best_cmd, best_score = None, 0
        for cmd in self.commands:
            score = SequenceMatcher(None, transcription, cmd).ratio()
            if score > best_score:
                best_cmd, best_score = cmd, score

        if best_score > 0.7 and best_cmd is not None:
            return self.commands[best_cmd]
        return
