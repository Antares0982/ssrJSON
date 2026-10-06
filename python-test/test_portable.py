"""Boundaries shared by scalar and SIMD backends."""

import json
import math
import struct

import pytest
import ssrjson


@pytest.mark.parametrize("length", [0, 1, 7, 8, 15, 16, 17, 31, 32, 63, 64, 65])
@pytest.mark.parametrize(
    "char", ["a", "\u00e9", "\u4e2d", "\U0001f600", "\\", '"', "\x00"]
)
def test_string_boundaries(length, char):
    value = {"key": ["a" * length + char, char * length, 2**64 - 1]}
    expected = json.dumps(value, ensure_ascii=False, separators=(",", ":"))
    assert ssrjson.dumps(value) == expected
    for cache in (False, True):
        encoded = ssrjson.dumps_to_bytes(value, is_write_cache=cache)
        assert encoded == expected.encode()
        assert ssrjson.loads(encoded) == value
    assert ssrjson.loads(expected) == value


def test_float_boundaries():
    for bits in (
        1,
        0x000FFFFFFFFFFFFF,
        0x0010000000000000,
        0x3FF0000000000001,
        0x7FEFFFFFFFFFFFFF,
    ):
        value = struct.unpack("d", struct.pack("Q", bits))[0]
        for number in (value, -value):
            encoded = ssrjson.dumps(number)
            assert math.isfinite(float(encoded))
            assert float(encoded) == number
            assert ssrjson.loads(encoded) == number
            assert ssrjson.dumps_to_bytes(number).decode() == encoded
