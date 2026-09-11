import json
import sys
import threading
import time

import pytest
import ssrjson


class TestArrayHook:
    def _test_all(self, text, hook, expected):
        assert ssrjson.loads(text, array_hook=hook) == expected
        assert ssrjson.loads(text.encode(), array_hook=hook) == expected
        assert ssrjson.loads(bytearray(text.encode()), array_hook=hook) == expected

    def test_none(self):
        self._test_all("[1, 2]", None, [1, 2])

    def test_invalid(self):
        with pytest.raises(TypeError, match="array_hook must be callable"):
            ssrjson.loads("[1]", array_hook=1)

    def test_simple(self):
        self._test_all("[]", tuple, ())
        self._test_all("[1, [2, []]]", tuple, (1, (2, ())))
        self._test_all(
            '["a", "\u00e9", "\u4e2d", "\U0001f600"]',
            tuple,
            ("a", "\u00e9", "\u4e2d", "\U0001f600"),
        )
        self._test_all("[\n  1,\n  [\n    2\n  ]\n]", tuple, (1, (2,)))

    def test_order(self):
        events = []

        def array_hook(values):
            events.append(("array", values))
            return tuple(values)

        def object_hook(value):
            events.append(("object", value))
            return frozenset(value.items())

        result = ssrjson.loads(
            '{"a": [{"b": [1]}]}', object_hook=object_hook, array_hook=array_hook
        )
        assert result == frozenset({("a", (frozenset({("b", (1,))}),))})
        assert events == [
            ("array", [1]),
            ("object", {"b": (1,)}),
            ("array", [frozenset({("b", (1,))})]),
            ("object", {"a": (frozenset({("b", (1,))}),)}),
        ]

    def test_stdlib(self):
        if sys.version_info < (3, 15):
            pytest.skip("array_hook requires Python 3.15")
        text = '[1, [], {"a": [2]}]'
        hook = lambda values: ("array", *values)
        assert ssrjson.loads(text, array_hook=hook) == json.loads(text, array_hook=hook)

    def test_error(self):
        def hook(values):
            raise ValueError("hook error")

        with pytest.raises(ValueError, match="hook error"):
            ssrjson.loads("[1]", array_hook=hook)

    def test_reentrant(self):
        def hook(values):
            return ssrjson.loads(values[0], array_hook=tuple)

        assert ssrjson.loads('["[1, 2]"]', array_hook=hook) == (1, 2)

    def test_recursive(self):
        def hook(values):
            return ssrjson.loads(ssrjson.dumps(values), array_hook=hook)

        with pytest.raises(RecursionError):
            ssrjson.loads("[1]", array_hook=hook)

    def test_multithread(self):
        threads = []
        results = []

        def worker():
            results.append(ssrjson.loads("[3]", array_hook=tuple))

        def hook(values):
            thread = threading.Thread(target=worker)
            threads.append(thread)
            thread.start()
            time.sleep(0)
            return tuple(values)

        assert ssrjson.loads("[1, [2]]", array_hook=hook) == (1, (2,))
        for thread in threads:
            thread.join()
        assert results == [(3,)] * len(threads)
