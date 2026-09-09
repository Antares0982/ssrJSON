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

"""Decoding strings out of `bytes` input.

Equality alone is a weak check here: `==` does not distinguish the four
PyUnicode kinds, and picking the wrong one (ASCII vs UCS1 vs UCS2 vs UCS4) is
the failure mode the UTF-8 decoder is most likely to hit. Every check below
therefore also pins the kind, via `sys.getsizeof`: the size of a compact string
is an affine function of its length whose two coefficients depend only on the
kind, so measuring the coefficients once lets us assert the kind without poking
at the object layout.

Note that comparing `sys.getsizeof` against the object `json.loads` returns
would *not* work: for one-character strings below U+0100 CPython hands out a
shared singleton, and those carry a cached utf8 representation that counts
towards their size. The expected kind is therefore derived from the decoded
value instead.
"""

import json
import sys

import pytest

import ssrjson

# One code point per kind. LATIN1 is two UTF-8 bytes but still fits ucs1, which
# is the case the u8 decode state has to get right.
ASCII = "a"
LATIN1 = "\u00e9"  # 2 utf-8 bytes, code point < 0x100
CJK = "\u4e2d"  # 3 utf-8 bytes
ASTRAL = "\U0001f600"  # 4 utf-8 bytes

# Vector widths the decoder may be compiled for, and the unrolled x4 stride.
VECTOR_WIDTHS = (16, 32, 64)
BOUNDARY_LENGTHS = sorted(
    {0, 1, 2, 3, 5, 7}
    | {
        n + d
        for w in VECTOR_WIDTHS
        for n in (w, 2 * w, 3 * w, 4 * w)
        for d in (-1, 0, 1)
    }
)


def _measure_bases():
    """sizeof(header) + 1 for the trailing NUL, per PyUnicode kind."""
    n = 100
    return {
        0: sys.getsizeof(ASCII * n) - n,
        1: sys.getsizeof(LATIN1 * n) - n,
        2: sys.getsizeof(CJK * n) - 2 * n,
        4: sys.getsizeof(ASTRAL * n) - 4 * n,
    }


_KIND_BASE = _measure_bases()


def expected_kind(value):
    """The canonical (minimal) PyUnicode kind for `value`. 0 means ASCII."""
    if not value:
        return 0
    largest = max(map(ord, value))
    if largest < 0x80:
        return 0
    if largest < 0x100:
        return 1
    if largest < 0x10000:
        return 2
    return 4


def check_kind(got):
    assert isinstance(got, str)
    kind = expected_kind(got)
    expected_size = _KIND_BASE[kind] + len(got) * (kind or 1)
    size = sys.getsizeof(got)
    if size == expected_size:
        return
    # A shared CPython singleton is canonical by construction, but may carry a
    # cached utf8 representation that counts towards its size.
    if len(got) == 1 and got is chr(ord(got)):
        return
    raise AssertionError(
        f"wrong PyUnicode kind for {got!r}: expected kind {kind} "
        f"(size {expected_size}), got size {size}"
    )


def check_value(value):
    """`value` must decode identically from str and from bytes input."""
    document = json.dumps(value, ensure_ascii=False)
    reference = json.loads(document)

    from_str = ssrjson.loads(document)
    from_bytes = ssrjson.loads(document.encode("utf-8"))
    from_bytearray = ssrjson.loads(bytearray(document.encode("utf-8")))

    for got in (from_str, from_bytes, from_bytearray):
        assert got == reference
    check_kinds(from_bytes, reference)
    check_kinds(from_bytearray, reference)


def check_kinds(got, reference):
    """Recursively check the PyUnicode kind of every string in the structure."""
    assert type(got) is type(reference)
    if isinstance(reference, str):
        assert got == reference
        check_kind(got)
    elif isinstance(reference, list):
        assert len(got) == len(reference)
        for a, b in zip(got, reference):
            check_kinds(a, b)
    elif isinstance(reference, dict):
        assert len(got) == len(reference)
        for key, ref_value in reference.items():
            # look the key up by content, then check the stored key object too:
            # object keys take the caching path in the decoder
            got_keys = [k for k in got if k == key]
            assert len(got_keys) == 1
            check_kinds(got_keys[0], key)
            check_kinds(got[key], ref_value)


