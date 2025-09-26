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

import pytest
import ssrjson


def _generate_special_float_string():
    sign = random.choice(["", "-"])

    int_length = random.choice([0, 1, 5, 10, 50, 100, 300])
    frac_length = random.choice([0, 1, 5, 10, 50, 100, 300])

    if int_length == 0:
        int_part = ""
    else:
        first_digit = random.choice("123456789")
        other_digits = "".join(random.choices("0123456789", k=int_length - 1))
        int_part = first_digit + other_digits

    if frac_length == 0:
        frac_part = ""
    else:
        frac_part = "".join(random.choices("0123456789", k=frac_length))

    if int_part == "" and frac_part == "":
        int_part = "0"
        has_decimal = False
    else:
        has_decimal = frac_length > 0

    has_exponent = random.choice([True, False])
    exponent_part = ""
    if has_exponent:
        e_char = random.choice(["e", "E"])
        e_sign = random.choice(["", "+", "-"])
        exp_length = random.choice([1, 3, 5, 10, 20, 30, 50])
        if exp_length == 1:
            exp_digits = random.choice("0123456789")
        else:
            first_exp_digit = random.choice("123456789")
            other_exp_digits = "".join(random.choices("0123456789", k=exp_length - 1))
            exp_digits = first_exp_digit + other_exp_digits

        exponent_part = e_char + e_sign + exp_digits

    s = sign
    s += int_part
    if has_decimal:
        s += "." + frac_part
    s += exponent_part

    return s


def generate_special_float_string():
    while True:
        ret = _generate_special_float_string()
        try:
            json.loads(ret)
            return ret
        except Exception:
            continue


template_u1 = """{"a":1,"b":@}"""
template_u2 = """{"好":1,"b":@}"""
template_u4 = """{"🐈":1,"b":@}"""
templates = [
    template_u1,
    template_u2,
    template_u4,
]


class TestFloat:
    def test_float000(
        self,
    ):
        for _ in range(1000):
            s = generate_special_float_string()
            f = float(s)
            x = ssrjson.loads(s)
            assert x == ssrjson.loads(s.encode("utf-8"))
            assert x == pytest.approx(f)
            for template in templates:
                y = ssrjson.loads(template.replace("@", s))
                assert y == ssrjson.loads(template.replace("@", s).encode("utf-8"))
                assert y["b"] == x
