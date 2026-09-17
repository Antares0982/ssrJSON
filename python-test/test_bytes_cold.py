import json

import numpy as np
import pytest
import ssrjson


class SubStr(str):
    pass


def test_cold_values():
    ssrjson.setup_numpy_types(np)
    text = SubStr("\u597d" * 32768)
    array = np.arange(4096, dtype=np.float64)
    value = [text, 1.25, {"values": (array, text, 2.5)}, array, 3.75]
    expected = [
        text,
        1.25,
        {"values": [array.tolist(), text, 2.5]},
        array.tolist(),
        3.75,
    ]
    for indent in (None, 2, 4):
        for cache in (False, True):
            for bad in (SubStr("\ud800"), np.array([1j])):
                for container in ([bad, 1.25], {"value": bad}, (bad, 1.25)):
                    with pytest.raises(ssrjson.JSONEncodeError):
                        ssrjson.dumps_to_bytes(
                            container, indent=indent, is_write_cache=cache
                        )
                    assert (
                        json.loads(
                            ssrjson.dumps_to_bytes(
                                value, indent=indent, is_write_cache=cache
                            )
                        )
                        == expected
                    )