class TestBytesDecodeKind:
    @pytest.mark.parametrize("length", BOUNDARY_LENGTHS)
    @pytest.mark.parametrize(
        "char", [ASCII, LATIN1, CJK, ASTRAL], ids=["ascii", "latin1", "cjk", "astral"]
    )
    def test_homogeneous(self, char, length):
        check_value(char * length)

    @pytest.mark.parametrize("length", BOUNDARY_LENGTHS)
    @pytest.mark.parametrize(
        "chars",
        [
            ASCII + LATIN1,
            ASCII + CJK,
            ASCII + ASTRAL,
            LATIN1 + CJK,
            LATIN1 + ASTRAL,
            CJK + ASTRAL,
            ASCII + LATIN1 + CJK + ASTRAL,
            ASTRAL + CJK + LATIN1 + ASCII,
        ],
        ids=[
            "ascii-latin1",
            "ascii-cjk",
            "ascii-astral",
            "latin1-cjk",
            "latin1-astral",
            "cjk-astral",
            "ascending",
            "descending",
        ],
    )
    def test_mixed(self, chars, length):
        check_value(chars * length)

    @pytest.mark.parametrize(
        "prefix_length", [0, 1, 15, 16, 17, 31, 32, 33, 63, 64, 65]
    )
    @pytest.mark.parametrize(
        "tail", [ASCII, LATIN1, CJK, ASTRAL], ids=["ascii", "latin1", "cjk", "astral"]
    )
    def test_promotion_position(self, prefix_length, tail):
        """The widening code point lands at every offset within a block."""
        check_value("a" * prefix_length + tail + "z")

    def test_as_object_key(self):
        """Keys take the caching path, and are hashed."""
        for char in (ASCII, LATIN1, CJK, ASTRAL):
            for length in (1, 8, 16, 17, 64, 65):
                key = char * length
                check_value({key: key})
                # decode twice: the second one may come out of the key cache
                document = json.dumps({key: 1}, ensure_ascii=False).encode("utf-8")
                for _ in range(2):
                    got = ssrjson.loads(document)
                    assert got == {key: 1}
                    check_kinds(got, {key: 1})

    @pytest.mark.parametrize(
        "char", [ASCII, LATIN1, CJK, ASTRAL], ids=["ascii", "latin1", "cjk", "astral"]
    )
    @pytest.mark.parametrize("delta", [-1, 0, 1])
    def test_dynamic_buffers(self, char, delta):
        """Documents large enough to allocate the source and destination
        buffers on the heap instead of reusing the per-decoder scratch space.

        SSRJSON_STRING_BUFFER_SIZE is 512 KiB, and both reservations compare
        against it, so a string of ~130k characters crosses one threshold and a
        document of ~530k bytes crosses the other. This is the only place where
        an undersized reservation is an out of bounds write rather than a write
        into the neighbouring scratch buffer.
        """
        for length in (130 * 1024 + delta, 540 * 1024 + delta):
            value = char * length
            check_value(value)
            # also as an object key and inside a container, which reserve the
            # destination buffer through a different call site
            check_value({value: [value]})

    def test_nested(self):
        check_value(
            {
                "ascii": ["a", "b" * 100],
                LATIN1 * 3: [LATIN1 * 40, CJK * 40],
                CJK * 3: {ASTRAL * 20: [ASCII * 20, ASTRAL, LATIN1]},
                "empty": ["", {}, []],
            }
        )


