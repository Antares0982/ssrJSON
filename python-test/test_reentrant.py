import gc
import json

import ssrjson


class C:
    c: "C"

    def __del__(self):
        ssrjson.loads('"' + "a" * 10000 + '"')


def test_reentrant():
    c = C()
    c.c = c
    del c

    ssrjson.loads("[" + "[]," * 1000 + "[]]")


class D:
    d: "D"

    def __del__(self):
        ssrjson.loads('[[[{"k": [1, 2, 3]}]], [[4]], {"a": [5, 6]}]')


def test_reentrant_container():
    # Python < 3.12 runs the cyclic GC on allocation, so the finalizers fire inside loads().
    expected = [["s%d" % i, [i, i + 1]] for i in range(3000)]
    s = json.dumps(expected)
    threshold = gc.get_threshold()
    try:
        for inp in (s, s.encode()):
            gc.disable()
            for _ in range(20):
                d = D()
                d.d = d
            del d
            gc.enable()
            gc.set_threshold(1)
            assert ssrjson.loads(inp) == expected
            gc.set_threshold(*threshold)
    finally:
        gc.set_threshold(*threshold)
        gc.enable()
