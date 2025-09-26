# # SPDX-License-Identifier: (Apache-2.0 OR MIT)

import ssrjson

try:
    from typing import TypedDict  # type: ignore
except ImportError:
    from typing_extensions import TypedDict


class TestTypedDict:
    def test_typeddict(self):
        """
        dumps() TypedDict
        """

        class TypedDict1(TypedDict):
            a: str
            b: int

        obj = TypedDict1(a="a", b=1)
        assert ssrjson.dumps(obj) == '{"a":"a","b":1}'
        assert ssrjson.dumps_to_bytes(obj) == b'{"a":"a","b":1}'
