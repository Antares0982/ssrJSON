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


import pytest
import ssrjson


class TestFail:
    def _decode_fail_with_match(self, s: str, match: str):
        with pytest.raises(ssrjson.JSONDecodeError, match=match):
            ssrjson.loads(s)
        with pytest.raises(ssrjson.JSONDecodeError, match=match):
            ssrjson.loads(s.encode("utf-8"))

    # test fail dict
    def test_decodefail001(self):
        """
        test literal 'false' fail
        """
        err = "such as 'false'"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '{"' + k + '":f}'
            self._decode_fail_with_match(s, err)
            s = '{\n  "' + k + '": f\n}'
            self._decode_fail_with_match(s, err)

    def test_decodefail002(self):
        """
        test literal 'true' fail
        """
        err = "invalid literal, expected a valid literal such as 'true'"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '{"' + k + '":t}'
            self._decode_fail_with_match(s, err)
            s = '{\n  "' + k + '": t\n}'
            self._decode_fail_with_match(s, err)

    def test_decodefail003(self):
        """
        test invalid number
        """
        err = "invalid number"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '{"' + k + '":-}'
            self._decode_fail_with_match(s, err)
            s = '{\n  "' + k + '": -\n}'
            self._decode_fail_with_match(s, err)

    def test_decodefail004(self):
        """
        test literal 'null' fail
        """
        err = "such as 'null'"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '{"' + k + '":n}'
            self._decode_fail_with_match(s, err)
            s = '{\n  "' + k + '": n\n}'
            self._decode_fail_with_match(s, err)

    def test_decodefail005(self):
        """
        test invalid JSON value fail
        """
        err = "unexpected character, expected a valid JSON value"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '{"' + k + '":i}'
            self._decode_fail_with_match(s, err)
            s = '{\n  "' + k + '": i\n}'
            self._decode_fail_with_match(s, err)
            s = '{"' + k + '":#}'
            self._decode_fail_with_match(s, err)
            s = '{\n  "' + k + '": #\n}'
            self._decode_fail_with_match(s, err)

    def test_decodefail006(self):
        """
        test fail string in object
        """
        err = "invalid string"
        s = '{"a}'
        self._decode_fail_with_match(s, err)
        s = '{\n  "a}'
        self._decode_fail_with_match(s, err)
        for k in ("a", "ÿ", "好", "🐈"):
            s = '{"' + k + '":"}'
            self._decode_fail_with_match(s, err)
            s = '{\n  "' + k + '": "\n}'
            self._decode_fail_with_match(s, err)

    def test_decodefail007(self):
        """
        test trailing comma fail
        """
        err = "trailing comma is not allowed"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '{"' + k + '":1,}'
            self._decode_fail_with_match(s, err)
            s = '{\n  "' + k + '": 1,\n}'
            self._decode_fail_with_match(s, err)

    def test_decodefail008(self):
        """
        test fail key
        """
        err = "unexpected character, expected a string for object key"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '{"' + k + '":1,a}'
            self._decode_fail_with_match(s, err)
            s = '{\n  "' + k + '": 1,a\n}'
            self._decode_fail_with_match(s, err)

    def test_decodefail009(self):
        """
        test fail key
        """
        err = "unexpected character, expected a colon after object key"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '{"' + k + '":1,"z"}'
            self._decode_fail_with_match(s, err)
            s = '{\n  "' + k + '": 1,"z"\n}'
            self._decode_fail_with_match(s, err)

    def test_decodefail010(self):
        """
        test fail object end
        """
        err = "unexpected character, expected a comma or a closing brace"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '{"' + k + '":1'
            self._decode_fail_with_match(s, err)
            s = '{\n  "' + k + '": 1\n'
            self._decode_fail_with_match(s, err)

    # test fail list
    def test_decodefail101(self):
        """
        test literal 'false' fail
        """
        err = "such as 'false'"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '["' + k + '",f]'
            self._decode_fail_with_match(s, err)
            s = '[\n  "' + k + '",\n  f\n]'
            self._decode_fail_with_match(s, err)

    def test_decodefail102(self):
        """
        test literal 'true' fail
        """
        err = "invalid literal, expected a valid literal such as 'true'"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '["' + k + '",t]'
            self._decode_fail_with_match(s, err)
            s = '[\n  "' + k + '",\n  t\n]'
            self._decode_fail_with_match(s, err)

    def test_decodefail103(self):
        """
        test invalid number
        """
        err = "invalid number"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '["' + k + '",-]'
            self._decode_fail_with_match(s, err)
            s = '[\n  "' + k + '",\n  -\n]'
            self._decode_fail_with_match(s, err)

    def test_decodefail104(self):
        """
        test literal 'null' fail
        """
        err = "such as 'null'"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '["' + k + '",n]'
            self._decode_fail_with_match(s, err)
            s = '[\n  "' + k + '",\n  n\n]'
            self._decode_fail_with_match(s, err)

    def test_decodefail105(self):
        """
        test invalid JSON value fail
        """
        err = "unexpected character, expected a valid JSON value"
        for k in ("a", "ÿ", "好", "🐈"):
            s = '["' + k + '",i]'
            self._decode_fail_with_match(s, err)
            s = '[\n  "' + k + '",\n  i\n]'
            self._decode_fail_with_match(s, err)

    def test_decodefail106(self):
        """
        test trailing comma fail
        """
        err = "trailing comma is not allowed"
        self._decode_fail_with_match("[1,]", err)
        self._decode_fail_with_match("[\n  1,\n]", err)

    def test_decodefail107(self):
        """
        test missing comma in list
        """
        err = "expected a comma or a closing bracket"
        self._decode_fail_with_match("[1 q]", err)
        self._decode_fail_with_match("[\n  1 q]", err)

    def test_decodefail108(self):
        """
        test invalid string
        """
        err = "invalid string"
        self._decode_fail_with_match('[1,"]', err)
        self._decode_fail_with_match('[\n  1, "]', err)

    # test fail at large position
    def test_decodefail201(self):
        err = "invalid literal, expected a valid literal such as 'true'"
        s = "[" + '"a",' * 4096 + "t]"
        self._decode_fail_with_match(s, err)
        s = "[" + '\n  "a",' * 4096 + "\n  t\n]"
        self._decode_fail_with_match(s, err)

    # test other failures
    def test_decodefail301(self):
        """
        test unexpected content after document
        """
        err = "unexpected content after document"
        self._decode_fail_with_match("{}a", err)
        self._decode_fail_with_match("{\n  \n}a", err)
        self._decode_fail_with_match("{}                \n    a", err)
        self._decode_fail_with_match("{\n  \n}                \n    a", err)
