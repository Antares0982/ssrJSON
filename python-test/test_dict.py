# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import pytest

import ssrjson


class TestDict:
    def test_dict(self):
        """
        dict
        """
        obj = {"key": "value"}
        ref = '{"key":"value"}'
        assert ssrjson.dumps(obj) == ref
        assert ssrjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert ssrjson.loads(ref) == obj

    def test_dict_duplicate_loads(self):
        assert ssrjson.loads(b'{"1":true,"1":false}') == {"1": False}

    def test_dict_empty(self):
        obj = [{"key": [{}] * 4096}] * 4096  # type:ignore
        assert ssrjson.loads(ssrjson.dumps(obj)) == obj
        assert ssrjson.loads(ssrjson.dumps_to_bytes(obj)) == obj

    def test_dict_large_dict(self):
        """
        dict with >512 keys
        """
        obj = {"key_%s" % idx: [{}, {"a": [{}, {}, {}]}, {}] for idx in range(513)}  # type: ignore
        assert len(obj) == 513
        assert ssrjson.loads(ssrjson.dumps(obj)) == obj
        assert ssrjson.loads(ssrjson.dumps_to_bytes(obj)) == obj

    def test_dict_large_4096(self):
        """
        dict with >4096 keys
        """
        obj = {"key_%s" % idx: "value_%s" % idx for idx in range(4097)}
        assert len(obj) == 4097
        assert ssrjson.loads(ssrjson.dumps(obj)) == obj
        assert ssrjson.loads(ssrjson.dumps_to_bytes(obj)) == obj

    def test_dict_large_65536(self):
        """
        dict with >65536 keys
        """
        obj = {"key_%s" % idx: "value_%s" % idx for idx in range(65537)}
        assert len(obj) == 65537
        assert ssrjson.loads(ssrjson.dumps(obj)) == obj
        assert ssrjson.loads(ssrjson.dumps_to_bytes(obj)) == obj

    def test_dict_large_keys(self):
        """
        dict with keys too large to cache
        """
        obj = {
            "keeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeey": "value"
        }
        ref = '{"keeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeey":"value"}'
        assert ssrjson.dumps(obj) == ref
        assert ssrjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert ssrjson.loads(ref) == obj

    def test_dict_unicode(self):
        """
        dict unicode keys
        """
        obj = {"🐈": "value"}
        ref = b'{"\xf0\x9f\x90\x88":"value"}'.decode("utf-8")
        assert ssrjson.dumps(obj) == ref
        assert ssrjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert ssrjson.loads(ref) == obj
        assert ssrjson.loads(ref)["🐈"] == "value"

    def test_dict_invalid_key_dumps(self):
        """
        dict invalid key dumps()
        """
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps({1: "value"})
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps({b"key": "value"})
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps_to_bytes({1: "value"})
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps_to_bytes({b"key": "value"})

    def test_dict_invalid_key_loads(self):
        """
        dict invalid key loads()
        """
        with pytest.raises(ssrjson.JSONDecodeError):
            ssrjson.loads('{1:"value"}')
        with pytest.raises(ssrjson.JSONDecodeError):
            ssrjson.loads('{{"a":true}:true}')

    def test_dict_similar_keys(self):
        """
        loads() similar keys

        This was a regression in 3.4.2 caused by using
        the implementation in wy instead of wyhash.
        """
        assert ssrjson.loads(
            '{"cf_status_firefox67": "---", "cf_status_firefox57": "verified"}'
        ) == {"cf_status_firefox57": "verified", "cf_status_firefox67": "---"}

    def test_dict_pop_replace_first(self):
        "Test pop and replace a first key in a dict with other keys."
        data = {"id": "any", "other": "any"}
        data.pop("id")
        assert ssrjson.dumps(data) == '{"other":"any"}'
        assert ssrjson.dumps_to_bytes(data) == b'{"other":"any"}'
        data["id"] = "new"
        assert ssrjson.dumps(data) == '{"other":"any","id":"new"}'
        assert ssrjson.dumps_to_bytes(data) == b'{"other":"any","id":"new"}'

    def test_dict_pop_replace_last(self):
        "Test pop and replace a last key in a dict with other keys."
        data = {"other": "any", "id": "any"}
        data.pop("id")
        assert ssrjson.dumps(data) == '{"other":"any"}'
        assert ssrjson.dumps_to_bytes(data) == b'{"other":"any"}'
        data["id"] = "new"
        assert ssrjson.dumps(data) == '{"other":"any","id":"new"}'
        assert ssrjson.dumps_to_bytes(data) == b'{"other":"any","id":"new"}'

    def test_dict_pop(self):
        "Test pop and replace a key in a dict with no other keys."
        data = {"id": "any"}
        data.pop("id")
        assert ssrjson.dumps(data) == "{}"
        assert ssrjson.dumps_to_bytes(data) == b"{}"
        data["id"] = "new"
        assert ssrjson.dumps(data) == '{"id":"new"}'
        assert ssrjson.dumps_to_bytes(data) == b'{"id":"new"}'

    def test_in_place(self):
        "Mutate dict in-place"
        data = {"id": "any", "static": "msg"}
        data["id"] = "new"
        assert ssrjson.dumps(data) == '{"id":"new","static":"msg"}'
        assert ssrjson.dumps_to_bytes(data) == b'{"id":"new","static":"msg"}'

    def test_dict_0xff(self):
        "dk_size <= 0xff"
        data = {str(idx): idx for idx in range(0, 0xFF)}
        data.pop("112")
        data["112"] = 1
        data["113"] = 2
        assert ssrjson.loads(ssrjson.dumps(data)) == data
        assert ssrjson.loads(ssrjson.dumps_to_bytes(data)) == data

    def test_dict_0xff_repeated(self):
        "dk_size <= 0xff repeated"
        for _ in range(0, 100):
            data = {str(idx): idx for idx in range(0, 0xFF)}
            data.pop("112")
            data["112"] = 1
            data["113"] = 2
            assert ssrjson.loads(ssrjson.dumps(data)) == data
            assert ssrjson.loads(ssrjson.dumps_to_bytes(data)) == data

    def test_dict_0xffff(self):
        "dk_size <= 0xffff"
        data = {str(idx): idx for idx in range(0, 0xFFFF)}
        data.pop("112")
        data["112"] = 1
        data["113"] = 2
        assert ssrjson.loads(ssrjson.dumps(data)) == data
        assert ssrjson.loads(ssrjson.dumps_to_bytes(data)) == data

    def test_dict_0xffff_repeated(self):
        "dk_size <= 0xffff repeated"
        for _ in range(0, 100):
            data = {str(idx): idx for idx in range(0, 0xFFFF)}
            data.pop("112")
            data["112"] = 1
            data["113"] = 2
            assert ssrjson.loads(ssrjson.dumps(data)) == data
            assert ssrjson.loads(ssrjson.dumps_to_bytes(data)) == data

    def test_dict_dict(self):
        class C:
            def __init__(self):
                self.a = 0
                self.b = 1

        assert ssrjson.dumps(C().__dict__) == '{"a":0,"b":1}'
        assert ssrjson.dumps_to_bytes(C().__dict__) == b'{"a":0,"b":1}'
