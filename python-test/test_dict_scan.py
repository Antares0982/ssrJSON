import json

import pytest
import ssrjson


def test_dict_scan():
    class Record:
        pass

    class Mapping(dict):
        pass

    class Key(str):
        pass

    first, second = Record(), Record()
    first.a, first.b, first.c = 1, 2, 3
    second.c, second.a = 4, 5
    del first.b
    first.b = 6
    values = [first.__dict__, second.__dict__, {}, Mapping(a=1, b=2), {Key("a"): 1}]
    for size in (1, 8, 32, 256, 1024, 22000):
        original = {str(i): i for i in range(size)}
        for removed in (range(0, size, 2), range(size // 2, size), range(size)):
            value = original.copy()
            for i in removed:
                del value[str(i)]
            values.append(value)
            value = value.copy()
            for i in reversed(removed):
                value[str(i)] = i
            values.append(value)
        general = original.copy()
        general[0] = 0
        del general[0]
        values.append(general)
    for value in values:
        for prefix in ("", "\u00ff", "\u597d", "\U0001f408"):
            data = [prefix, value, {"nested": [value, {prefix: value}]}]
            for indent in (None, 2, 4):
                expected = json.dumps(
                    data,
                    ensure_ascii=False,
                    indent=indent,
                    separators=(",", ":") if indent is None else None,
                )
                assert ssrjson.dumps(data, indent=indent) == expected
                assert ssrjson.dumps_to_bytes(data, indent=indent) == expected.encode()
    for encode in (ssrjson.dumps, ssrjson.dumps_to_bytes):
        with pytest.raises(ssrjson.JSONEncodeError):
            encode({1: "invalid"})
