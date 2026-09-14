import json

import ssrjson


def test_tail_copy():
    for char in ("a", "\u00ff", "\u597d", "\U0001f408"):
        for length in range(1, 98):
            for position in range(length):
                text = char * position + "\n" + char * (length - position - 1)
                value = {text: [text, "\n" * length, char * length]}
                encoded = json.dumps(value, ensure_ascii=False)
                assert ssrjson.loads(encoded) == value
                assert json.loads(ssrjson.dumps(value)) == value
                assert json.loads(ssrjson.dumps_to_bytes(value)) == value
