import json

import ssrjson


def test_short_clean_tail():
    for length in range(66):
        for escape in ("", '"', "\\", "\n", "\x00"):
            for position in range(length + 1):
                text = "a" * position + escape + "b" * (length - position)
                value = {text: [text, text * 3]}
                for indent in (None, 2, 4):
                    expected = json.dumps(value, ensure_ascii=False, indent=indent)
                    assert json.loads(
                        ssrjson.dumps(value, indent=indent)
                    ) == json.loads(expected)
                    assert json.loads(
                        ssrjson.dumps_to_bytes(value, indent=indent)
                    ) == json.loads(expected)


def test_encode_tail():
    for char in ("a", "\u00ff", "\u597d", "\U0001f408", "\ud800"):
        for length in range(1, 66):
            for escape in ('"', "\\", "\n", "\x00", "\x1f"):
                for position in range(length):
                    text = char * position + escape + char * (length - position - 1)
                    for prefix in ("", "\u597d", "\U0001f408"):
                        value = [prefix, text, escape * length]
                        assert json.loads(ssrjson.dumps(value)) == value
                        assert json.loads(ssrjson.dumps({text: value})) == {text: value}


def test_encode_loop():
    for char in ("a", "\u00ff", "\u597d", "\U0001f408"):
        for length in (32, 64, 128, 257):
            for escape in ('"', "\\", "\n", "\x00", "\x1f"):
                for position in range(length):
                    text = char * position + escape + char * (length - position - 1)
                    value = {text: [text, escape * length]}
                    assert json.loads(ssrjson.dumps(value)) == value
                    assert json.loads(ssrjson.dumps_to_bytes(value)) == value
