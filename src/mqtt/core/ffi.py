from pathlib import Path
from cffi import FFI

current_dir = Path(__file__).parent


def _build_protocol_accessor():
    ffi = FFI()

    ffi.cdef("""
    const char *ffi_mqttProtocol(const char *protocolKey, const char *key);
    """)

    lib = ffi.dlopen(str(current_dir / "protocol.dll"))
    return _AccessorWrapper(lib, ffi)


class _AccessorWrapper:
    def __init__(self, lib, ffi):
        self.lib = lib
        self.ffi = ffi

    def __getattr__(self, protocol_key: str):
        that = self

        class _Wrapper:
            def __getattr__(self, key: str) -> str:
                result = that.lib.ffi_mqttProtocol(protocol_key.encode(), key.encode())
                return that.ffi.string(result).decode()

        return _Wrapper()


Protocol = _build_protocol_accessor()
