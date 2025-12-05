# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import inspect
import json

import pytest

import ssrjson

SIMPLE_TYPES = (1, 1.0, -1, None, "str", True, False)

LOADS_RECURSION_LIMIT = 1024


class TestApi:
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
