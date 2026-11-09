# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import inspect
import json

import pytest

import ssrjson

SIMPLE_TYPES = (1, 1.0, -1, None, "str", True, False)

LOADS_RECURSION_LIMIT = 1024


class TestApi:
    def test_current_features(self):
        features = ssrjson.get_current_features()
        for key in (
            "multi_lib",
            "free_threading",
            "lockfree",
            "pgo",
            "write_utf8_cache",
            "strict_arg_parse",
        ):
            assert isinstance(features[key], bool)
        assert features["simd"] in {
            "SCALAR",
            "SSE2",
            "SSE4.2",
            "AVX2",
            "AVX512",
            "NEON",
        }

    def test_loads_trailing(self):
        """
        loads() handles trailing whitespace
        """
        assert ssrjson.loads("{}\n\t ") == {}

    def test_loads_trailing_invalid(self):
        """
        loads() handles trailing invalid
        """
        pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, "{}\n\t a")

    def test_simple_json(self):
        """
        dumps() equivalent to json on simple types
        """
        for obj in SIMPLE_TYPES:
            assert ssrjson.dumps(obj) == json.dumps(obj)
            assert ssrjson.dumps_to_bytes(obj) == json.dumps(obj).encode("utf-8")

    def test_simple_round_trip(self):
        """
        dumps(), loads() round trip on simple types
        """
        for obj in SIMPLE_TYPES:
            assert ssrjson.loads(ssrjson.dumps(obj)) == obj
            assert ssrjson.loads(ssrjson.dumps_to_bytes(obj)) == obj

    def test_loads_type(self):
        """
        loads() invalid type
        """
        for val in (1, 3.14, [], {}, None):
            # pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, val)
            pytest.raises(TypeError, ssrjson.loads, val)

    def test_loads_recursion_partial(self):
        """
        loads() recursion limit partial
        """
        pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, "[" * (1024 * 1024))

    def test_loads_recursion_valid_limit_array(self):
        """
        loads() recursion limit at limit array
        """
        n = LOADS_RECURSION_LIMIT + 1
        value = b"[" * n + b"]" * n
        pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, value)

    def test_loads_recursion_valid_limit_object(self):
        """
        loads() recursion limit at limit object
        """
        n = LOADS_RECURSION_LIMIT
        value = b'{"key":' * n + b'{"key":true}' + b"}" * n
        pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, value)

    def test_loads_recursion_valid_limit_mixed(self):
        """
        loads() recursion limit at limit mixed
        """
        n = LOADS_RECURSION_LIMIT
        value = b'[{"key":' * n + b'{"key":true}' + b"}" * n + b"]"
        pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, value)

    def test_loads_recursion_valid_excessive_array(self):
        """
        loads() recursion limit excessively high value
        """
        n = 10000000
        value = b"[" * n + b"]" * n
        pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, value)

    def test_loads_recursion_valid_limit_array_pretty(self):
        """
        loads() recursion limit at limit array pretty
        """
        n = LOADS_RECURSION_LIMIT + 1
        value = b"[\n  " * n + b"]" * n
        pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, value)

    def test_loads_recursion_valid_limit_object_pretty(self):
        """
        loads() recursion limit at limit object pretty
        """
        n = LOADS_RECURSION_LIMIT
        value = b'{\n  "key":' * n + b'{"key":true}' + b"}" * n
        pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, value)

    def test_loads_recursion_valid_limit_mixed_pretty(self):
        """
        loads() recursion limit at limit mixed pretty
        """
        n = LOADS_RECURSION_LIMIT
        value = b'[\n  {"key":' * n + b'{"key":true}' + b"}" * n + b"]"
        pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, value)

    def test_loads_recursion_valid_excessive_array_pretty(self):
        """
        loads() recursion limit excessively high value pretty
        """
        n = 10000000
        value = b"[\n  " * n + b"]" * n
        pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, value)

    def test_value_error(self):
        """
        ssrjson.JSONDecodeError is a subclass of ValueError
        """
        pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, "{")
        pytest.raises(ValueError, ssrjson.loads, "{")

    def test_all_keywords(self):
        """
        all keywords
        """
        assert ssrjson.dumps(obj={}) == "{}"
        assert ssrjson.dumps_to_bytes(obj={}) == b"{}"
        assert ssrjson.dumps_to_bytes(obj={}, is_write_cache=True) == b"{}"
        assert ssrjson.dumps_to_bytes(obj={}, is_write_cache=False) == b"{}"
        assert ssrjson.dumps(indent=2, obj={}) == "{}"
        assert ssrjson.dumps_to_bytes(indent=2, obj={}) == b"{}"
        assert ssrjson.dumps_to_bytes(indent=2, obj={}, is_write_cache=True) == b"{}"
        assert ssrjson.dumps_to_bytes(indent=2, obj={}, is_write_cache=False) == b"{}"
        assert ssrjson.loads(s="{}") == {}

    def test_redundant_keywords(self):
        """
        redundant keywords
        """
        with pytest.raises(TypeError):
            ssrjson.dumps({}, obj={})  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes({}, obj={})  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.loads("{}", s="{}")  # type: ignore

    def test_missing_required_positional(self):
        """
        missing required arg
        """
        with pytest.raises(TypeError):
            ssrjson.dumps()  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes()  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.loads()  # type: ignore

    def test_more_than_required_positional(self):
        """
        too many positional args
        """
        with pytest.raises(TypeError):
            ssrjson.dumps({}, 2)  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.dumps({}, 2, a=1)  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes({}, 2)  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes({}, 2, a=1)  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.loads("{}", "extra")  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.loads("{}", "extra", a=1)  # type: ignore

    def test_default_positional(self):
        """
        dumps() positional arg
        """
        ssrjson.suppress_api_warning()
        with pytest.raises(TypeError):
            ssrjson.dumps(__obj={})  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.dumps(zxc={})  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes(__obj={})  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes(zxc={})  # type: ignore

    def test_unknown_kwarg(self):
        """
        unknown kwarg
        """
        ssrjson.suppress_api_warning()
        #
        ssrjson.strict_argparse(True)
        with pytest.raises(TypeError):
            ssrjson.dumps({}, zxc=1)  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes({}, zxc=1)  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.loads("{}", zxc=1)  # type: ignore
        #
        ssrjson.strict_argparse(False)
        ssrjson.dumps({}, zxc=1)
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes({}, zxc=1)  # type: ignore
        #
        assert ssrjson.dumps({}, skipkeys="a") == "{}"

    def test_nonascii_unknown_arg(self):
        """
        dumps() unknown arg with non-ascii chars
        """
        ssrjson.suppress_api_warning()
        #
        ssrjson.strict_argparse(True)
        with pytest.raises(TypeError):
            ssrjson.dumps({}, неизвестный_аргумент=1)  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes({}, 未知参数=1)  # type: ignore
        #
        ssrjson.strict_argparse(False)
        ssrjson.dumps({}, 不明なパラメータ=1)
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes({}, unknöwn_args=1)  # type: ignore

    def test_strsubclass_unknown_arg(self):
        """
        dumps() arg with subclass of str
        """

        class MyStr(str):
            pass

        kw = {MyStr("indent"): 2}
        assert ssrjson.dumps({"a": "b"}, **kw) == '{\n  "a": "b"\n}'
        assert ssrjson.dumps_to_bytes({"a": "b"}, **kw) == b'{\n  "a": "b"\n}'
        #
        kw = {MyStr("unknown_arg"): "value"}
        #
        ssrjson.strict_argparse(True)
        with pytest.raises(TypeError):
            ssrjson.dumps({}, **kw)  # type: ignore
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes({}, **kw)  # type: ignore
        #
        ssrjson.strict_argparse(False)
        ssrjson.dumps({}, **kw)
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes({}, **kw)  # type: ignore

    def test_default_empty_kwarg(self):
        """
        dumps() empty kwarg
        """
        assert ssrjson.dumps(None, **{}) == "null"
        assert ssrjson.dumps_to_bytes(None, **{}) == b"null"

    def test_dumps_signature(self):
        """
        dumps() valid __text_signature__
        """
        assert str(inspect.signature(ssrjson.dumps)) == "(obj, indent=None)"
        assert (
            str(inspect.signature(ssrjson.dumps_to_bytes))
            == "(obj, indent=None, is_write_cache=None)"
        )

    def test_loads_signature(self):
        """
        loads() valid __text_signature__
        """
        assert str(inspect.signature(ssrjson.loads)) == "(s)"

    def test_dumps_module_str(self):
        """
        ssrjson.dumps.__module__ is a str
        """
        assert "ssrjson" in ssrjson.dumps.__module__
        assert "ssrjson" in ssrjson.dumps_to_bytes.__module__

    def test_loads_module_str(self):
        """
        ssrjson.loads.__module__ is a str
        """
        assert "ssrjson" in ssrjson.loads.__module__

    def test_bytes_buffer(self):
        """
        dumps() trigger buffer growing where length is greater than growth
        """
        a = "a" * 900
        b = "b" * 4096
        c = "c" * 4096 * 4096
        assert ssrjson.dumps([a, b, c]) == f'["{a}","{b}","{c}"]'
        assert ssrjson.dumps_to_bytes([a, b, c]) == f'["{a}","{b}","{c}"]'.encode(
            "utf-8"
        )

    def test_bytes_null_terminated(self):
        """
        dumps() PyBytesObject buffer is null-terminated
        """
        # would raise ValueError: invalid literal for int() with base 10: b'1596728892'
        int(ssrjson.dumps(1596728892))
        int(ssrjson.dumps_to_bytes(1596728892))


