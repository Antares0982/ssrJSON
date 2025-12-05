#  Copyright (c) 2025 Antares <antares0982@gmail.com>

#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:

#  The above copyright notice and this permission notice shall be included in all
#  copies or substantial portions of the Software.

#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.

import json
import random

import ssrjson


def random_concat_string(
    pool1, pool2=None, long_length=10000, long_string_probability=0.1, is_bytes=False
):
    result = []
    current_length = 0

    combined_pool = pool1 if pool2 is None else pool1 + pool2

    is_long_string = random.random() < long_string_probability

    if pool2 is not None:
        s = random.choice(pool2)
        result.append(s)
        current_length += len(s)

    while True:
        s = random.choice(combined_pool)
        result.append(s)
        current_length += len(s)

        if current_length >= long_length:
            break

        if not is_long_string:
            if random.random() < 0.1:
                break

    random.shuffle(result)

    return "".join(result) if not is_bytes else b"".join(result)


ascii_pool = ["a", "\\", "\f", "\n", '"', "\r", "\t", "\b"]

ascii_loads_pool = [
    "a",
    "\\\\",
    "\\f",
    "\\n",
    '\\"',
    "\\r",
    "\\t",
    "\\b",
    "/",
    "\\/",
    "\\u00ff",
    "\\u597d",
    "\\ud83d\\udc08",
]

ascii_loads_bytes_pool = [x.encode("utf-8") for x in ascii_loads_pool]

ucs1_pool = ["ÿ"]
ucs2_pool = ["好"]
ucs4_pool = ["🐈"]

bytes2_pool = [b"\xc3\xbf"]
bytes3_pool = [b"\xe5\xa5\xbd"]
bytes4_pool = [b"\xf0\x9f\x90\x88"]


def random_ascii_str():
    return random_concat_string(ascii_pool)


def random_ucs1_str():
    return random_concat_string(ascii_pool, ucs1_pool)


def random_ucs2_str():
    return random_concat_string(ascii_pool + ucs1_pool, ucs2_pool)


def random_ucs4_str():
    return random_concat_string(ascii_pool + ucs1_pool + ucs2_pool, ucs4_pool)


def random_ascii_str_for_loads():
    return random_concat_string(ascii_loads_pool)


def random_ucs1_str_for_loads():
    return random_concat_string(ascii_loads_pool, ucs1_pool)


def random_ucs2_str_for_loads():
    return random_concat_string(ascii_loads_pool + ucs1_pool, ucs2_pool)


def random_ucs4_str_for_loads():
    return random_concat_string(ascii_loads_pool + ucs1_pool + ucs2_pool, ucs4_pool)


def random_ascii_bytes() -> bytes:
    return random_concat_string(ascii_loads_bytes_pool, is_bytes=True)


def random_2bytes_bytes() -> bytes:
    return random_concat_string(ascii_loads_bytes_pool, bytes2_pool, is_bytes=True)


def random_3bytes_bytes() -> bytes:
    return random_concat_string(
        ascii_loads_bytes_pool + bytes2_pool, bytes3_pool, is_bytes=True
    )


def random_4bytes_bytes() -> bytes:
    return random_concat_string(
        ascii_loads_bytes_pool + bytes2_pool + bytes3_pool, bytes4_pool, is_bytes=True
    )


def get_random_str():
    rd = random.randint(0, 3)
    if rd == 0:
        s = random_ascii_str()
    elif rd == 1:
        s = random_ucs1_str()
    elif rd == 2:
        s = random_ucs2_str()
    elif rd == 3:
        s = random_ucs4_str()

    return s


def get_random_str_for_loads():
    rd = random.randint(0, 3)
    if rd == 0:
        s = random_ascii_str_for_loads()
    elif rd == 1:
        s = random_ucs1_str_for_loads()
    elif rd == 2:
        s = random_ucs2_str_for_loads()
    elif rd == 3:
        s = random_ucs4_str_for_loads()

    return s


