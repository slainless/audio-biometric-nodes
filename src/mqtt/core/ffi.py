from pathlib import Path
from cffi import FFI
from functools import cached_property, cache

current_dir = Path(__file__).parent


def _build_protocol_accessor():
    ffi = FFI()

    ffi.cdef("""
    const char *ffi_mqttProtocol(const char *protocolKey, const char *key);
    const char *const *ffi_mqttProtocolList(const char *protocolKey);
    """)

    lib = ffi.dlopen(str(current_dir / ".." / "protocol.dll"))
    return _AccessorWrapper(lib, ffi)


class _AccessorWrapper:
    def __init__(self, lib, ffi):
        self.lib = lib
        self.ffi = ffi

    def __getattr__(self, protocol_key: str):
        that = self

        class _Wrapper:
            @cached_property
            def Values(self) -> list[str] | None:
                result = that.lib.ffi_mqttProtocolList(protocol_key.encode())
                if result == FFI.NULL:
                    return None

                i = 0
                strings = []
                while result[i] != FFI.NULL:
                    strings.append(that.ffi.string(result[i]).decode())
                    i += 1

                return strings

            @cache
            def __getattr__(self, key: str) -> str | None:
                result = that.lib.ffi_mqttProtocol(protocol_key.encode(), key.encode())
                if result == FFI.NULL:
                    return None

                return that.ffi.string(result).decode()

        return _Wrapper()


Protocol = _build_protocol_accessor()
