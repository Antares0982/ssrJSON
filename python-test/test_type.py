# # SPDX-License-Identifier: (Apache-2.0 OR MIT)

import io
import sys

import pytest
import random

try:
    import xxhash
except ImportError:
    xxhash = None

import ssrjson


class TestType:
    def test_fragment(self):
        """
        ssrjson.JSONDecodeError on fragments
        """
        for val in ("n", "{", "[", "t"):
            pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, val)

    def test_invalid(self):
        """
        ssrjson.JSONDecodeError on invalid
        """
        for val in ('{"age", 44}', "[31337,]", "[,31337]", "[]]", "[,]"):
            pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, val)

    def test_str(self):
        """
        str
        """
        for obj, ref in (
            ("blah", '"blah"'),
            ("東京", b'"\xe6\x9d\xb1\xe4\xba\xac"'.decode("utf-8")),
        ):
            assert ssrjson.dumps(obj) == ref
            assert ssrjson.dumps_to_bytes(obj) == ref.encode("utf-8")
            assert ssrjson.loads(ref) == obj

    def test_str_latin1(self):
        """
        str latin1
        """
        assert ssrjson.loads(ssrjson.dumps("üýþÿ")) == "üýþÿ"
        assert ssrjson.loads(ssrjson.dumps_to_bytes("üýþÿ")) == "üýþÿ"

    def test_str_long(self):
        """
        str long
        """
        for obj in ("aaaa" * 1024, "üýþÿ" * 1024, "好" * 1024, "�" * 1024):
            assert ssrjson.loads(ssrjson.dumps(obj)) == obj
            assert ssrjson.loads(ssrjson.dumps_to_bytes(obj)) == obj

    def test_str_other(self):
        """
        various str
        """
        for s in (
            '{"a":"aa","aaüý":"üýüý","aa":"aüaüýaa","üýüý":"üýüýüýüý","aaa":"aaaaaaaaaaa"}',  # range 0-255
            '{"a":"aa","aaüý":"üýüý","üý":"aüý好üý","aaa":"aü好ü好aaaüý好好üý好好aa","üý好好üý":"üýüýüýüý","aaaa":"aaaaaaa"}',  # range 0-65535
            '{"a":"aa","aaüý":"üýüý","üý":"aüý好üý","aaa":"aü好🐈ü🐈a好a🐈ü好a🐈好üaaaüý好好🐈🐈üý🐈🐈aa好好aa🐈🐈üý好好aa🐈🐈好好üýaa","üý好好üý":"üýüý好好好üýüý","üýüýüý":"üýüý","aaaa":"aaaaaaa"}',  # range 0-1114110
        ):
            obj = ssrjson.loads(s)
            assert ssrjson.loads(ssrjson.dumps(obj)) == obj
            d = dict()
            for k, v in obj.items():
                k2 = ""
                v2 = ""
                for c in k:
                    k2 += c * 64
                for c in v:
                    v2 += c * 64
                d[k2] = v2
            assert ssrjson.loads(ssrjson.dumps(d)) == d

    def test_str_other_escape(self):
        """
        various str
        """
        escapes = [
            "\\\\",
            '\\"',
            "\\u0061",
            "\\u00ff",
            "\\u0666",
            "\\u597d",
            "\\ud83d\\udc08",
        ]
        escape_refs = [
            "\\\\",
            '\\"',
            "a",
            "ÿ",
            "٦",
            "好",
            "🐈",
        ]

        def update_immutable_indices(_s: str, pattern: str, immutable_indices: set):
            left = 0
            _l = len(pattern)
            while True:
                index = _s.find(pattern, left)
                if index == -1:
                    break
                for i in range(1, _l):
                    immutable_indices.add(index + i)
                left = index + _l

        def get_random_indices(_l: int, immutable_indices: set, indices_set: set):
            count = random.randint(1, _l - 1)
            for _ in range(count):
                index = random.randint(2, _l - 2)
                if index not in immutable_indices:
                    indices_set.add(index)

        def get_variant(_s: str):
            _l = len(_s)
            immutable_indices = set()
            update_immutable_indices(_s, '":"', immutable_indices)
            update_immutable_indices(_s, '","', immutable_indices)
            rand_indices = set()
            get_random_indices(_l, immutable_indices, rand_indices)
            all_indices = sorted([x for x in rand_indices], reverse=True)
            escapes_size = len(escapes)
            ref = s
            for index in all_indices:
                _r = random.randint(0, escapes_size - 1)
                escape = escapes[_r]
                escape_ref = escape_refs[_r]
                _s = _s[:index] + escape + _s[index:]
                ref = ref[:index] + escape_ref + ref[index:]
            return _s, ref

        def split_kv(_s: str):
            return list(map(lambda x: x.split(":"), _s[1 : len(_s) - 1]))

        for s in (
            '{"a":"aa","aaüý":"üýüý","aa":"aüaüýaa","üýüý":"üýüýüýüý","aaa":"aaaaaaaaaaa"}',  # range 0-255
            '{"a":"aa","aaüý":"üýüý","üý":"aüý好üý","aaa":"aü好ü好aaaüý好好üý好好aa","üý好好üý":"üýüýüýüý","aaaa":"aaaaaaa"}',  # range 0-65535
            '{"a":"aa","aaüý":"üýüý","üý":"aüý好üý","aaa":"aü好🐈ü🐈a好a🐈ü好a🐈好üaaaüý好好🐈🐈üý🐈🐈aa好好aa🐈🐈üý好好aa🐈🐈好好üýaa","üý好好üý":"üýüý好好好üýüý","üýüýüý":"üýüý","aaaa":"aaaaaaa"}',  # range 0-1114110
        ):
            for _ in range(10):
                while True:
                    var, ref = get_variant(s)
                    dumped = ssrjson.dumps(ssrjson.loads(var))
                    a = sorted(split_kv(dumped))
                    b = sorted(split_kv(ref))
                    if len(a) == len(b):
                        assert a == b
                        break

    def test_str_2mib(self):
        ref = '🐈🐈🐈🐈🐈"üýa0s9999🐈🐈🐈🐈🐈9\0999\\9999' * 1 * 1
        assert ssrjson.loads(ssrjson.dumps(ref)) == ref
        assert ssrjson.loads(ssrjson.dumps_to_bytes(ref)) == ref

    def test_str_very_long(self):
        """
        str long enough to trigger overflow in bytecount
        """
        for obj in ("aaaa" * 20000, "üýþÿ" * 20000, "好" * 20000, "�" * 20000):
            assert ssrjson.loads(ssrjson.dumps(obj)) == obj
            assert ssrjson.loads(ssrjson.dumps_to_bytes(obj)) == obj

    def test_str_replacement(self):
        """
        str roundtrip �
        """
        assert ssrjson.dumps("�") == b'"\xef\xbf\xbd"'.decode("utf-8")
        assert ssrjson.dumps_to_bytes("�") == b'"\xef\xbf\xbd"'
        assert ssrjson.loads(b'"\xef\xbf\xbd"') == "�"

    def test_str_trailing_4_byte(self):
        ref = "うぞ〜😏🙌"
        assert ssrjson.loads(ssrjson.dumps(ref)) == ref
        assert ssrjson.loads(ssrjson.dumps_to_bytes(ref)) == ref

    def test_str_ascii_control(self):
        """
        worst case format_escaped_str_with_escapes() allocation
        """
        ref = "\x01\x1f" * 1024 * 16
        assert ssrjson.loads(ssrjson.dumps(ref)) == ref
        assert ssrjson.loads(ssrjson.dumps(ref, indent=2)) == ref
        assert ssrjson.loads(ssrjson.dumps_to_bytes(ref)) == ref
        assert ssrjson.loads(ssrjson.dumps_to_bytes(ref, indent=2)) == ref

    def test_str_escape_quote_0(self):
        assert ssrjson.dumps('"aaaaaaabb') == '"\\"aaaaaaabb"'
        assert ssrjson.dumps_to_bytes('"aaaaaaabb') == b'"\\"aaaaaaabb"'

    def test_str_escape_quote_1(self):
        assert ssrjson.dumps('a"aaaaaabb') == '"a\\"aaaaaabb"'
        assert ssrjson.dumps_to_bytes('a"aaaaaabb') == b'"a\\"aaaaaabb"'

    def test_str_escape_quote_2(self):
        assert ssrjson.dumps('aa"aaaaabb') == '"aa\\"aaaaabb"'
        assert ssrjson.dumps_to_bytes('aa"aaaaabb') == b'"aa\\"aaaaabb"'

    def test_str_escape_quote_3(self):
        assert ssrjson.dumps('aaa"aaaabb') == '"aaa\\"aaaabb"'
        assert ssrjson.dumps_to_bytes('aaa"aaaabb') == b'"aaa\\"aaaabb"'

    def test_str_escape_quote_4(self):
        assert ssrjson.dumps('aaaa"aaabb') == '"aaaa\\"aaabb"'
        assert ssrjson.dumps_to_bytes('aaaa"aaabb') == b'"aaaa\\"aaabb"'

    def test_str_escape_quote_5(self):
        assert ssrjson.dumps('aaaaa"aabb') == '"aaaaa\\"aabb"'
        assert ssrjson.dumps_to_bytes('aaaaa"aabb') == b'"aaaaa\\"aabb"'

    def test_str_escape_quote_6(self):
        assert ssrjson.dumps('aaaaaa"abb') == '"aaaaaa\\"abb"'
        assert ssrjson.dumps_to_bytes('aaaaaa"abb') == b'"aaaaaa\\"abb"'

    def test_str_escape_quote_7(self):
        assert ssrjson.dumps('aaaaaaa"bb') == '"aaaaaaa\\"bb"'
        assert ssrjson.dumps_to_bytes('aaaaaaa"bb') == b'"aaaaaaa\\"bb"'

    def test_str_escape_quote_8(self):
        assert ssrjson.dumps('aaaaaaaab"') == '"aaaaaaaab\\""'
        assert ssrjson.dumps_to_bytes('aaaaaaaab"') == b'"aaaaaaaab\\""'

    def test_str_escape_quote_multi(self):
        assert (
            ssrjson.dumps('aa"aaaaabbbbbbbbbbbbbbbbbbbb"bb')
            == '"aa\\"aaaaabbbbbbbbbbbbbbbbbbbb\\"bb"'
        )
        assert (
            ssrjson.dumps_to_bytes('aa"aaaaabbbbbbbbbbbbbbbbbbbb"bb')
            == b'"aa\\"aaaaabbbbbbbbbbbbbbbbbbbb\\"bb"'
        )

    def test_str_escape_backslash_0(self):
        assert ssrjson.dumps("\\aaaaaaabb") == '"\\\\aaaaaaabb"'
        assert ssrjson.dumps_to_bytes("\\aaaaaaabb") == b'"\\\\aaaaaaabb"'

    def test_str_escape_backslash_1(self):
        assert ssrjson.dumps("a\\aaaaaabb") == '"a\\\\aaaaaabb"'
        assert ssrjson.dumps_to_bytes("a\\aaaaaabb") == b'"a\\\\aaaaaabb"'

    def test_str_escape_backslash_2(self):
        assert ssrjson.dumps("aa\\aaaaabb") == '"aa\\\\aaaaabb"'
        assert ssrjson.dumps_to_bytes("aa\\aaaaabb") == b'"aa\\\\aaaaabb"'

    def test_str_escape_backslash_3(self):
        assert ssrjson.dumps("aaa\\aaaabb") == '"aaa\\\\aaaabb"'
        assert ssrjson.dumps_to_bytes("aaa\\aaaabb") == b'"aaa\\\\aaaabb"'

    def test_str_escape_backslash_4(self):
        assert ssrjson.dumps("aaaa\\aaabb") == '"aaaa\\\\aaabb"'
        assert ssrjson.dumps_to_bytes("aaaa\\aaabb") == b'"aaaa\\\\aaabb"'

    def test_str_escape_backslash_5(self):
        assert ssrjson.dumps("aaaaa\\aabb") == '"aaaaa\\\\aabb"'
        assert ssrjson.dumps_to_bytes("aaaaa\\aabb") == b'"aaaaa\\\\aabb"'

    def test_str_escape_backslash_6(self):
        assert ssrjson.dumps("aaaaaa\\abb") == '"aaaaaa\\\\abb"'
        assert ssrjson.dumps_to_bytes("aaaaaa\\abb") == b'"aaaaaa\\\\abb"'

    def test_str_escape_backslash_7(self):
        assert ssrjson.dumps("aaaaaaa\\bb") == '"aaaaaaa\\\\bb"'
        assert ssrjson.dumps_to_bytes("aaaaaaa\\bb") == b'"aaaaaaa\\\\bb"'

    def test_str_escape_backslash_8(self):
        assert ssrjson.dumps("aaaaaaaab\\") == '"aaaaaaaab\\\\"'
        assert ssrjson.dumps_to_bytes("aaaaaaaab\\") == b'"aaaaaaaab\\\\"'

    def test_str_escape_backslash_multi(self):
        assert (
            ssrjson.dumps("aa\\aaaaabbbbbbbbbbbbbbbbbbbb\\bb")
            == '"aa\\\\aaaaabbbbbbbbbbbbbbbbbbbb\\\\bb"'
        )
        assert (
            ssrjson.dumps_to_bytes("aa\\aaaaabbbbbbbbbbbbbbbbbbbb\\bb")
            == b'"aa\\\\aaaaabbbbbbbbbbbbbbbbbbbb\\\\bb"'
        )

    def test_str_escape_x32_0(self):
        assert ssrjson.dumps("\taaaaaaabb") == '"\\taaaaaaabb"'
        assert ssrjson.dumps_to_bytes("\taaaaaaabb") == b'"\\taaaaaaabb"'

    def test_str_escape_x32_1(self):
        assert ssrjson.dumps("a\taaaaaabb") == '"a\\taaaaaabb"'
        assert ssrjson.dumps_to_bytes("a\taaaaaabb") == b'"a\\taaaaaabb"'

    def test_str_escape_x32_2(self):
        assert ssrjson.dumps("aa\taaaaabb") == '"aa\\taaaaabb"'
        assert ssrjson.dumps_to_bytes("aa\taaaaabb") == b'"aa\\taaaaabb"'

    def test_str_escape_x32_3(self):
        assert ssrjson.dumps("aaa\taaaabb") == '"aaa\\taaaabb"'
        assert ssrjson.dumps_to_bytes("aaa\taaaabb") == b'"aaa\\taaaabb"'

    def test_str_escape_x32_4(self):
        assert ssrjson.dumps("aaaa\taaabb") == '"aaaa\\taaabb"'
        assert ssrjson.dumps_to_bytes("aaaa\taaabb") == b'"aaaa\\taaabb"'

    def test_str_escape_x32_5(self):
        assert ssrjson.dumps("aaaaa\taabb") == '"aaaaa\\taabb"'
        assert ssrjson.dumps_to_bytes("aaaaa\taabb") == b'"aaaaa\\taabb"'

    def test_str_escape_x32_6(self):
        assert ssrjson.dumps("aaaaaa\tabb") == '"aaaaaa\\tabb"'
        assert ssrjson.dumps_to_bytes("aaaaaa\tabb") == b'"aaaaaa\\tabb"'

    def test_str_escape_x32_7(self):
        assert ssrjson.dumps("aaaaaaa\tbb") == '"aaaaaaa\\tbb"'
        assert ssrjson.dumps_to_bytes("aaaaaaa\tbb") == b'"aaaaaaa\\tbb"'

    def test_str_escape_x32_8(self):
        assert ssrjson.dumps("aaaaaaaab\t") == '"aaaaaaaab\\t"'
        assert ssrjson.dumps_to_bytes("aaaaaaaab\t") == b'"aaaaaaaab\\t"'

    def test_str_escape_x32_multi(self):
        assert (
            ssrjson.dumps("aa\taaaaabbbbbbbbbbbbbbbbbbbb\tbb")
            == '"aa\\taaaaabbbbbbbbbbbbbbbbbbbb\\tbb"'
        )
        assert (
            ssrjson.dumps_to_bytes("aa\taaaaabbbbbbbbbbbbbbbbbbbb\tbb")
            == b'"aa\\taaaaabbbbbbbbbbbbbbbbbbbb\\tbb"'
        )

    def test_str_emoji(self):
        ref = "®️"
        assert ssrjson.loads(ssrjson.dumps(ref)) == ref
        assert ssrjson.loads(ssrjson.dumps_to_bytes(ref)) == ref

    def test_str_emoji_escape(self):
        ref = '/"®️/"'
        assert ssrjson.loads(ssrjson.dumps(ref)) == ref
        assert ssrjson.loads(ssrjson.dumps_to_bytes(ref)) == ref

    def test_very_long_list(self):
        ssrjson.dumps([[]] * 1024 * 16)
        ssrjson.dumps_to_bytes([[]] * 1024 * 16)

    def test_very_long_list_pretty(self):
        ssrjson.dumps([[]] * 1024 * 16, indent=2)
        ssrjson.dumps_to_bytes([[]] * 1024 * 16, indent=2)

    def test_very_long_dict(self):
        ssrjson.dumps([{}] * 1024 * 16)
        ssrjson.dumps_to_bytes([{}] * 1024 * 16)

    def test_very_long_dict_pretty(self):
        ssrjson.dumps([{}] * 1024 * 16, indent=2)
        ssrjson.dumps_to_bytes([{}] * 1024 * 16, indent=2)

    def test_very_long_str_empty(self):
        ssrjson.dumps([""] * 1024 * 16)
        ssrjson.dumps_to_bytes([""] * 1024 * 16)

    def test_very_long_str_empty_pretty(self):
        ssrjson.dumps([""] * 1024 * 16, indent=2)
        ssrjson.dumps_to_bytes([""] * 1024 * 16, indent=2)

    def test_very_long_str_not_empty(self):
        ssrjson.dumps(["a"] * 1024 * 16)
        ssrjson.dumps_to_bytes(["a"] * 1024 * 16)

    def test_very_long_str_not_empty_pretty(self):
        ssrjson.dumps(["a"] * 1024 * 16, indent=2)
        ssrjson.dumps_to_bytes(["a"] * 1024 * 16, indent=2)

    def test_very_long_bool(self):
        ssrjson.dumps([True] * 1024 * 16)
        ssrjson.dumps_to_bytes([True] * 1024 * 16)

    def test_very_long_bool_pretty(self):
        ssrjson.dumps([True] * 1024 * 16, indent=2)
        ssrjson.dumps_to_bytes([True] * 1024 * 16, indent=2)

    def test_very_long_int(self):
        ssrjson.dumps([(2**64) - 1] * 1024 * 16)
        ssrjson.dumps_to_bytes([(2**64) - 1] * 1024 * 16)

    def test_very_long_int_pretty(self):
        ssrjson.dumps([(2**64) - 1] * 1024 * 16, indent=2)
        ssrjson.dumps_to_bytes([(2**64) - 1] * 1024 * 16, indent=2)

    def test_very_long_float(self):
        ssrjson.dumps([sys.float_info.max] * 1024 * 16)
        ssrjson.dumps_to_bytes([sys.float_info.max] * 1024 * 16)

    def test_very_long_float_pretty(self):
        ssrjson.dumps([sys.float_info.max] * 1024 * 16, indent=2)
        ssrjson.dumps_to_bytes([sys.float_info.max] * 1024 * 16, indent=2)

    def test_str_surrogates_loads(self):
        """
        str unicode surrogates loads()
        """
        import json

        json.loads('"\ud800"')
        json.loads('"\ud83d\ude80"')
        json.loads('"\udcff"')
        ssrjson.loads('"\ud800"')
        ssrjson.loads('"\ud83d\ude80"')
        ssrjson.loads('"\udcff"')
        # pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, '"\ud800"')
        # pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, '"\ud83d\ude80"')
        # pytest.raises(ssrjson.JSONDecodeError, ssrjson.loads, '"\udcff"')
        pytest.raises(
            ssrjson.JSONDecodeError, ssrjson.loads, b'"\xed\xa0\xbd\xed\xba\x80"'
        )  # \ud83d\ude80

    def test_str_surrogates_dumps(self):
        """
        str unicode surrogates dumps()
        """
        import json

        json.dumps("\ud800")
        json.dumps("\ud83d\ude80")
        json.dumps("\udcff")
        json.dumps({"\ud83d\ude80": None})
        ssrjson.dumps("\ud800")
        ssrjson.dumps("\ud83d\ude80")
        ssrjson.dumps("\udcff")
        ssrjson.dumps({"\ud83d\ude80": None})
        pytest.raises(ssrjson.JSONEncodeError, ssrjson.dumps_to_bytes, "\ud800")
        pytest.raises(ssrjson.JSONEncodeError, ssrjson.dumps_to_bytes, "\ud83d\ude80")
        pytest.raises(ssrjson.JSONEncodeError, ssrjson.dumps_to_bytes, "\udcff")
        pytest.raises(
            ssrjson.JSONEncodeError, ssrjson.dumps_to_bytes, {"\ud83d\ude80": None}
        )

    def test_bytes_dumps(self):
        """
        bytes dumps not supported
        """
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps([b"a"])
            ssrjson.dumps_to_bytes([b"a"])

    def test_bytes_loads(self):
        """
        bytes loads
        """
        assert ssrjson.loads(b"[]") == []

    def test_bytearray_loads(self):
        """
        bytearray loads
        """
        arr = bytearray()
        arr.extend(b"[]")
        assert ssrjson.loads(arr) == []

    # def test_memoryview_loads(self):
    #     """
    #     memoryview loads
    #     """
    #     arr = bytearray()
    #     arr.extend(b"[]")
    #     assert ssrjson.loads(memoryview(arr)) == []

    # def test_bytesio_loads(self):
    #     """
    #     memoryview loads
    #     """
    #     arr = io.BytesIO(b"[]")
    #     assert ssrjson.loads(arr.getbuffer()) == []

    def test_bool(self):
        """
        bool
        """
        for obj, ref in ((True, "true"), (False, "false")):
            assert ssrjson.dumps(obj) == ref
            assert ssrjson.dumps_to_bytes(obj) == ref.encode("utf-8")
            assert ssrjson.loads(ref) == obj

    def test_bool_true_array(self):
        """
        bool true array
        """
        obj = [True] * 256
        ref = "[" + ("true," * 255) + "true]"
        assert ssrjson.dumps(obj) == ref
        assert ssrjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert ssrjson.loads(ref) == obj

    def test_bool_false_array(self):
        """
        bool false array
        """
        obj = [False] * 256
        ref = "[" + ("false," * 255) + "false]"
        assert ssrjson.dumps(obj) == ref
        assert ssrjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert ssrjson.loads(ref) == obj

    def test_none(self):
        """
        null
        """
        obj = None
        ref = "null"
        assert ssrjson.dumps(obj) == ref
        assert ssrjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert ssrjson.loads(ref) == obj

    def test_int(self):
        """
        int compact and non-compact
        """
        obj = [-5000, -1000, -10, -5, -2, -1, 0, 1, 2, 5, 10, 1000, 50000]
        ref = "[-5000,-1000,-10,-5,-2,-1,0,1,2,5,10,1000,50000]"
        assert ssrjson.dumps(obj) == ref
        assert ssrjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert ssrjson.loads(ref) == obj

    def test_null_array(self):
        """
        null array
        """
        obj = [None] * 256
        ref = "[" + ("null," * 255) + "null]"
        assert ssrjson.dumps(obj) == ref
        assert ssrjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert ssrjson.loads(ref) == obj

    def test_nan_dumps(self):
        """
        NaN serializes to null
        """
        assert ssrjson.dumps(float("NaN")) == "NaN"
        assert ssrjson.dumps_to_bytes(float("NaN")) == b"NaN"

    def test_nan_loads(self):
        """
        NaN loads
        """
        import math

        def is_nan_list(obj):
            assert type(obj) is list
            assert len(obj) == 1
            assert math.isnan(obj[0])

        is_nan_list(ssrjson.loads("[NaN]"))
        is_nan_list(ssrjson.loads("[nan]"))

    def test_infinity_dumps(self):
        """
        Infinity serializes to null
        """
        assert ssrjson.dumps(float("Infinity")) == "Infinity"
        assert ssrjson.dumps_to_bytes(float("Infinity")) == b"Infinity"

    def test_infinity_loads(self):
        """
        Infinity loads
        """
        import math

        def is_inf_list(obj):
            assert type(obj) is list
            assert len(obj) == 1
            assert math.isinf(obj[0])

        is_inf_list(ssrjson.loads("[infinity]"))
        is_inf_list(ssrjson.loads("[Infinity]"))
        is_inf_list(ssrjson.loads("[-Infinity]"))
        is_inf_list(ssrjson.loads("[-infinity]"))

    def test_int_53(self):
        """
        int 53-bit
        """
        for val in (9007199254740991, -9007199254740991):
            assert ssrjson.loads(str(val)) == val
            assert ssrjson.dumps(val) == str(val)
            assert ssrjson.dumps_to_bytes(val) == str(val).encode("utf-8")

    def test_int_53_exc(self):
        """
        int 53-bit exception on 64-bit
        """
        for val in (9007199254740992, -9007199254740992):
            assert ssrjson.dumps(val) == str(val)
            assert ssrjson.dumps_to_bytes(val) == str(val).encode("utf-8")
            # with pytest.raises(ssrjson.JSONEncodeError):
            #     ssrjson.dumps(val)

    def test_int_53_exc_usize(self):
        """
        int 53-bit exception on 64-bit usize
        """
        for val in (9223372036854775808, 18446744073709551615):
            assert ssrjson.dumps(val) == str(val)
            assert ssrjson.dumps_to_bytes(val) == str(val).encode("utf-8")
            # with pytest.raises(ssrjson.JSONEncodeError):
            #     ssrjson.dumps(val)

    def test_int_64(self):
        """
        int 64-bit
        """
        for val in (9223372036854775807, -9223372036854775807):
            assert ssrjson.loads(str(val)) == val
            assert ssrjson.dumps(val) == str(val)
            assert ssrjson.dumps_to_bytes(val) == str(val).encode("utf-8")

    def test_uint_64(self):
        """
        uint 64-bit
        """
        for val in (0, 9223372036854775808, 18446744073709551615):
            assert ssrjson.loads(str(val)) == val
            assert ssrjson.dumps(val) == str(val)
            assert ssrjson.dumps_to_bytes(val) == str(val).encode("utf-8")

    def test_int_128(self):
        """
        int 128-bit
        """
        for val in (18446744073709551616, -9223372036854775809):
            pytest.raises(ssrjson.JSONEncodeError, ssrjson.dumps, val)
            pytest.raises(ssrjson.JSONEncodeError, ssrjson.dumps_to_bytes, val)

    def test_float(self):
        """
        float
        """
        assert -1.1234567893 == ssrjson.loads("-1.1234567893")
        assert -1.234567893 == ssrjson.loads("-1.234567893")
        assert -1.34567893 == ssrjson.loads("-1.34567893")
        assert -1.4567893 == ssrjson.loads("-1.4567893")
        assert -1.567893 == ssrjson.loads("-1.567893")
        assert -1.67893 == ssrjson.loads("-1.67893")
        assert -1.7893 == ssrjson.loads("-1.7893")
        assert -1.893 == ssrjson.loads("-1.893")
        assert -1.3 == ssrjson.loads("-1.3")

        assert 1.1234567893 == ssrjson.loads("1.1234567893")
        assert 1.234567893 == ssrjson.loads("1.234567893")
        assert 1.34567893 == ssrjson.loads("1.34567893")
        assert 1.4567893 == ssrjson.loads("1.4567893")
        assert 1.567893 == ssrjson.loads("1.567893")
        assert 1.67893 == ssrjson.loads("1.67893")
        assert 1.7893 == ssrjson.loads("1.7893")
        assert 1.893 == ssrjson.loads("1.893")
        assert 1.3 == ssrjson.loads("1.3")

    def test_float_precision_loads(self):
        """
        float precision loads()
        """
        assert ssrjson.loads("31.245270191439438") == 31.245270191439438
        assert ssrjson.loads("-31.245270191439438") == -31.245270191439438
        assert ssrjson.loads("121.48791951161945") == 121.48791951161945
        assert ssrjson.loads("-121.48791951161945") == -121.48791951161945
        assert ssrjson.loads("100.78399658203125") == 100.78399658203125
        assert ssrjson.loads("-100.78399658203125") == -100.78399658203125

        assert ssrjson.loads("3.1245270191439438e1") == 31.245270191439438
        assert ssrjson.loads("-3.1245270191439438e1") == -31.245270191439438
        assert ssrjson.loads("1.2148791951161945e2") == 121.48791951161945
        assert ssrjson.loads("-1.2148791951161945e2") == -121.48791951161945
        assert ssrjson.loads("1.0078399658203125e2") == 100.78399658203125
        assert ssrjson.loads("-1.0078399658203125e2") == -100.78399658203125

    def test_float_precision_dumps(self):
        """
        float precision dumps()
        """
        assert ssrjson.dumps(31.245270191439438) == "3.1245270191439438e1"
        assert ssrjson.dumps(-31.245270191439438) == "-3.1245270191439438e1"
        assert ssrjson.dumps(121.48791951161945) == "1.2148791951161945e2"
        assert ssrjson.dumps(-121.48791951161945) == "-1.2148791951161945e2"
        assert ssrjson.dumps(100.78399658203125) == "1.0078399658203125e2"
        assert ssrjson.dumps(-100.78399658203125) == "-1.0078399658203125e2"

        assert ssrjson.dumps_to_bytes(31.245270191439438) == b"3.1245270191439438e1"
        assert ssrjson.dumps_to_bytes(-31.245270191439438) == b"-3.1245270191439438e1"
        assert ssrjson.dumps_to_bytes(121.48791951161945) == b"1.2148791951161945e2"
        assert ssrjson.dumps_to_bytes(-121.48791951161945) == b"-1.2148791951161945e2"
        assert ssrjson.dumps_to_bytes(100.78399658203125) == b"1.0078399658203125e2"
        assert ssrjson.dumps_to_bytes(-100.78399658203125) == b"-1.0078399658203125e2"

    def test_float_edge(self):
        """
        float edge cases
        """
        assert ssrjson.dumps(0.8701) == "8.701e-1"
        assert ssrjson.dumps_to_bytes(0.8701) == b"8.701e-1"

        assert ssrjson.loads("0.8701") == 0.8701
        assert ssrjson.loads(b"0.8701") == 0.8701
        assert ssrjson.loads("8.701e-1") == 0.8701
        assert ssrjson.loads(b"8.701e-1") == 0.8701
        assert (
            ssrjson.loads("0.0000000000000000000000000000000000000000000000000123e50")
            == 1.23
        )
        assert ssrjson.loads("0.4e5") == 40000.0
        assert ssrjson.loads("0.00e-00") == 0.0
        assert ssrjson.loads("0.4e-001") == 0.04
        assert ssrjson.loads("0.123456789e-12") == 1.23456789e-13
        assert ssrjson.loads("1.234567890E+34") == 1.23456789e34
        assert ssrjson.loads("23456789012E66") == 2.3456789012e76

    def test_float_notation(self):
        """
        float notation
        """
        for val in ("1.337E40", "1.337e+40", "1337e40", "1.337E-4"):
            obj = ssrjson.loads(val)
            assert obj == float(val)
            assert ssrjson.dumps(val) == ('"%s"' % val)
            assert ssrjson.dumps_to_bytes(val) == ('"%s"' % val).encode("utf-8")

    def test_list(self):
        """
        list
        """
        obj = ["a", "😊", True, {"b": 1.1}, 2]
        ref = '["a","😊",true,{"b":1.1},2]'
        assert ssrjson.dumps(obj) == ref
        assert ssrjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert ssrjson.loads(ref) == obj

    def test_tuple(self):
        """
        tuple
        """
        obj = ("a", "😊", True, {"b": 1.1}, 2)
        ref = '["a","😊",true,{"b":1.1},2]'
        assert ssrjson.dumps(obj) == ref
        assert ssrjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert ssrjson.loads(ref) == list(obj)

    def test_object(self):
        """
        object() dumps()
        """
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps(object())
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps_to_bytes(object())
