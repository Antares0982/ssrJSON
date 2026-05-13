import random
import sys

import ssrjson

try:
    import numpy as np

    HAS_NUMPY = True
    ssrjson.setup_numpy_types(np)
except ImportError:
    HAS_NUMPY = False


# --- Byte consumer ---


class ByteConsumer:
    """Consumes bytes from fuzzer input to make decisions."""

    __slots__ = ("data", "pos")

    def __init__(self, data: bytes):
        self.data = data
        self.pos = 0

    def has(self, n: int = 1) -> bool:
        return self.pos + n <= len(self.data)

    def take(self, n: int = 1) -> bytes:
        if self.pos + n > len(self.data):
            return b""
        result = self.data[self.pos : self.pos + n]
        self.pos += n
        return result

    def take_byte(self) -> int:
        if self.pos >= len(self.data):
            return 0
        b = self.data[self.pos]
        self.pos += 1
        return b

    def choose(self, n: int) -> int:
        """Choose a value in [0, n) using one byte."""
        return self.take_byte() % n

    def remaining(self) -> bytes:
        return self.data[self.pos :]


# --- Subclass types ---


class DictSubclass(dict):
    pass


class ListSubclass(list):
    pass


class TupleSubclass(tuple):
    pass


class IntSubclass(int):
    pass


class FloatSubclass(float):
    pass


class StrSubclass(str):
    pass


# --- Object generation ---