class TestBytesDecodeEscape:
    @pytest.mark.parametrize(
        "document,expected",
        [
            (r'"\""', '"'),
            (r'"\\"', "\\"),
            (r'"\/"', "/"),
            (r'"\b"', "\b"),
            (r'"\f"', "\f"),
            (r'"\n"', "\n"),
            (r'"\r"', "\r"),
            (r'"\t"', "\t"),
            (r'"\u0000"', "\x00"),
            (r'"\u007f"', "\x7f"),
            (r'"\u0080"', "\x80"),
            (r'"\u00e9"', "\u00e9"),
            (r'"\u00ff"', "\u00ff"),
            (r'"\u0100"', "\u0100"),
            (r'"\u4e2d"', "\u4e2d"),
            (r'"\uffff"', "\uffff"),
            (r'"\ud83d\ude00"', "\U0001f600"),
        ],
    )
    def test_single_escape(self, document, expected):
        got = ssrjson.loads(document.encode("utf-8"))
        assert got == expected
        check_kind(got)

    @pytest.mark.parametrize(
        "prefix_length", [0, 1, 15, 16, 17, 31, 32, 33, 63, 64, 65]
    )
    @pytest.mark.parametrize(
        "escape,expected",
        [
            (r"\n", "\n"),
            (r"\u00e9", "\u00e9"),
            (r"\u4e2d", "\u4e2d"),
            (r"\ud83d\ude00", "\U0001f600"),
        ],
        ids=["ascii", "latin1", "cjk", "astral"],
    )
    def test_escape_position(self, prefix_length, escape, expected):
        document = '"' + "a" * prefix_length + escape + 'z"'
        reference = "a" * prefix_length + expected + "z"
        got = ssrjson.loads(document.encode("utf-8"))
        assert got == reference
        check_kinds(got, reference)

    @pytest.mark.parametrize(
        "document,reference",
        [
            # escape before / after a wider raw code point: the buffer widens
            # from either direction
            (r'"\u00e9\u4e2d"', "\u00e9\u4e2d"),
            (r'"\u4e2d\u00e9"', "\u4e2d\u00e9"),
            ('"\u00e9\\u4e2d"', "\u00e9\u4e2d"),
            ('"\\u00e9\u4e2d"', "\u00e9\u4e2d"),
            ('"\u4e2d\\ud83d\\ude00"', "\u4e2d\U0001f600"),
            ('"\\ud83d\\ude00\u4e2d"', "\U0001f600\u4e2d"),
            ('"a\u00e9\\n\u4e2d\\t\U0001f600"', "a\u00e9\n\u4e2d\t\U0001f600"),
        ],
    )
    def test_escape_and_raw_mixed(self, document, reference):
        got = ssrjson.loads(document.encode("utf-8"))
        assert got == reference
        check_kinds(got, reference)

    @pytest.mark.parametrize("lead", [2, 3, 4])
    @pytest.mark.parametrize("fill", [LATIN1, CJK, ASTRAL, ASCII])
    def test_sequence_truncated_by_block_boundary(self, fill, lead):
        """A sequence straddling the end of a vector block.

        Whether byte 15 of a block ends a code point depends on byte 16, which
        a single register load cannot see. Get that wrong and a straddling
        sequence looks like a complete shorter one, so a fixed-stride fast path
        consumes one byte too many. Slide such a sequence across every offset.
        """
        wide = {2: LATIN1, 3: CJK, 4: ASTRAL}[lead]
        for width in VECTOR_WIDTHS:
            for offset in range(width + lead + 2):
                fill_len = len(fill.encode("utf-8"))
                # pad with `fill` up to roughly `offset` bytes, then straddle
                check_value(fill * (offset // fill_len) + wide + fill * 3)


class TestBytesDecodeInvalid:
    @pytest.mark.parametrize(
        "payload",
        [
            b"\x80",  # stray continuation byte
            b"\xbf",
            b"\xc0\x80",  # overlong two byte form of U+0000
            b"\xc1\xbf",  # overlong two byte form of U+007F
            b"\xc2",  # truncated two byte sequence
            b"\xe0\x80\x80",  # overlong three byte form
            b"\xe0\x9f\xbf",  # overlong three byte form of U+07FF
            b"\xe2\x82",  # truncated three byte sequence
            b"\xed\xa0\x80",  # U+D800 encoded as utf-8
            b"\xed\xbf\xbf",  # U+DFFF encoded as utf-8
            b"\xf0\x80\x80\x80",  # overlong four byte form
            b"\xf0\x8f\xbf\xbf",  # overlong four byte form of U+FFFF
            b"\xf4\x90\x80\x80",  # above U+10FFFF
            b"\xf5\x80\x80\x80",  # lead byte out of range
            b"\xff",
            b"\xf0\x9f\x98",  # truncated four byte sequence
        ],
    )
    @pytest.mark.parametrize("prefix_length", [0, 1, 17, 64, 65])
    def test_invalid_utf8(self, payload, prefix_length):
        document = b'"' + b"a" * prefix_length + payload + b'"'
        with pytest.raises(ssrjson.JSONDecodeError):
            ssrjson.loads(document)
        # also inside a container, which uses the other buffer reservation path
        with pytest.raises(ssrjson.JSONDecodeError):
            ssrjson.loads(b"[" + document + b"]")

    @pytest.mark.parametrize("prefix_length", [0, 1, 17, 64, 65])
    @pytest.mark.parametrize(
        "payload",
        [
            b"",  # unterminated
            b"\xe4\xb8\xad",  # unterminated, ends with a complete sequence
            b'\x01"',  # raw control character
            b'\x1f"',
            b'\\"',  # trailing backslash escapes the closing quote
            b'\\x"',  # unknown escape
            b'\\u00g0"',  # bad hex
            b'\\ud800"',  # lone high surrogate escape
            b'\\ud800\\u0041"',  # high surrogate not followed by a low one
            b'\\udc00"',  # lone low surrogate escape
        ],
    )
    def test_invalid_string(self, prefix_length, payload):
        document = b'"' + b"a" * prefix_length + payload
        with pytest.raises(ssrjson.JSONDecodeError):
            ssrjson.loads(document)

    def test_valid_after_invalid_prefix_bytes(self):
        """A sequence that only becomes invalid at its last byte."""
        for bad in (b"\xe4\xb8\x20", b"\xf0\x9f\x98\x20"):
            with pytest.raises(ssrjson.JSONDecodeError):
                ssrjson.loads(b'"' + bad + b'"')


class TestBytesDecodeAgainstStr:
    """The str decode path is the reference implementation."""

    @pytest.mark.parametrize(
        "value",
        [
            "",
            "a",
            "\u00e9" * 200,
            "\u4e2d\u6587" * 200,
            "\U0001f600" * 200,
            "".join(chr(c) for c in range(0x20, 0x7F)),
            "".join(chr(c) for c in range(0x80, 0x200)),
            "".join(chr(c) for c in range(0x4E00, 0x4E80)),
            "".join(chr(c) for c in range(0x1F600, 0x1F640)),
            # every code point length, repeated so that sequences land at many
            # different offsets inside a vector
            "".join("a\u00e9\u4e2d\U0001f600"[: (i % 4) + 1] for i in range(200)),
        ],
    )
    def test_roundtrip(self, value):
        check_value(value)
        check_value([value])
        check_value({value: value})


class TestBytesDecodeInvalidInVectorShapes:
    """Invalid UTF-8 arranged so that a SIMD block kernel, not the scalar
    fallback, is what has to reject it.

    The payloads in `TestBytesDecodeInvalid` all sit behind an ASCII prefix.
    That makes the block holding them fail every shape check, so the decoder
    leaves the vector path and the scalar fallback reports the error -- the
    four block validators are never asked about invalid input at all. The cases
    here keep the block's end-of-code-point mask matching one of the vector
    shapes, so the matching validator is the one on the hook:

      utf8_seq2x8_has_error_128   eight two byte sequences
      utf8_seq3x4_has_error_128   four three byte sequences
      utf8_seq4x4_has_error_128   four four byte sequences
      utf8_block_has_error_128    the mixed shuffle table path (x86 only)

    Two things have to hold for a case to reach its validator, and both are
    checked rather than assumed:

    * the leading run of valid text must be a whole number of blocks, so that
      the bad unit starts on a block boundary. `test_filler_is_block_aligned`
      pins that by decoding the uncorrupted document.
    * the destination width must already be the one whose instantiation
      compiles that branch, which is what choosing the filler's code point
      does (seq3x4 needs ucs2 or wider, seq4x4 needs ucs4).
    """

    UCS1 = b"\xc3\xa9"  # U+00E9, two utf-8 bytes, still ucs1
    UCS2 = b"\xd0\xb0"  # U+0430
    CJK = b"\xe4\xb8\xad"  # U+4E2D
    ASTRAL = b"\xf0\x9f\x98\x80"  # U+1F600

    # (filler, units of filler before and after, bad unit repeated to fill the
    # shape). filler * lead must be a multiple of the block stride: 16 bytes
    # for seq2x8 and seq4x4, 12 for seq3x4, 16 for the shuffle table.
    CASES = [
        # eight two byte sequences, ucs1 destination
        (UCS1, 8, b"\xc0\x80" * 8),  # overlong lead
        (UCS1, 8, b"\x41\x80" * 8),  # ascii byte in a lead slot
        # eight two byte sequences, ucs2 destination
        (UCS2, 8, b"\xc0\x80" * 8),
        (UCS2, 8, b"\xc1\xbf" * 8),  # the other overlong lead
        (UCS2, 8, b"\xe0\x80" * 8),  # three byte lead in a two byte slot
        (UCS2, 8, b"\x41\x80" * 8),
        # four three byte sequences
        (CJK, 8, b"\xe0\x80\x80" * 4),  # overlong
        (CJK, 8, b"\xe0\x9f\xbf" * 4),  # overlong, largest such form
        (CJK, 8, b"\xed\xa0\x80" * 4),  # U+D800
        (CJK, 8, b"\xed\xbf\xbf" * 4),  # U+DFFF
        (CJK, 8, b"\xc2\x80\x80" * 4),  # two byte lead in a three byte slot
        # four four byte sequences
        (ASTRAL, 8, b"\xf0\x8f\xbf\xbf" * 4),  # overlong
        (ASTRAL, 8, b"\xf4\x90\x80\x80" * 4),  # above U+10FFFF
        (ASTRAL, 8, b"\xf5\x80\x80\x80" * 4),  # lead out of range
        # 0xfc leads nothing, but the four byte composition masks its low three
        # bits down to a code point inside the valid range, so the range test
        # alone would accept it and only the lead test rejects it
        (ASTRAL, 8, b"\xfc\x80\x80\x80" * 4),
        (ASTRAL, 8, b"\xe0\x80\x80\x80" * 4),  # three byte lead
        # a dense mix of one and two byte sequences, which is what sends the
        # block to the shuffle table instead of a homogeneous shape
        (UCS2, 8, UCS2 * 3 + b"a" + UCS2 * 2 + b"\xc0\x80" + b"a" + UCS2),
        (UCS2, 8, UCS2 * 3 + b"a" + UCS2 * 2 + b"\xd0\xb0\xb0" + UCS2),
        (UCS2, 8, UCS2 * 3 + b"a" + UCS2 * 2 + b"\xd0a" + b"a" + UCS2),
    ]

    @staticmethod
    def _document(filler, lead, bad):
        return b'"' + filler * lead + bad + filler * lead + b'"'

    @pytest.mark.parametrize("filler,lead,bad", CASES)
    def test_invalid_utf8_in_shape(self, filler, lead, bad):
        document = self._document(filler, lead, bad)
        with pytest.raises(ssrjson.JSONDecodeError):
            ssrjson.loads(document)
        # the container path reserves its buffer differently
        with pytest.raises(ssrjson.JSONDecodeError):
            ssrjson.loads(b"[" + document + b"]")

    @pytest.mark.parametrize("filler,lead,bad", CASES)
    def test_filler_is_block_aligned(self, filler, lead, bad):
        """The leading run must be a whole number of blocks, or the bad unit
        lands mid-block, loses its shape and gets caught by the scalar path --
        which would leave the validator above untested while the test still
        passes."""
        assert (len(filler) * lead) % 12 == 0 or (len(filler) * lead) % 16 == 0
        # and the same document without the corruption must decode
        good = self._document(filler, lead, filler * (len(bad) // len(filler)))
        check_value(json.loads(good))
