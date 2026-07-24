import ssrjson

try:
    import numpy as np

    HAS_NUMPY = True
    ssrjson.setup_numpy_types(np)
except ImportError:
    HAS_NUMPY = False


# --- Subclass types ---
#
# Object generation now happens in C (src/ctests/encode_fuzz_gen.c). The C side
# imports these subclass types from this module to build subclassed objects.


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


# --- Verification entry point (called from C) ---


def verify_encode(obj, has_surrogate: bool):
    """Verify ssrjson encode/decode round-trips for a C-generated object.

    `obj` is the object built by the C generator; `has_surrogate` is True iff it
    contains a lone surrogate (which makes UTF-8 / bytes encoding raise
    ValueError).
    """
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
