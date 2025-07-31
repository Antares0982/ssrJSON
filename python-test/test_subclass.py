# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import collections
import json

import pytest
import ssrjson


class SubStr(str):
    pass


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
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps(SubStr("abc"))
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps_to_bytes(SubStr("abc"))
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps({SubStr("abc"): SubStr("abcd")})
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps([SubStr("abc")])

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
