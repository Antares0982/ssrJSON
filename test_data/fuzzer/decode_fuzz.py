import ssrjson

_SENTINEL = object()


def fuzz_bytes_input(input_bytes: bytes):
    try:
        decoded = ssrjson.loads(input_bytes)
    except Exception:
        decoded = _SENTINEL
    if decoded is not _SENTINEL:
        ssrjson.dumps(decoded)
        ssrjson.dumps_to_bytes(decoded)
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