def get_random_bytes_for_loads():
    rd = random.randint(0, 3)
    if rd == 0:
        b = random_ascii_bytes()
    elif rd == 1:
        b = random_2bytes_bytes()
    elif rd == 2:
        b = random_3bytes_bytes()
    elif rd == 3:
        b = random_4bytes_bytes()

    return b


class TestStr:
    def _test_dumps_str(self, d):
        x = ssrjson.dumps(d)
        assert x == json.dumps(d, ensure_ascii=False).replace(" ", "")
        assert ssrjson.dumps(d, indent=2) == json.dumps(d, indent=2, ensure_ascii=False)
        assert ssrjson.dumps(d, indent=4) == json.dumps(d, indent=4, ensure_ascii=False)
        return x

    def _test_dumps_bytes(self, d):
        def _do_test(is_write_cache: bool):
            b = ssrjson.dumps_to_bytes(d)
            assert b == json.dumps(d, ensure_ascii=False).replace(" ", "").encode(
                "utf-8"
            )
            assert ssrjson.dumps_to_bytes(
                d, indent=2, is_write_cache=is_write_cache
            ) == json.dumps(d, indent=2, ensure_ascii=False).encode("utf-8")
            assert ssrjson.dumps_to_bytes(
                d, indent=4, is_write_cache=is_write_cache
            ) == json.dumps(d, indent=4, ensure_ascii=False).encode("utf-8")
            return b

        a = _do_test(False)
        b = _do_test(True)
        assert a == b
        self._test_correct_utf8_cache_recursively(d)
        return b

    def _test_correct_utf8_cache(self, s: str):
        if not s:
            return
        s_copy = s[:-1] + s[-1]  # does not have a cache
        # compare if UTF-8 bytes encoded by CPython is same as ssrJSON
        assert s_copy.encode("utf-8") == s.encode("utf-8")

    def _test_correct_utf8_cache_recursively(self, d):
        if isinstance(d, str):
            self._test_correct_utf8_cache(d)
            return
        elif isinstance(d, list):
            for _d in d:
                self._test_correct_utf8_cache_recursively(_d)
            return
        elif isinstance(d, dict):
            for k, v in d.items():
                self._test_correct_utf8_cache_recursively(k)
                self._test_correct_utf8_cache_recursively(v)
            return

    def test_ascii_from_dumps(self):
        for _ in range(100):
            s = random_ascii_str()
            d = self._test_dumps_str(s)
            assert s == ssrjson.loads(d)
            b = self._test_dumps_bytes(s)
            assert s == ssrjson.loads(b)

    def test_ucs1_from_dumps(self):
        for _ in range(100):
            s = random_ucs1_str()
            d = self._test_dumps_str(s)
            assert s == ssrjson.loads(d)
            b = self._test_dumps_bytes(s)
            assert s == ssrjson.loads(b)

    def test_ucs2_from_dumps(self):
        for _ in range(100):
            s = random_ucs2_str()
            d = self._test_dumps_str(s)
            assert s == ssrjson.loads(d)
            b = self._test_dumps_bytes(s)
            assert s == ssrjson.loads(b)

    def test_ucs4_from_dumps(self):
        for _ in range(100):
            s = random_ucs4_str()
            d = self._test_dumps_str(s)
            assert s == ssrjson.loads(d)
            b = self._test_dumps_bytes(s)
            assert s == ssrjson.loads(b)

    def test_ascii_from_loads_str(self):
        for _ in range(100):
            d = '"{}"'.format(random_ascii_str_for_loads())
            assert json.loads(d) == ssrjson.loads(d)

    def test_ucs1_from_loads_str(self):
        for _ in range(100):
            d = '"{}"'.format(random_ucs1_str_for_loads())
            assert json.loads(d) == ssrjson.loads(d)

    def test_ucs2_from_loads_str(self):
        for _ in range(100):
            d = '"{}"'.format(random_ucs2_str_for_loads())
            assert json.loads(d) == ssrjson.loads(d)

    def test_ucs4_from_loads_str(self):
        for _ in range(100):
            d = '"{}"'.format(random_ucs4_str_for_loads())
            assert json.loads(d) == ssrjson.loads(d)

    def test_ascii_from_loads_bytes(self):
        for _ in range(100):
            b = b'"' + random_ascii_bytes() + b'"'
            assert json.loads(b) == ssrjson.loads(b)

    def test_2bytes_from_loads_bytes(self):
        for _ in range(100):
            b = b'"' + random_2bytes_bytes() + b'"'
            assert json.loads(b) == ssrjson.loads(b)

    def test_3bytes_from_loads_bytes(self):
        for _ in range(100):
            b = b'"' + random_3bytes_bytes() + b'"'
            assert json.loads(b) == ssrjson.loads(b)

    def test_4bytes_from_loads_bytes(self):
        for _ in range(100):
            b = b'"' + random_4bytes_bytes() + b'"'
            assert json.loads(b) == ssrjson.loads(b)

    def test_dumps_object(self):
        for _ in range(100):
            k = get_random_str()
            v = get_random_str()
            k2 = get_random_str()
            v2 = get_random_str()
            obj = {k: v, k2: v2}
            sample = (
                json.dumps(obj, ensure_ascii=False)
                .replace(": ", ":", 2)
                .replace(", ", ",", 1)
            )
            assert self._test_dumps_str(obj) == sample
            assert self._test_dumps_bytes(obj) == sample.encode("utf-8")

    def test_loads_str(self):
        template = '"{}":"{}","{}":"{}"'
        for _ in range(100):
            k = get_random_str_for_loads()
            v = get_random_str_for_loads()
            k2 = get_random_str_for_loads()
            v2 = get_random_str_for_loads()
            d = "{" + template.format(k, v, k2, v2) + "}"
            sample = json.loads(d)
            assert ssrjson.loads(d) == sample

    def test_loads_bytes(self):
        template = [b'{"', b'":"', b'","', b'":"', b'"}']
        for _ in range(100):
            k = get_random_bytes_for_loads()
            v = get_random_bytes_for_loads()
            k2 = get_random_bytes_for_loads()
            v2 = get_random_bytes_for_loads()
            b = (
                template[0]
                + k
                + template[1]
                + v
                + template[2]
                + k2
                + template[3]
                + v2
                + template[4]
            )
            sample = json.loads(b)
            assert ssrjson.loads(b) == sample

    # cover
    def test_str_cover(self):
        for a in ("a", "ÿ", "好", "🐈"):
            for b in ("a", "ÿ", "好", "🐈"):
                obj = {a: b}
                self._test_dumps_str(obj)
                self._test_dumps_bytes(obj)
                obj = [a, b]
                self._test_dumps_str(obj)
                self._test_dumps_bytes(obj)

        for a in ("a" * 4096, "ÿ" * 4096, "好" * 4096, "🐈" * 4096):
            for b in ("a" * 4096, "ÿ" * 4096, "好" * 4096, "🐈" * 4096):
                obj = {a: b}
                self._test_dumps_str(obj)
                self._test_dumps_bytes(obj)
                obj = [a, b]
                self._test_dumps_str(obj)
                self._test_dumps_bytes(obj)

        def _test_1(repeat: int):
            for c in ("ÿ", "好", "🐈"):
                a = "a" * repeat + c + "a" * repeat
                b = "a" * repeat + c * 2 + "a" * repeat
                obj = {a: b}
                self._test_dumps_bytes(obj)
                a = "a" * repeat + c + "a" * repeat
                b = "a" * repeat + c * 2 + "a" * repeat
                obj = {a: b}
                self._test_dumps_str(obj)

        for repeat in (1, 2, 4, 8, 16, 32, 64, 128, 256):
            _test_1(repeat)
            _test_1(repeat + 4096)
