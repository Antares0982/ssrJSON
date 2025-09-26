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

import random

import pytest
import ssrjson


def _generate_random_number_string():
    prob_long = 0.05
    normal_length_range = (1, 10)
    long_length_range = (50, 100)
    if random.random() < prob_long:
        length = random.randint(*long_length_range)
    else:
        length = random.randint(*normal_length_range)

    if length == 1:
        return str(random.randint(0, 9))
    else:
        first_digit = str(random.randint(1, 9))
        other_digits = "".join(str(random.randint(0, 9)) for _ in range(length - 1))
        return first_digit + other_digits


def generate_random_number_string():
    s = _generate_random_number_string()
    has_sign = random.random() < 0.5
    return "-" + s if has_sign else s


template_u1 = """{"a":1,"b":@}"""
template_u2 = """{"好":1,"b":@}"""
template_u4 = """{"🐈":1,"b":@}"""
templates = [
    template_u1,
    template_u2,
    template_u4,
]


class TestInt:
    def test_int000(self):
        for _ in range(1000):
            s = generate_random_number_string()
            i = int(s)
            x = ssrjson.loads(s)
            assert x == ssrjson.loads(s.encode("utf-8"))
            if i >= -9223372036854775808 and i <= 18446744073709551615:
                assert x == i
            else:
                assert x == pytest.approx(i)
            for template in templates:
                y = ssrjson.loads(template.replace("@", s))
                assert y == ssrjson.loads(template.replace("@", s).encode("utf-8"))
                assert y["b"] == x

    def test_int001(self):
        for template in templates:
            y = ssrjson.loads(template.replace("@", "-9223372036854775809"))
            assert y["b"] == float(-9223372036854775809)

    def test_int002(self):
        for template in templates:
            y = ssrjson.loads(template.replace("@", "-10000000000000000999"))
            assert y["b"] == float(-10000000000000000999)

    def test_int003(self):
        for template in templates:
            y = ssrjson.loads(template.replace("@", "7568627624816930841e1"))
            assert y["b"] == float(7568627624816930841e1)

    def test_int004(self):
        for template in templates:
            pytest.raises(
                ssrjson.JSONDecodeError,
                ssrjson.loads,
                template.replace("@", "7568627624816930841."),
            )
