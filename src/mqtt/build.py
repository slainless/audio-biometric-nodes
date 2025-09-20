import os

Import("env")
lib = SharedLibrary(target="protocol.dll", source=["protocol.cpp"])
Default(lib)
