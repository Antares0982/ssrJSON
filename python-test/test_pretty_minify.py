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

import math
import random

import ssrjson

SAMPLE = {
    "a": 1,
    "b": [1, 2, 3, "4", [5], {"h": False}, False, True, None],
    "c": {"d": True, "e": None},
    "f": "g",
}
PRETTY_000 = """{
  "a":1,"b":[1,2,3,"4",[5],{"h":false},false,true,null],"c":{"d":true,"e":null},"f":"g"}"""

PRETTY_001 = """{
  "a"
  : 1,
  "b": [
    1,
    2 ,3  
    
    ,"4",[5],{ "h": false}
,false,
true,null
  ]
  ,
  "c": {
    "d": true,
    "e": null}, "f":
"g"}"""

PRETTY_002 = """{
  \t"a"
  
  : 1,
  "b": [
    1,
    2,
    
    3,
    
  "4",[5]
     ,       {"h"
:false
}
,false,
true,null
  ],
  "c": {
    "d": true,
    "e": null}, "f":
"g"}"""
MINIFY_000 = """{"a":1,"b":[1,2,3,"4",[5],{"h":false},false,true,null],"c":{"d":true,"e":null},"f":"g"}"""
MINIFY_001 = """{"a":
  1,"b": [
    1,
    2,3,"4",[5],{"h":\tfalse} , false,
true,null
  ],
  "c": {
    "d": true,
    "e": null}, "f":
"g"}"""

MINIFY_002 = """{"a":
  
  1,"b": [
    1,
    2,3,
  "4",[5],{
  "h":
  false
  },false,true,null],
  "c": {
    "d":true,
    "e": null}, "f":
"g"}"""


EMPTY_CHARS = [" ", "\t", "\n", "  ", "    "]


class TestPrettyMinify:
    def _pretty_minify_test(self, content: str, sample=None):
        if sample is None:
            sample = SAMPLE
        obj = ssrjson.loads(content)
        assert obj == sample
        obj = ssrjson.loads(content.encode("utf-8"))
        assert obj == sample

    def _find_insertion_points(self, content: str, start_idx: int):
        def _in_word(c):
            return (
                c == '"'
                or (ord(c) >= ord("a") and ord(c) <= ord("z"))
                or (ord(c) >= ord("0") and ord(c) <= ord("9"))
            )

        entered = False
        insertion_points = []

        for idx, c in enumerate(content[start_idx:]):
            idx += start_idx
            if _in_word(c):
                if not entered:
                    insertion_points.append(idx)
                    entered = True
                else:
                    continue
            else:
                if entered:
                    insertion_points.append(idx)
                    entered = False
                else:
                    insertion_points.append(idx)
        insertion_points.reverse()
        return insertion_points

    def _random_insertion(self, s, insertion_points):
        for i in insertion_points:
            do_insert = random.randint(0, 1)
            if do_insert:
                part1 = s[:i]
                part2 = s[i:]
                tmp = ""
                while True:
                    r = random.randint(0, 9)
                    if r == 0:
                        break
                    c = EMPTY_CHARS[r % len(EMPTY_CHARS)]
                    tmp += c
                s = part1 + tmp + part2
        return s

    def test_pretty000(self):
        insertion_points = self._find_insertion_points(PRETTY_000, 4)
        for _ in range(50):
            self._pretty_minify_test(
                self._random_insertion(PRETTY_000, insertion_points)
            )

    def test_pretty001(self):
        self._pretty_minify_test(PRETTY_001)

    def test_pretty002(self):
        self._pretty_minify_test(PRETTY_002)

    def test_minify000(self):
        insertion_points = self._find_insertion_points(MINIFY_000, 1)
        for _ in range(50):
            self._pretty_minify_test(
                self._random_insertion(MINIFY_000, insertion_points)
            )

    def test_minify001(self):
        self._pretty_minify_test(MINIFY_001)

    def test_minify002(self):
        self._pretty_minify_test(MINIFY_002)

    def test_nan(self):
        content = """[
  nan]"""
        assert math.isnan(ssrjson.loads(content)[0])
        assert math.isnan(ssrjson.loads(content.encode("utf-8"))[0])

    def test_inf(self):
        content = """[
  infinity]"""
        assert math.isinf(ssrjson.loads(content)[0])
        assert math.isinf(ssrjson.loads(content.encode("utf-8"))[0])

    def test_empty_arr(self):
        self._pretty_minify_test(
            """[
                                 
                                 ]""",
            [],
        )