MAX_DEPTH = min(256, (sys.getrecursionlimit() - 200) // 3)

NUMPY_DTYPES = (
    [
        np.float16,
        np.float32,
        np.float64,
        np.uint8,
        np.uint16,
        np.uint32,
        np.uint64,
        np.int8,
        np.int16,
        np.int32,
        np.int64,
        np.bool_,
    ]
    if HAS_NUMPY
    else []
)

# value type IDs
TYPE_STR = 0
TYPE_INT = 1
TYPE_BOOL = 2
TYPE_NONE = 3
TYPE_FLOAT = 4
TYPE_NDARRAY = 5
TYPE_NUMPY_VAL = 6
NUM_VALUE_TYPES = 7 if HAS_NUMPY else 5

# container type IDs
TYPE_DICT = 0
TYPE_LIST = 1
TYPE_TUPLE = 2
NUM_CONTAINER_TYPES = 3


def make_rng(bc: ByteConsumer) -> random.Random:
    """Create a seeded RNG from the remaining fuzzer bytes."""
    seed_bytes = bc.take(4)
    seed = int.from_bytes(seed_bytes, "little") if len(seed_bytes) == 4 else 0
    return random.Random(seed)


def _is_escape_char(c: int) -> bool:
    """Check if a codepoint requires JSON escaping: control chars, '"', '\\'."""
    return c < 0x20 or c == 0x22 or c == 0x5C


_ESCAPE_OPTIONS = list(range(0x20)) + [0x22, 0x5C]


def _gen_safe_char(rng: random.Random, lo: int, hi: int) -> int:
    """Generate a random char in [lo, hi] that is NOT an escape char."""
    c = rng.randint(lo, hi)
    while _is_escape_char(c):
        c = rng.randint(lo, hi)
    return c


def _validate_gen_str(
    chars: list, ucs_choice: int, has_surrogate: bool, has_escape: bool
) -> None:
    """Validate gen_str output. Raises RuntimeError if invariants are violated."""
    has_surr_actual = any(0xD800 <= c <= 0xDFFF for c in chars)
    has_esc_actual = any(_is_escape_char(c) for c in chars)

    # Check UCS level
    if ucs_choice == 0:
        if any(c > 127 for c in chars):
            raise RuntimeError(
                f"gen_str bug: ucs_choice=0 (ASCII) but char > 127 found"
            )
    elif ucs_choice == 1:
        if any(c > 0xFF for c in chars if not (0xD800 <= c <= 0xDFFF)):
            raise RuntimeError(
                f"gen_str bug: ucs_choice=1 (Latin-1) but non-surrogate char > 0xFF"
            )
        non_surr_max = max((c for c in chars if not (0xD800 <= c <= 0xDFFF)), default=0)
        if non_surr_max < 0x80:
            raise RuntimeError(
                f"gen_str bug: ucs_choice=1 (Latin-1) but max non-surrogate "
                f"ord={non_surr_max:#x} < 0x80 (anchor missing)"
            )
    elif ucs_choice == 2:
        non_surr_max = max((c for c in chars if not (0xD800 <= c <= 0xDFFF)), default=0)
        if non_surr_max < 0x100:
            raise RuntimeError(
                f"gen_str bug: ucs_choice=2 (UCS2) but max non-surrogate "
                f"ord={non_surr_max:#x} < 0x100 (anchor missing)"
            )
        if any(c > 0xFFFF for c in chars if not (0xD800 <= c <= 0xDFFF)):
            raise RuntimeError(
                f"gen_str bug: ucs_choice=2 (UCS2) but non-surrogate char > 0xFFFF"
            )
    elif ucs_choice == 3:
        non_surr_max = max((c for c in chars if not (0xD800 <= c <= 0xDFFF)), default=0)
        if non_surr_max < 0x10000:
            raise RuntimeError(
                f"gen_str bug: ucs_choice=3 (UCS4) but max non-surrogate "
                f"ord={non_surr_max:#x} < 0x10000 (anchor missing)"
            )

    # Check surrogate invariant
    if has_surrogate and not has_surr_actual:
        raise RuntimeError("gen_str bug: has_surrogate=True but no surrogate in output")
    if not has_surrogate and has_surr_actual:
        raise RuntimeError(
            "gen_str bug: has_surrogate=False but surrogate found in output"
        )

    # Check escape invariant
    if has_escape and not has_esc_actual:
        raise RuntimeError("gen_str bug: has_escape=True but no escape char in output")
    if not has_escape and has_esc_actual:
        esc_found = [c for c in chars if _is_escape_char(c)]
        raise RuntimeError(
            f"gen_str bug: has_escape=False but escape chars found: "
            f"{[hex(c) for c in esc_found]}"
        )


def _gen_rand_char(
    rng: random.Random, ucs_choice: int, has_surrogate: bool, has_escape: bool
) -> int:
    """Generate one random base char respecting UCS level and escape constraints."""
    if ucs_choice == 0:
        if has_escape:
            return rng.randint(0, 127)
        return _gen_safe_char(rng, 0x20, 127)

    if ucs_choice == 1:
        if has_escape:
            return rng.randint(0, 255)
        return _gen_safe_char(rng, 0x20, 255)

    # UCS2 / UCS4
    hi = 0xFFFF if ucs_choice == 2 else 0x10FFFF
    if has_surrogate and rng.random() < 0.1:
        return rng.randint(0xD800, 0xDFFF)
    c = rng.randint(0, hi)
    if has_escape:
        while 0xD800 <= c <= 0xDFFF:
            c = rng.randint(0, hi)
    else:
        while 0xD800 <= c <= 0xDFFF or _is_escape_char(c):
            c = rng.randint(0, hi)
    return c


def _place_anchors(
    chars: list,
    rng: random.Random,
    ucs_choice: int,
    has_surrogate: bool,
    has_escape: bool,
) -> None:
    """Place anchor, surrogate, and escape chars at distinct reserved positions."""
    positions = list(range(len(chars)))
    rng.shuffle(positions)
    pos_idx = 0

    # Anchor: guarantee the string reaches the expected UCS level.
    if ucs_choice == 1:
        chars[positions[pos_idx]] = rng.randint(0x80, 0xFF)
        pos_idx += 1
    elif ucs_choice == 2:
        anchor = rng.randint(0x100, 0xFFFF)
        while 0xD800 <= anchor <= 0xDFFF:
            anchor = rng.randint(0x100, 0xFFFF)
        chars[positions[pos_idx]] = anchor
        pos_idx += 1
    elif ucs_choice == 3:
        chars[positions[pos_idx]] = rng.randint(0x10000, 0x10FFFF)
        pos_idx += 1

    if has_surrogate:
        chars[positions[pos_idx]] = rng.randint(0xD800, 0xDFFF)
        pos_idx += 1

    if has_escape:
        chars[positions[pos_idx]] = rng.choice(_ESCAPE_OPTIONS)
        pos_idx += 1


def gen_str(bc: ByteConsumer, rng: random.Random) -> str:
    """Generate a string based on fuzzer decisions."""
    # unicode range: 0=ASCII, 1=latin-1, 2=UCS2, 3=UCS4
    ucs_choice = bc.choose(4)
    # byte length range
    len_choice = bc.choose(4)
    # whether to include chars that require JSON escaping
    has_escape = bc.choose(2) == 0

    byte_ranges = [(0, 16), (16, 32), (32, 64), (64, 1024)]
    lo, hi = byte_ranges[len_choice]

    if ucs_choice <= 1:
        char_bytes = 1
    elif ucs_choice == 2:
        char_bytes = 2
    else:
        char_bytes = 4

    char_lo = lo // char_bytes
    char_hi = max(hi // char_bytes, char_lo + 1)

    # decide whether to include surrogates (only for UCS2/UCS4)
    has_surrogate = False
    if ucs_choice >= 2:
        has_surrogate = bc.choose(8) == 0  # ~12.5% chance

    # Each special property needs its own position in the string:
    # anchor (ucs >= 1), surrogate, escape char.
    min_length = (
        (1 if ucs_choice >= 1 else 0)
        + (1 if has_surrogate else 0)
        + (1 if has_escape else 0)
    )
    char_lo = max(char_lo, min_length)
    char_hi = max(char_hi, char_lo + 1)
    length = rng.randint(char_lo, char_hi - 1)

    chars = [
        _gen_rand_char(rng, ucs_choice, has_surrogate, has_escape)
        for _ in range(length)
    ]
    _place_anchors(chars, rng, ucs_choice, has_surrogate, has_escape)

    # skip validation for speed
    # _validate_gen_str(chars, ucs_choice, has_surrogate, has_escape)

    return "".join(chr(c) for c in chars), has_surrogate


def gen_int(bc: ByteConsumer, rng: random.Random):
    """Generate an int, possibly subclassed."""
    i64_min = -(1 << 63)
    u64_max = (1 << 64) - 1
    val = rng.randint(i64_min, u64_max)
    if bc.choose(8) == 0:
        return IntSubclass(val)
    return val


def gen_float(bc: ByteConsumer, rng: random.Random):
    """Generate a float, possibly subclassed."""
    kind = bc.choose(8)
    if kind == 0:
        val = float("inf")
    elif kind == 1:
        val = float("-inf")
    elif kind == 2:
        val = float("nan")
    else:
        val = rng.uniform(-1e308, 1e308)
    if bc.choose(8) == 0:
        return FloatSubclass(val)
    return val


def gen_numpy_dtype_and_value(bc: ByteConsumer, rng: random.Random):
    """Generate a single numpy scalar value."""
    dtype_idx = bc.choose(len(NUMPY_DTYPES))
    dtype = NUMPY_DTYPES[dtype_idx]

    if dtype == np.bool_:
        return dtype(rng.choice([True, False]))
    elif np.issubdtype(dtype, np.floating):
        kind = bc.choose(8)
        if kind == 0:
            return dtype(float("inf"))
        elif kind == 1:
            return dtype(float("-inf"))
        elif kind == 2:
            return dtype(float("nan"))
        else:
            info = np.finfo(dtype)
            return dtype(rng.uniform(float(info.min), float(info.max)))
    elif np.issubdtype(dtype, np.unsignedinteger):
        info = np.iinfo(dtype)
        return dtype(rng.randint(int(info.min), int(info.max)))
    else:  # signed integer
        info = np.iinfo(dtype)
        return dtype(rng.randint(int(info.min), int(info.max)))


def gen_ndarray(bc: ByteConsumer, rng: random.Random):
    """Generate a numpy ndarray."""
    dtype_idx = bc.choose(len(NUMPY_DTYPES))
    dtype = NUMPY_DTYPES[dtype_idx]

    ndim = bc.choose(4) + 1  # 1-4 dimensions (keep small to avoid huge arrays)
    # generate shape, total elements <= 4096
    shape = []
    remaining = 4096
    for i in range(ndim):
        if i == ndim - 1:
            dim = rng.randint(1, min(remaining, 64))
        else:
            dim = rng.randint(1, min(remaining, 8))
        shape.append(dim)
        remaining = max(1, remaining // dim)

    total = 1
    for d in shape:
        total *= d

    if dtype == np.bool_:
        data = [rng.choice([True, False]) for _ in range(total)]
    elif np.issubdtype(dtype, np.floating):
        data = []
        for _ in range(total):
            kind = rng.randint(0, 15)
            if kind == 0:
                data.append(float("inf"))
            elif kind == 1:
                data.append(float("-inf"))
            elif kind == 2:
                data.append(float("nan"))
            else:
                info = np.finfo(dtype)
                data.append(rng.uniform(float(info.min), float(info.max)))
    elif np.issubdtype(dtype, np.unsignedinteger):
        info = np.iinfo(dtype)
        data = [rng.randint(int(info.min), int(info.max)) for _ in range(total)]
    else:
        info = np.iinfo(dtype)
        data = [rng.randint(int(info.min), int(info.max)) for _ in range(total)]

    return np.array(data, dtype=dtype).reshape(shape)


def gen_value(bc: ByteConsumer, rng: random.Random):
    """Generate a non-container value. Returns (value, has_surrogate)."""
    type_id = bc.choose(NUM_VALUE_TYPES)
    has_surrogate = False
    if type_id == TYPE_STR:
        val, has_surrogate = gen_str(bc, rng)
        if bc.choose(4) == 0:
            val = StrSubclass(val)
    elif type_id == TYPE_INT:
        val = gen_int(bc, rng)
    elif type_id == TYPE_BOOL:
        val = rng.choice([True, False])
    elif type_id == TYPE_NONE:
        val = None
    elif type_id == TYPE_FLOAT:
        val = gen_float(bc, rng)
    elif type_id == TYPE_NDARRAY:
        val = gen_ndarray(bc, rng)
    elif type_id == TYPE_NUMPY_VAL:
        val = gen_numpy_dtype_and_value(bc, rng)
    else:
        val = None
    return val, has_surrogate


def gen_object(bc: ByteConsumer, rng: random.Random, depth: int = 0):
    """Generate an object (container or value). Returns (obj, has_surrogate, has_tuple, has_ndarray, has_subclass)."""
    if depth >= MAX_DEPTH or not bc.has():
        val, has_surr = gen_value(bc, rng)
        has_subclass = isinstance(val, (IntSubclass, FloatSubclass, StrSubclass))
        has_lossy_float = HAS_NUMPY and (
            isinstance(val, np.ndarray) or isinstance(val, (np.float16, np.float32))
        )
        return val, has_surr, False, has_lossy_float, has_subclass

    # 50% container, 50% value
    if bc.choose(2) == 0:
        return gen_container(bc, rng, depth)
    else:
        val, has_surr = gen_value(bc, rng)
        has_subclass = isinstance(val, (IntSubclass, FloatSubclass, StrSubclass))
        has_lossy_float = HAS_NUMPY and (
            isinstance(val, np.ndarray) or isinstance(val, (np.float16, np.float32))
        )
        return val, has_surr, False, has_lossy_float, has_subclass


def gen_container(bc: ByteConsumer, rng: random.Random, depth: int):
    """Generate a container. Returns (obj, has_surrogate, has_tuple, has_ndarray, has_subclass)."""
    ctype = bc.choose(NUM_CONTAINER_TYPES)
    if ctype == TYPE_DICT:
        return gen_dict(bc, rng, depth)
    elif ctype == TYPE_LIST:
        return gen_list(bc, rng, depth)
    else:
        return gen_tuple(bc, rng, depth)


def gen_dict(bc: ByteConsumer, rng: random.Random, depth: int):
    """Generate a dict, possibly subclassed."""
    is_subclass = bc.choose(8) == 0
    d = DictSubclass() if is_subclass else {}
    has_surr_any = False
    has_tuple_any = False
    has_ndarray_any = False
    has_subclass_any = is_subclass

    while bc.has():
        action = bc.choose(4)
        if action == 0:
            # exit container
            # when bytes is consumed out, always exit
            break
        # generate a key-value pair
        val, has_surr, has_tup, has_nd, has_sub = gen_object(bc, rng, depth + 1)
        has_surr_any |= has_surr
        has_tuple_any |= has_tup
        has_ndarray_any |= has_nd
        has_subclass_any |= has_sub
        # generate key (always str, no surrogates for simplicity)
        key_str, key_surr = gen_str(bc, rng)
        has_surr_any |= key_surr
        d[key_str] = val

    return d, has_surr_any, has_tuple_any, has_ndarray_any, has_subclass_any


def gen_list(bc: ByteConsumer, rng: random.Random, depth: int):
    """Generate a list, possibly subclassed."""
    is_subclass = bc.choose(8) == 0
    lst = ListSubclass() if is_subclass else []
    has_surr_any = False
    has_tuple_any = False
    has_ndarray_any = False
    has_subclass_any = is_subclass

    while bc.has():
        action = bc.choose(4)
        if action == 0:
            # exit container
            # when bytes is consumed out, always exit
            break
        val, has_surr, has_tup, has_nd, has_sub = gen_object(bc, rng, depth + 1)
        has_surr_any |= has_surr
        has_tuple_any |= has_tup
        has_ndarray_any |= has_nd
        has_subclass_any |= has_sub
        lst.append(val)

    return lst, has_surr_any, has_tuple_any, has_ndarray_any, has_subclass_any


def gen_tuple(bc: ByteConsumer, rng: random.Random, depth: int):
    """Generate a tuple, possibly subclassed."""
    is_subclass = bc.choose(8) == 0
    items = []
    has_surr_any = False
    has_tuple_any = True  # always true since we're generating a tuple
    has_ndarray_any = False
    has_subclass_any = is_subclass

    while bc.has():
        action = bc.choose(4)
        if action == 0:
            break
        val, has_surr, has_tup, has_nd, has_sub = gen_object(bc, rng, depth + 1)
        has_surr_any |= has_surr
        has_tuple_any |= has_tup
        has_ndarray_any |= has_nd
        has_subclass_any |= has_sub
        items.append(val)

    if is_subclass:
        t = TupleSubclass(items)
    else:
        t = tuple(items)
    return t, has_surr_any, has_tuple_any, has_ndarray_any, has_subclass_any


# --- Comparison helpers ---


def normalize_for_comparison(obj):
    """Normalize an object for comparison after dumps->loads round-trip.

    - tuple -> list
    - ndarray -> nested list
    - subclass -> base type
    - np.float32/float16 kept as numpy scalars for precision-aware comparison
    """
    if isinstance(obj, dict):
        return {k: normalize_for_comparison(v) for k, v in obj.items()}
    elif isinstance(obj, (list, tuple)):
        return [normalize_for_comparison(v) for v in obj]
    elif HAS_NUMPY and isinstance(obj, np.ndarray):
        if obj.dtype in (np.dtype("float32"), np.dtype("float16")):
            # Keep as numpy scalars so values_equal can convert decoded floats back
            return [normalize_for_comparison(obj[i]) for i in range(len(obj))]
        return normalize_for_comparison(obj.tolist())
    elif HAS_NUMPY and isinstance(obj, (np.float32, np.float16)):
        return obj  # keep for precision-aware comparison
    elif HAS_NUMPY and isinstance(obj, np.floating):
        return float(obj)
    elif HAS_NUMPY and isinstance(obj, np.integer):
        return int(obj)
    elif HAS_NUMPY and isinstance(obj, np.bool_):
        return bool(obj)
    elif isinstance(obj, bool):
        return obj
    elif isinstance(obj, int):
        return int(obj)  # strip subclass
    elif isinstance(obj, float):
        return float(obj)  # strip subclass
    elif isinstance(obj, str):
        return str(obj)  # strip subclass
    return obj


def values_equal(a, b) -> bool:
    """Compare two values, handling NaN and float32/float16 precision."""
    # np.float32/float16 vs decoded Python float: convert decoded back to same type
    if HAS_NUMPY and isinstance(a, (np.float32, np.float16)) and isinstance(b, float):
        b_cast = type(a)(b)
        if a != a and b_cast != b_cast:  # both NaN
            return True
        return bool(a == b_cast)
    if HAS_NUMPY and isinstance(b, (np.float32, np.float16)) and isinstance(a, float):
        a_cast = type(b)(a)
        if b != b and a_cast != a_cast:  # both NaN
            return True
        return bool(b == a_cast)
    if isinstance(a, float) and isinstance(b, float):
        if a != a and b != b:  # both NaN
            return True
        return a == b
    if isinstance(a, dict) and isinstance(b, dict):
        if set(a.keys()) != set(b.keys()):
            return False
        return all(values_equal(a[k], b[k]) for k in a)
    if isinstance(a, list) and isinstance(b, list):
        if len(a) != len(b):
            return False
        return all(values_equal(x, y) for x, y in zip(a, b))
    return a == b


# --- Main fuzz entry point ---


def fuzz_encode(input_bytes: bytes):
    """Main encoding fuzzer entry point called from C."""
    if len(input_bytes) < 5:
        return

    bc = ByteConsumer(input_bytes)
    rng = make_rng(bc)

    obj, has_surrogate, has_tuple, has_ndarray, has_subclass = gen_object(bc, rng)

    # test dumps (str output)
    try:
        encoded_str = ssrjson.dumps(obj)
    except MemoryError:
        return
    except Exception as e:
        raise AssertionError(
            f"dumps raised unexpected exception: {type(e).__name__}: {e}"
        ) from e

    # verify round-trip via loads (no indent)
    try:
        decoded = ssrjson.loads(encoded_str)
    except MemoryError:
        return
    except Exception as e:
        raise AssertionError(
            f"loads failed on dumps output: {type(e).__name__}: {e}\n"
            f"encoded_str (repr): {encoded_str!r}"
        ) from e

    normalized_orig = normalize_for_comparison(obj)
    if not values_equal(normalized_orig, decoded):
        raise AssertionError(
            f"Round-trip mismatch!\n"
            f"original (normalized): {normalized_orig!r}\n"
            f"decoded: {decoded!r}"
        )

    # test dumps with indent and round-trip
    for indent in (2, 4):
        try:
            encoded_indent = ssrjson.dumps(obj, indent=indent)
        except MemoryError:
            return
        except Exception as e:
            raise AssertionError(
                f"dumps(indent={indent}) raised unexpected exception: {type(e).__name__}: {e}"
            ) from e

        try:
            decoded_indent = ssrjson.loads(encoded_indent)
        except MemoryError:
            return
        except Exception as e:
            raise AssertionError(
                f"loads failed on dumps(indent={indent}) output: {type(e).__name__}: {e}"
            ) from e

        if not values_equal(normalized_orig, decoded_indent):
            raise AssertionError(
                f"Indent round-trip mismatch (indent={indent})!\n"
                f"original (normalized): {normalized_orig!r}\n"
                f"decoded: {decoded_indent!r}"
            )

    # test dumps_to_bytes: is_write_cache=False for all indents first
    dtb_nocache = {}  # indent -> bytes result
    dtb_ok = {}  # indent -> bool (False if ValueError from surrogates)
    for indent in (None, 2, 4):
        try:
            result = ssrjson.dumps_to_bytes(obj, indent=indent, is_write_cache=False)
            dtb_nocache[indent] = result
            dtb_ok[indent] = True
        except MemoryError:
            return
        except ValueError:
            if has_surrogate:
                dtb_ok[indent] = False
            else:
                raise
        except Exception as e:
            raise AssertionError(
                f"dumps_to_bytes(indent={indent}, is_write_cache=False) raised "
                f"unexpected exception: {type(e).__name__}: {e}"
            ) from e

    # test dumps_to_bytes: is_write_cache=True, compare with nocache results
    for indent in (None, 2, 4):
        try:
            result = ssrjson.dumps_to_bytes(obj, indent=indent, is_write_cache=True)
        except MemoryError:
            return
        except ValueError:
            if has_surrogate:
                if dtb_ok.get(indent, False):
                    raise AssertionError(
                        f"dumps_to_bytes(indent={indent}, is_write_cache=True) raised "
                        f"ValueError but is_write_cache=False succeeded"
                    )
                continue
            else:
                raise
        except Exception as e:
            raise AssertionError(
                f"dumps_to_bytes(indent={indent}, is_write_cache=True) raised "
                f"unexpected exception: {type(e).__name__}: {e}"
            ) from e

        if not dtb_ok.get(indent, False):
            raise AssertionError(
                f"dumps_to_bytes(indent={indent}, is_write_cache=True) succeeded "
                f"but is_write_cache=False raised ValueError"
            )

        if result != dtb_nocache[indent]:
            raise AssertionError(
                f"dumps_to_bytes(indent={indent}) mismatch between "
                f"is_write_cache=False and True!\n"
                f"is_write_cache=False: {dtb_nocache[indent]!r}\n"
                f"is_write_cache=True: {result!r}"
            )

    # verify round-trip for dumps_to_bytes results (once per indent)
    for indent, encoded_bytes in dtb_nocache.items():
        try:
            decoded_bytes = ssrjson.loads(encoded_bytes)
        except MemoryError:
            return
        except Exception as e:
            raise AssertionError(
                f"loads failed on dumps_to_bytes(indent={indent}) output: "
                f"{type(e).__name__}: {e}\n"
                f"encoded_bytes (repr): {encoded_bytes!r}"
            ) from e

        if not values_equal(normalized_orig, decoded_bytes):
            raise AssertionError(
                f"Bytes round-trip mismatch (indent={indent})!\n"
                f"original (normalized): {normalized_orig!r}\n"
                f"decoded: {decoded_bytes!r}"
            )