_MODULE_LIFETIME_SCRIPT = r"""
import gc, sys
import ssrjson

def check(loads, dumps, E):
    try:
        loads("[")
    except E:
        pass
    else:
        raise AssertionError("no JSONDecodeError")
    assert loads('[{"a": 1}]', object_hook=lambda d: d) == [{"a": 1}]
    assert dumps({"a": [1]}) == '{"a":[1]}'

# 1. Module object dropped while a function reference is still alive.
loads, dumps, E = ssrjson.loads, ssrjson.dumps, ssrjson.JSONDecodeError
del sys.modules["ssrjson"], ssrjson
gc.collect()
check(loads, dumps, E)

# 2. Second module object from a re-import; the first one is freed.
import ssrjson as first
del sys.modules["ssrjson"]
import ssrjson as second
assert first.JSONDecodeError is second.JSONDecodeError
del first, loads, dumps
gc.collect()
check(second.loads, second.dumps, second.JSONDecodeError)
print("ok")
"""


def test_module_lifetime():
    import os
    import subprocess
    import sys

    r = subprocess.run(
        [sys.executable, "-c", _MODULE_LIFETIME_SCRIPT],
        capture_output=True,
        text=True,
        env=os.environ.copy(),
        timeout=120,
    )
    assert r.returncode == 0, r.stderr[-2000:]
    assert r.stdout.strip() == "ok"


