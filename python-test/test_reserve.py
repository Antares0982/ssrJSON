import json

import pytest
import ssrjson


@pytest.mark.parametrize("count", [0, 1, *range(80, 100), 180, 190, 1000])
@pytest.mark.parametrize("indent", [None, 2, 4])
def test_reserve_growth(count, indent):
    items = []
    for char in ("", "\u00e9", "\u4e2d", "\U0001f600"):
        items.append(char)
        items.extend([True, None, False] * count)
    for value in (items, {str(key): item for key, item in enumerate(items)}):
        expected = json.dumps(
            value,
            ensure_ascii=False,
            indent=indent,
            separators=(",", ":") if indent is None else (",", ": "),
        )
        assert ssrjson.dumps(value, indent=indent) == expected
        assert ssrjson.dumps_to_bytes(value, indent=indent) == expected.encode()
