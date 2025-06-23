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