_SUBINTERPRETER_SCRIPT = r"""
import gc
import _xxsubinterpreters as interpreters
import ssrjson

SUB = '''
import ssrjson
assert ssrjson.loads('[1, {"a": [true]}]') == [1, {"a": [True]}]
'''
iid = interpreters.create()
interpreters.run_string(iid, SUB)
for _ in range(3):
    other = interpreters.create()
    interpreters.run_string(other, SUB)
    interpreters.destroy(other)
interpreters.run_string(iid, SUB)
interpreters.destroy(iid)
gc.collect()
try:
    ssrjson.loads("[")
except ssrjson.JSONDecodeError:
    pass
else:
    raise AssertionError("no JSONDecodeError")
assert ssrjson.loads('{"a": [1]}', object_hook=lambda d: d) == {"a": [1]}
print("ok")
"""


def test_subinterpreter_lifetime():
    # Before 3.12 there is no Py_mod_multiple_interpreters slot, so subinterpreters share ssrjson's
    # process-wide state; destroying one must not tear it down for the others.
    import os
    import subprocess
    import sys

    if sys.version_info >= (3, 12):
        pytest.skip("import in subinterpreters is refused on 3.12+")
    r = subprocess.run(
        [sys.executable, "-c", _SUBINTERPRETER_SCRIPT],
        capture_output=True,
        text=True,
        env=os.environ.copy(),
        timeout=120,
    )
    assert r.returncode == 0, r.stderr[-2000:]
    assert r.stdout.strip() == "ok"


_UTF8_CACHE_SCRIPT = r"""
import ssrjson
s = "é" * 100
k = "ключ" * 5
assert ssrjson.dumps_to_bytes(s, is_write_cache=True) == ('"' + s + '"').encode()
assert ssrjson.dumps_to_bytes({k: s + "好"}, is_write_cache=True)
del s, k
print("ok")
"""


def test_utf8_cache_allocator_domain():
    # The debug allocator aborts if the cache is freed through a different API than it was allocated with.
    import os
    import subprocess
    import sys

    env = os.environ.copy()
    if getattr(sys, "_is_gil_enabled", lambda: True)():
        env["PYTHONMALLOC"] = "malloc_debug"
    else:
        # free-threaded builds only accept mimalloc; -X dev still installs the debug hooks
        env.pop("PYTHONMALLOC", None)
    r = subprocess.run(
        [sys.executable, "-X", "dev", "-c", _UTF8_CACHE_SCRIPT],
        capture_output=True,
        text=True,
        env=env,
        timeout=120,
    )
    assert r.returncode == 0, r.stderr[-2000:]
    assert r.stdout.strip() == "ok"


def test_dumps_to_bytes_hash():
    for obj in ([1], {"a": 1}, "x" * 100, {"k": "é" * 50}, list(range(1000))):
        b = ssrjson.dumps_to_bytes(obj)
        fresh = bytes(bytearray(b))
        assert hash(b) == hash(fresh)
        assert fresh in {b}
