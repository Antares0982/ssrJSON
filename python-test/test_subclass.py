# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import collections
import json

import pytest
import ssrjson


class SubStr(str):
    pass


class SubStr2(str):
    pass


class SubStrCustomNew(str):
    def __new__(cls, val):
        return super().__new__(cls, val)


class SubInt(int):
    pass


class SubDict(dict):
    pass


class SubList(list):
    pass


class SubFloat(float):
    pass


class SubTuple(tuple):
    pass


class TestSubclass:
    def test_subclass_str(self):
        # -- Basic SubStr encoding (dumps path) --
        assert ssrjson.dumps(SubStr("a")) == '"a"'
        assert ssrjson.dumps(SubStr("")) == '""'

        # Unicode ranges (dumps path)
        for s in ("a", "\u00ff", "\u597d", "\U0001f408"):
            ref = json.dumps(s, ensure_ascii=False)
            assert ssrjson.dumps(SubStr(s)) == ref

        # Escape characters (dumps path)
        for s in ('"', "\\", "\n", "\r", "\t", "\b", "\f", 'a"b'):
            ref = json.dumps(s, ensure_ascii=False)
            assert ssrjson.dumps(SubStr(s)) == ref

        # SubStr top-level with indent (dumps path)
        assert ssrjson.dumps(SubStr("abc"), indent=2) == '"abc"'
        assert ssrjson.dumps(SubStr("test"), indent=4) == '"test"'

        # SubStr in containers (dumps path)
        d = {SubStr("key"): SubStr("value")}
        assert ssrjson.dumps(d) == '{"key":"value"}'

        d = {SubStr("\u597d"): SubStr("\U0001f408")}
        assert ssrjson.dumps(d) == '{"\u597d":"\U0001f408"}'

        lst = [SubStr("a"), SubStr("\u00ff"), SubStr("\u597d"), SubStr("\U0001f408")]
        assert json.loads(ssrjson.dumps(lst)) == ["a", "\u00ff", "\u597d", "\U0001f408"]

        # SubStr in containers with indent (dumps path)
        assert ssrjson.dumps({SubStr("k"): SubStr("v")}, indent=2) == '{\n  "k": "v"\n}'
        assert (
            ssrjson.dumps({SubStr("k"): SubStr("v")}, indent=4) == '{\n    "k": "v"\n}'
        )

        # Mixed str and SubStr in same structure (dumps path)
        obj = {"key": SubStr("value")}
        assert ssrjson.dumps(obj) == '{"key":"value"}'
        obj = {SubStr("mixed_key"): "plain_value"}
        assert ssrjson.dumps(obj) == '{"mixed_key":"plain_value"}'

        # -- SubStr in containers (dumps_to_bytes path) --
        assert (
            ssrjson.dumps_to_bytes({SubStr("k"): SubStr("v")}, indent=2)
            == b'{\n  "k": "v"\n}'
        )
        assert (
            ssrjson.dumps_to_bytes({SubStr("key"): SubStr("value")})
            == b'{"key":"value"}'
        )

        # -- Long strings (dumps path) --
        long_s = "a" * 4096
        ref = json.dumps(long_s, ensure_ascii=False)
        assert ssrjson.dumps(SubStr(long_s)) == ref

        long_s = "a" * 4096 + "\u00ff" + "a" * 4096
        ref = json.dumps(long_s, ensure_ascii=False)
        assert ssrjson.dumps(SubStr(long_s)) == ref

        long_s = "a" * 4096 + "\u597d" + "a" * 4096
        ref = json.dumps(long_s, ensure_ascii=False)
        assert ssrjson.dumps(SubStr(long_s)) == ref

        long_s = "a" * 4096 + "\U0001f408" + "a" * 4096
        ref = json.dumps(long_s, ensure_ascii=False)
        assert ssrjson.dumps(SubStr(long_s)) == ref

        # Long repeated single non-ASCII char (dumps path)
        for c in ("\u00ff", "\u597d", "\U0001f408"):
            s = c * 4096
            ref = json.dumps(s, ensure_ascii=False)
            assert ssrjson.dumps(SubStr(s)) == ref

        # -- Multiple str subclass types (dumps path) --
        assert ssrjson.dumps(SubStr2("abc")) == '"abc"'
        assert ssrjson.dumps(SubStrCustomNew("ghi")) == '"ghi"'

        # -- Nested structures with SubStr --
        obj = {SubStr("k1"): [SubStr("v1"), {SubStr("k2"): SubStr("v2")}]}
        assert json.loads(ssrjson.dumps(obj)) == {"k1": ["v1", {"k2": "v2"}]}
        assert json.loads(ssrjson.dumps_to_bytes(obj)) == {"k1": ["v1", {"k2": "v2"}]}

        # -- Large nested structure (dumps path) --
        large_obj = {
            SubStr("k%d" % i): [SubStr("v%d_%d" % (i, j)) for j in range(10)]
            for i in range(100)
        }
        assert json.loads(ssrjson.dumps(large_obj)) == json.loads(json.dumps(large_obj))
        assert json.loads(ssrjson.dumps_to_bytes(large_obj)) == json.loads(
            json.dumps(large_obj)
        )

    def test_subclass_str_top_level_bytes(self):
        assert ssrjson.dumps_to_bytes(SubStr("a")) == b'"a"'
        assert ssrjson.dumps_to_bytes(SubStr("")) == b'""'

        for s in ("a", "\u00ff", "\u597d", "\U0001f408"):
            ref = json.dumps(s, ensure_ascii=False)
            assert ssrjson.dumps_to_bytes(SubStr(s)) == ref.encode("utf-8")

        for s in ('"', "\\", "\n", "\r", "\t", "\b", "\f"):
            ref = json.dumps(s, ensure_ascii=False)
            assert ssrjson.dumps_to_bytes(SubStr(s)) == ref.encode("utf-8")

        assert ssrjson.dumps_to_bytes(SubStr("abc"), indent=2) == b'"abc"'
        assert ssrjson.dumps_to_bytes(SubStr("test"), indent=4) == b'"test"'

    def test_subclass_str_top_level_long_bytes(self):
        long_s = "a" * 4096
        ref = json.dumps(long_s, ensure_ascii=False)
        assert ssrjson.dumps_to_bytes(SubStr(long_s)) == ref.encode("utf-8")

        long_s = "a" * 4096 + "\u00ff" + "a" * 4096
        ref = json.dumps(long_s, ensure_ascii=False)
        assert ssrjson.dumps_to_bytes(SubStr(long_s)) == ref.encode("utf-8")

        long_s = "a" * 4096 + "\u597d" + "a" * 4096
        ref = json.dumps(long_s, ensure_ascii=False)
        assert ssrjson.dumps_to_bytes(SubStr(long_s)) == ref.encode("utf-8")

        long_s = "a" * 4096 + "\U0001f408" + "a" * 4096
        ref = json.dumps(long_s, ensure_ascii=False)
        assert ssrjson.dumps_to_bytes(SubStr(long_s)) == ref.encode("utf-8")

        for c in ("\u00ff", "\u597d", "\U0001f408"):
            s = c * 4096
            ref = json.dumps(s, ensure_ascii=False)
            assert ssrjson.dumps_to_bytes(SubStr(s)) == ref.encode("utf-8")

    def test_subclass_str_multi_types_bytes(self):
        assert ssrjson.dumps_to_bytes(SubStr2("def")) == b'"def"'
        assert ssrjson.dumps_to_bytes(SubStrCustomNew("jkl")) == b'"jkl"'

    def test_subclass_str_containers_bytes_nonascii(self):
        # SubStr with non-ASCII in containers via dumps_to_bytes

        def _check(is_write_cache):
            d = {SubStr("\u597d"): SubStr("\U0001f408")}
            assert ssrjson.dumps_to_bytes(
                d, is_write_cache=is_write_cache
            ) == '{"\u597d":"\U0001f408"}'.encode("utf-8")

            lst = [SubStr("\u00ff"), SubStr("\u597d"), SubStr("\U0001f408")]
            assert json.loads(
                ssrjson.dumps_to_bytes(lst, is_write_cache=is_write_cache)
            ) == [
                "\u00ff",
                "\u597d",
                "\U0001f408",
            ]

            d = {SubStr("\u597d"): SubStr("\U0001f408")}
            assert ssrjson.dumps_to_bytes(
                d, indent=2, is_write_cache=is_write_cache
            ) == '{\n  "\u597d": "\U0001f408"\n}'.encode("utf-8")

        _check(False)
        _check(True)

    def test_subclass_str_containers_bytes_escapes(self):
        # SubStr values with escape characters via dumps_to_bytes

        def _check(is_write_cache):
            for c in ("\n", "\r", "\t", "\b", "\f", '"', "\\"):
                d = {SubStr("k"): SubStr(c)}
                ref = json.dumps({"k": c}, ensure_ascii=False).replace(" ", "")
                assert ssrjson.dumps_to_bytes(
                    d, is_write_cache=is_write_cache
                ) == ref.encode("utf-8")

            d = {SubStr("k"): SubStr('a\nb"c\\d')}
            ref = json.dumps({"k": 'a\nb"c\\d'}, ensure_ascii=False).replace(" ", "")
            b = ssrjson.dumps_to_bytes(d, is_write_cache=is_write_cache)
            assert b == ref.encode("utf-8")

            # With indent
            d = {SubStr("k"): SubStr("a\nb")}
            ref = json.dumps({"k": "a\nb"}, indent=2, ensure_ascii=False)
            assert ssrjson.dumps_to_bytes(
                d, indent=2, is_write_cache=is_write_cache
            ) == ref.encode("utf-8")

            # Non-ASCII with \uXXXX-escaped control chars (len >= 6 so cache matters)
            for s in (
                "\u00d2\u0003\u00ee\u0004",
                "\u00d2\u00b6{=\u0003\u00ee\u0004\u00b4\u00d5",
                "\u00d2\u00d2\u00d2\u0003\u00ee\u0004",
            ):
                d = {SubStr("k"): SubStr(s)}
                ref = json.dumps({"k": s}, ensure_ascii=False).replace(" ", "")
                assert ssrjson.dumps_to_bytes(
                    d, is_write_cache=is_write_cache
                ) == ref.encode("utf-8")

            return b

        a = _check(False)
        b = _check(True)
        assert a == b

    def test_subclass_int(self):
        assert ssrjson.dumps(SubInt(1)) == "1"
        assert ssrjson.dumps_to_bytes(SubInt(1)) == b"1"

    def test_subclass_int_64(self):
        for val in (9223372036854775807, -9223372036854775807):
            assert ssrjson.dumps(SubInt(val)) == str(val)
            assert ssrjson.dumps_to_bytes(SubInt(val)) == str(val).encode("utf-8")

    def test_subclass_dict(self):
        assert ssrjson.dumps(SubDict({"a": "b"})) == '{"a":"b"}'
        assert ssrjson.dumps_to_bytes(SubDict({"a": "b"})) == b'{"a":"b"}'

    def test_subclass_list(self):
        assert ssrjson.dumps(SubList(["a", "b"])) == '["a","b"]'
        assert ssrjson.dumps_to_bytes(SubList(["a", "b"])) == b'["a","b"]'
        ref = [True] * 512
        assert ssrjson.loads(ssrjson.dumps(SubList(ref))) == ref
        assert ssrjson.loads(ssrjson.dumps_to_bytes(SubList(ref))) == ref

    def test_nested_containers(self):
        d = collections.defaultdict(SubList)
        d["a"].append("b")
        assert ssrjson.dumps(d) == '{"a":["b"]}'
        assert ssrjson.dumps_to_bytes(d) == b'{"a":["b"]}'
        d = SubList([collections.defaultdict(a="b")])
        assert ssrjson.dumps(d) == '[{"a":"b"}]'
        assert ssrjson.dumps_to_bytes(d) == b'[{"a":"b"}]'

    def test_subclass_float(self):
        assert ssrjson.dumps(SubFloat(1.1)) == "1.1"
        assert ssrjson.dumps_to_bytes(SubFloat(1.1)) == b"1.1"
        assert json.dumps(SubFloat(1.1)) == "1.1"

    def test_subclass_tuple(self):
        assert ssrjson.dumps(SubTuple((1, 2))) == "[1,2]"
        assert ssrjson.dumps_to_bytes(SubTuple((1, 2))) == b"[1,2]"
        assert json.dumps(SubTuple((1, 2))) == "[1, 2]"

    def test_namedtuple(self):
        Point = collections.namedtuple("Point", ["x", "y"])
        assert ssrjson.dumps(Point(1, 2)) == "[1,2]"
        assert ssrjson.dumps_to_bytes(Point(1, 2)) == b"[1,2]"

    def test_subclass_circular_dict(self):
        obj = SubDict({})
        obj["obj"] = obj
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps(obj)

    def test_subclass_circular_list(self):
        obj = SubList([])
        obj.append(obj)
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps(obj)

    def test_subclass_circular_nested(self):
        obj = SubDict({})
        obj["list"] = SubList([{"obj": obj}])
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps(obj)
