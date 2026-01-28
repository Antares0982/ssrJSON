import pytest

import ssrjson
import json


class TestObjectHook:
    def _test_both(self, s, obj):
        exp = json.loads(s, object_hook=obj)
        assert ssrjson.loads(s, object_hook=obj) == exp
        assert ssrjson.loads(s.encode(), object_hook=obj) == exp

    def test_nothing(self):
        self._test_both('{"a": 1}', lambda d: d)

    def test_invalid_hook(self):
        with pytest.raises(TypeError):
            # not callable
            ssrjson.loads('{"a": 1}', object_hook=1)

    def test_simple_hook(self):
        def hook(d):
            return {"hooked": d}

        self._test_both('{"a": 1}', hook)
        self._test_both('{"a": {"b": 2}}', hook)
        self._test_both('{"a": {"b": {"c": 3}}}', hook)

    def test_nested_hook(self):
        def hook(d):
            return {
                "hooked": ssrjson.loads(d["text"]),
                "origin": ssrjson.dumps(d),
                "hooked2": ssrjson.loads(d["text"]),
            }

        self._test_both('{"text": "{\\"test\\": [1,2,3]}"}', hook)

    def test_nested2_hook(self):
        def hook(h):
            return lambda d: {
                "hooked": ssrjson.loads(d["text"], object_hook=h),
                "origin": ssrjson.dumps(d),
            }

        self._test_both(
            '{"text": "{\\"text\\": \\"{\\\\\\"text\\\\\\": 2}\\"}"}',
            hook(hook(None)),
        )

    def test_error_in_hook(self):
        def hook(d):
            raise ValueError("hook error")

        with pytest.raises(ValueError, match="hook error"):
            ssrjson.loads('{"a": 1}', object_hook=hook)

    def test_error_in_hook2(self):
        def hook2(d):
            raise ValueError("hook error")

        def hook(d):
            return {"hooked": ssrjson.loads(d["text"], object_hook=hook2)}

        with pytest.raises(ValueError, match="hook error"):
            ssrjson.loads('{"text": "{\\"a\\": 1}"}', object_hook=hook)

    def test_recursive(self):
        def hook(d):
            return ssrjson.loads(ssrjson.dumps(d), object_hook=hook)

        with pytest.raises(RecursionError):
            ssrjson.loads('{"a": 1}', object_hook=hook)

    def test_multithread(self):
        import time
        import threading

        def hook(d):
            threading.Thread(target=func).start()
            time.sleep(0)
            return d

        def hook2(d):
            time.sleep(1)
            return d

        def func():
            assert ssrjson.loads('{"c": 3}', object_hook=hook2) == {"c": 3}

        assert ssrjson.loads('{"a": {"b": 2}}', object_hook=hook) == {"a": {"b": 2}}
