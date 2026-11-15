import ssrjson

_SENTINEL = object()


class _SubStr(str):
    pass


def _hook(x):
    # Decode string values that look like JSON again, with the same hooks:
    # nested loads() calls from inside a hook.
    if isinstance(x, dict):
        for k, v in x.items():
            if isinstance(v, str) and v[:1] in ("[", "{"):
                try:
                    x[k] = ssrjson.loads(v, object_hook=_hook, array_hook=_hook)
                except Exception:
                    pass
    return x


def _loads_variants(inp):
    try:
        ssrjson.loads(inp, object_hook=_hook, array_hook=_hook)
    except Exception:
        pass


def fuzz_bytes_input(input_bytes: bytes):
    try:
        decoded = ssrjson.loads(input_bytes)
    except Exception:
        decoded = _SENTINEL
    if decoded is not _SENTINEL:
        ssrjson.dumps(decoded)
        ssrjson.dumps_to_bytes(decoded)
    _loads_variants(input_bytes)
    _loads_variants(bytearray(input_bytes))
    try:
        input_str = input_bytes.decode("utf-8")
    except Exception:
        return
    try:
        decoded = ssrjson.loads(input_str)
    except Exception:
        decoded = _SENTINEL
    if decoded is not _SENTINEL:
        ssrjson.dumps(decoded)
        ssrjson.dumps_to_bytes(decoded)
    _loads_variants(input_str)
    try:
        ssrjson.loads(_SubStr(input_str))
    except Exception:
        pass
