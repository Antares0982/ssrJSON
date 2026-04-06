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

import types as _types

import pytest
import ssrjson

# Check if numpy is available
try:
    import numpy as np

    HAS_NUMPY = True
except ImportError:
    HAS_NUMPY = False

# Attribute names that setup_numpy_types requires, in order.
# np.float64 is omitted: it is a subclass of Python float and handled
# by the standard T_Float path, so setup_numpy_types does not register it.
_NUMPY_ATTR_NAMES = [
    "ndarray",
    "float32",
    "int64",
    "int32",
    "uint64",
    "uint32",
    "bool_",
    "float16",
    "int16",
    "int8",
    "uint16",
    "uint8",
]


def _make_fake_numpy():
    """Return a fake numpy module with all required attrs as fresh Python types."""
    mod = _types.ModuleType("fake_numpy")
    for name in _NUMPY_ATTR_NAMES:
        setattr(mod, name, type(f"Fake_{name}", (), {}))
    return mod


# ---------------------------------------------------------------------------
# Situation 1: setup_numpy_types call fails
#
# These tests are intentionally placed FIRST so that when pytest-random-order
# is disabled, they run before any real numpy setup.  At that point numpy
# types are unregistered; the tests verify a failed call leaves that state
# intact.
# ---------------------------------------------------------------------------


class TestSetupNumpyTypesFailure:
    def test_non_module_raises_type_error(self):
        """Passing a non-module argument raises TypeError."""
        with pytest.raises(TypeError):
            ssrjson.setup_numpy_types(42)
        with pytest.raises(TypeError):
            ssrjson.setup_numpy_types("not a module")

    def test_incomplete_module_raises_attribute_error(self):
        """A module missing required attributes raises AttributeError mid-way."""
        # Provide only the first two attrs; 'int64' (3rd) is absent.
        fake = _types.ModuleType("fake_numpy_incomplete")
        fake.ndarray = object
        fake.float32 = object

        with pytest.raises(AttributeError):
            ssrjson.setup_numpy_types(fake)

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_incomplete_setup_preserves_state(self):
        """
        After a failed setup call _NumpyTypes must be unchanged.

        When run first (sequential order) numpy types have never been
        registered, so dumps of a real numpy scalar must still raise.
        When run later (random order) the pre-call state — whatever it is —
        must be preserved.

        np.int32 is used as the probe because np.float64 is a Python float
        subclass and always serializes successfully regardless of registration.
        """
        # Snapshot current behaviour before the failing call.
        try:
            result_before = ssrjson.dumps(np.int32(1))
            setup_done_before = True
        except ssrjson.JSONEncodeError:
            setup_done_before = False

        # Trigger partial failure (missing 'int64' and onwards).
        fake = _types.ModuleType("fake_numpy_incomplete")
        fake.ndarray = object
        fake.float32 = object

        with pytest.raises(AttributeError):
            ssrjson.setup_numpy_types(fake)

        # State must match what it was before the failed call.
        if setup_done_before:
            assert ssrjson.dumps(np.int32(1)) == result_before
        else:
            with pytest.raises(ssrjson.JSONEncodeError):
                ssrjson.dumps(np.int32(1))


# ---------------------------------------------------------------------------
# Situation 2: setup_numpy_types succeeds with a fake module
#
# The fake module has all required attributes, but they are plain Python
# types rather than real numpy types.  After setup:
#   - Real numpy objects are no longer recognised → JSONEncodeError.
#   - An instance of the fake ndarray type lacks __array_struct__ and
#     therefore also raises an error rather than crashing.
# ---------------------------------------------------------------------------


class TestSetupNumpyTypesFakeModule:
    @pytest.fixture(autouse=True)
    def restore_numpy(self):
        """Restore real numpy types after every test in this class."""
        yield
        if HAS_NUMPY:
            ssrjson.setup_numpy_types(np)

    def test_fake_module_setup_succeeds(self):
        """setup_numpy_types with all required attributes must not raise."""
        ssrjson.setup_numpy_types(_make_fake_numpy())

    def test_repeated_fake_setup_does_not_crash(self):
        """Calling setup twice with different fake modules must not crash or leak."""
        ssrjson.setup_numpy_types(_make_fake_numpy())
        ssrjson.setup_numpy_types(_make_fake_numpy())

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_dumps_real_numpy_scalars_raise_after_fake_setup(self):
        """
        After registering fake types, real numpy scalars have unrecognised
        types (T_Unknown) → dumps raises JSONEncodeError without crashing.

        Note: np.float64 (subclass of float) and np.bool_ (subclass of
        bool/int) are caught by standard Python type checks before the
        numpy path, so they serialize successfully even with fake types.
        """
        ssrjson.setup_numpy_types(_make_fake_numpy())

        # Separate numpy types into those that are subclasses of Python
        # builtins (handled by standard type checks, always serializable)
        # and those that rely solely on numpy type registration.
        must_fail = []
        must_pass = []
        numpy_scalars = [
            (np.float64, 1.0, float),
            (np.float32, 1.0, float),
            (np.float16, 1.0, float),
            (np.int64, 1, int),
            (np.int32, 1, int),
            (np.int16, 1, int),
            (np.int8, 1, int),
            (np.uint64, 1, int),
            (np.uint32, 1, int),
            (np.uint16, 1, int),
            (np.uint8, 1, int),
            (np.bool_, True, (bool, int)),
        ]
        for dtype, val, builtin in numpy_scalars:
            if issubclass(dtype, builtin):
                must_pass.append(dtype(val))
            else:
                must_fail.append(dtype(val))

        for val in must_fail:
            with pytest.raises(ssrjson.JSONEncodeError):
                ssrjson.dumps(val)

        for val in must_pass:
            ssrjson.dumps(val)  # must not raise

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_dumps_real_numpy_array_raises_after_fake_setup(self):
        """
        After registering a fake ndarray type, real numpy arrays become
        T_Unknown → dumps raises JSONEncodeError without crashing.
        """
        ssrjson.setup_numpy_types(_make_fake_numpy())
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps(np.array([1, 2, 3]))

    def test_dumps_fake_ndarray_instance_raises_not_crashes(self):
        """
        An instance of the fake ndarray class is recognised as T_NumpyArray,
        but it has no __array_struct__ attribute.  The encoder must raise an
        error rather than crashing.
        """
        fake = _make_fake_numpy()
        ssrjson.setup_numpy_types(fake)
        fake_arr = fake.ndarray()
        with pytest.raises(Exception):
            ssrjson.dumps(fake_arr)


class TestNumpy:
    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_setup_numpy_types(self):
        """
        Test setup_numpy_types function
        """
        ssrjson.setup_numpy_types(np)

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_float16(self):
        """
        Test numpy.float16
        """
        ssrjson.setup_numpy_types(np)
        val = np.float16(1.5)
        result = ssrjson.dumps(val)
        assert result == "1.5"
        result_bytes = ssrjson.dumps_to_bytes(val)
        assert result_bytes == b"1.5"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_float32(self):
        """
        Test numpy.float32
        """
        ssrjson.setup_numpy_types(np)
        val = np.float32(3.14)
        result = ssrjson.dumps(val)
        # Check that it's a valid float representation
        assert float(result) == pytest.approx(3.14, rel=1e-6)

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_float64(self):
        """
        Test numpy.float64
        """
        ssrjson.setup_numpy_types(np)
        val = np.float64(3.141592653589793)
        result = ssrjson.dumps(val)
        assert float(result) == 3.141592653589793

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_int8(self):
        """
        Test numpy.int8
        """
        ssrjson.setup_numpy_types(np)
        val = np.int8(42)
        result = ssrjson.dumps(val)
        assert result == "42"
        result_bytes = ssrjson.dumps_to_bytes(val)
        assert result_bytes == b"42"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_int16(self):
        """
        Test numpy.int16
        """
        ssrjson.setup_numpy_types(np)
        val = np.int16(-1000)
        result = ssrjson.dumps(val)
        assert result == "-1000"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_int32(self):
        """
        Test numpy.int32
        """
        ssrjson.setup_numpy_types(np)
        val = np.int32(123456)
        result = ssrjson.dumps(val)
        assert result == "123456"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_int64(self):
        """
        Test numpy.int64
        """
        ssrjson.setup_numpy_types(np)
        val = np.int64(9223372036854775807)
        result = ssrjson.dumps(val)
        assert result == "9223372036854775807"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_uint8(self):
        """
        Test numpy.uint8
        """
        ssrjson.setup_numpy_types(np)
        val = np.uint8(255)
        result = ssrjson.dumps(val)
        assert result == "255"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_uint16(self):
        """
        Test numpy.uint16
        """
        ssrjson.setup_numpy_types(np)
        val = np.uint16(65535)
        result = ssrjson.dumps(val)
        assert result == "65535"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_uint32(self):
        """
        Test numpy.uint32
        """
        ssrjson.setup_numpy_types(np)
        val = np.uint32(4294967295)
        result = ssrjson.dumps(val)
        assert result == "4294967295"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_uint64(self):
        """
        Test numpy.uint64
        """
        ssrjson.setup_numpy_types(np)
        val = np.uint64(18446744073709551615)
        result = ssrjson.dumps(val)
        assert result == "18446744073709551615"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_bool_true(self):
        """
        Test numpy.bool_ True
        """
        ssrjson.setup_numpy_types(np)
        val = np.bool_(True)
        result = ssrjson.dumps(val)
        assert result == "true"
        result_bytes = ssrjson.dumps_to_bytes(val)
        assert result_bytes == b"true"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_bool_false(self):
        """
        Test numpy.bool_ False
        """
        ssrjson.setup_numpy_types(np)
        val = np.bool_(False)
        result = ssrjson.dumps(val)
        assert result == "false"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_array_1d(self):
        """
        Test 1D numpy array
        """
        ssrjson.setup_numpy_types(np)
        val = np.array([1, 2, 3, 4, 5])
        result = ssrjson.dumps(val)
        assert result == "[1,2,3,4,5]"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_array_2d(self):
        """
        Test 2D numpy array
        """
        ssrjson.setup_numpy_types(np)
        val = np.array([[1, 2], [3, 4]])
        result = ssrjson.dumps(val)
        assert result == "[[1,2],[3,4]]"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_array_float(self):
        """
        Test numpy array with floats
        """
        ssrjson.setup_numpy_types(np)
        val = np.array([1.1, 2.2, 3.3])
        result = ssrjson.dumps(val)
        # Parse and check values
        loaded = ssrjson.loads(result)
        assert len(loaded) == 3
        assert loaded[0] == pytest.approx(1.1)
        assert loaded[1] == pytest.approx(2.2)
        assert loaded[2] == pytest.approx(3.3)

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_array_empty(self):
        """
        Test empty numpy array
        """
        ssrjson.setup_numpy_types(np)
        val = np.array([])
        result = ssrjson.dumps(val)
        assert result == "[]"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_in_dict(self):
        """
        Test numpy types in dict
        """
        ssrjson.setup_numpy_types(np)
        val = {
            "int64": np.int64(42),
            "float64": np.float64(3.14),
            "bool": np.bool_(True),
            "array": np.array([1, 2, 3]),
        }
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded["int64"] == 42
        assert loaded["float64"] == pytest.approx(3.14)
        assert loaded["bool"] is True
        assert loaded["array"] == [1, 2, 3]

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_in_list(self):
        """
        Test numpy types in list
        """
        ssrjson.setup_numpy_types(np)
        val = [np.int64(42), np.float64(3.14), np.bool_(False), np.array([1, 2])]
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded[0] == 42
        assert loaded[1] == pytest.approx(3.14)
        assert loaded[2] is False
        assert loaded[3] == [1, 2]

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_float32(self):
        ssrjson.setup_numpy_types(np)
        val = np.float32(3.14)
        result = ssrjson.dumps_to_bytes(val)
        assert float(result) == pytest.approx(3.14, rel=1e-6)

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_float64(self):
        ssrjson.setup_numpy_types(np)
        val = np.float64(3.141592653589793)
        result = ssrjson.dumps_to_bytes(val)
        assert float(result) == 3.141592653589793

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_int16(self):
        ssrjson.setup_numpy_types(np)
        assert ssrjson.dumps_to_bytes(np.int16(-1000)) == b"-1000"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_int32(self):
        ssrjson.setup_numpy_types(np)
        assert ssrjson.dumps_to_bytes(np.int32(123456)) == b"123456"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_int64(self):
        ssrjson.setup_numpy_types(np)
        assert (
            ssrjson.dumps_to_bytes(np.int64(9223372036854775807))
            == b"9223372036854775807"
        )

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_uint8(self):
        ssrjson.setup_numpy_types(np)
        assert ssrjson.dumps_to_bytes(np.uint8(255)) == b"255"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_uint16(self):
        ssrjson.setup_numpy_types(np)
        assert ssrjson.dumps_to_bytes(np.uint16(65535)) == b"65535"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_uint32(self):
        ssrjson.setup_numpy_types(np)
        assert ssrjson.dumps_to_bytes(np.uint32(4294967295)) == b"4294967295"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_uint64(self):
        ssrjson.setup_numpy_types(np)
        assert (
            ssrjson.dumps_to_bytes(np.uint64(18446744073709551615))
            == b"18446744073709551615"
        )

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_bool_false(self):
        ssrjson.setup_numpy_types(np)
        assert ssrjson.dumps_to_bytes(np.bool_(False)) == b"false"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_array_1d(self):
        ssrjson.setup_numpy_types(np)
        assert ssrjson.dumps_to_bytes(np.array([1, 2, 3, 4, 5])) == b"[1,2,3,4,5]"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_array_2d(self):
        ssrjson.setup_numpy_types(np)
        assert ssrjson.dumps_to_bytes(np.array([[1, 2], [3, 4]])) == b"[[1,2],[3,4]]"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_array_empty(self):
        ssrjson.setup_numpy_types(np)
        assert ssrjson.dumps_to_bytes(np.array([])) == b"[]"

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_in_dict(self):
        ssrjson.setup_numpy_types(np)
        val = {
            "int8": np.int8(-1),
            "int64": np.int64(42),
            "uint32": np.uint32(100),
            "float16": np.float16(1.5),
            "float64": np.float64(3.14),
            "bool": np.bool_(True),
            "array": np.array([1, 2, 3]),
        }
        result = ssrjson.dumps_to_bytes(val)
        loaded = ssrjson.loads(result)
        assert loaded["int8"] == -1
        assert loaded["int64"] == 42
        assert loaded["uint32"] == 100
        assert loaded["float16"] == 1.5
        assert loaded["float64"] == pytest.approx(3.14)
        assert loaded["bool"] is True
        assert loaded["array"] == [1, 2, 3]

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_in_list(self):
        ssrjson.setup_numpy_types(np)
        val = [
            np.int8(-1),
            np.int64(42),
            np.uint32(100),
            np.float16(1.5),
            np.float64(3.14),
            np.bool_(False),
            np.array([1, 2]),
        ]
        result = ssrjson.dumps_to_bytes(val)
        loaded = ssrjson.loads(result)
        assert loaded[0] == -1
        assert loaded[1] == 42
        assert loaded[2] == 100
        assert loaded[3] == 1.5
        assert loaded[4] == pytest.approx(3.14)
        assert loaded[5] is False
        assert loaded[6] == [1, 2]

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_in_dict_all_int_types(self):
        ssrjson.setup_numpy_types(np)
        val = {
            "int8": np.int8(-128),
            "int16": np.int16(-32768),
            "int32": np.int32(-2147483648),
            "int64": np.int64(-9223372036854775808),
            "uint8": np.uint8(255),
            "uint16": np.uint16(65535),
            "uint32": np.uint32(4294967295),
            "uint64": np.uint64(18446744073709551615),
        }
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded["int8"] == -128
        assert loaded["int16"] == -32768
        assert loaded["int32"] == -2147483648
        assert loaded["int64"] == -9223372036854775808
        assert loaded["uint8"] == 255
        assert loaded["uint16"] == 65535
        assert loaded["uint32"] == 4294967295
        assert loaded["uint64"] == 18446744073709551615

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_in_list_all_int_types(self):
        ssrjson.setup_numpy_types(np)
        val = [
            np.int8(-128),
            np.int16(-32768),
            np.int32(-2147483648),
            np.int64(-9223372036854775808),
            np.uint8(255),
            np.uint16(65535),
            np.uint32(4294967295),
            np.uint64(18446744073709551615),
        ]
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded == [
            -128,
            -32768,
            -2147483648,
            -9223372036854775808,
            255,
            65535,
            4294967295,
            18446744073709551615,
        ]

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_in_dict_all_float_types(self):
        ssrjson.setup_numpy_types(np)
        val = {
            "float16": np.float16(1.5),
            "float32": np.float32(3.14),
            "float64": np.float64(2.718281828459045),
        }
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded["float16"] == 1.5
        assert loaded["float32"] == pytest.approx(3.14, rel=1e-6)
        assert loaded["float64"] == pytest.approx(2.718281828459045)

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_in_list_all_float_types(self):
        ssrjson.setup_numpy_types(np)
        val = [np.float16(1.5), np.float32(3.14), np.float64(2.718281828459045)]
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded[0] == 1.5
        assert loaded[1] == pytest.approx(3.14, rel=1e-6)
        assert loaded[2] == pytest.approx(2.718281828459045)

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_in_dict_all_int_types(self):
        ssrjson.setup_numpy_types(np)
        val = {
            "int8": np.int8(-128),
            "int16": np.int16(-32768),
            "int32": np.int32(-2147483648),
            "int64": np.int64(-9223372036854775808),
            "uint8": np.uint8(255),
            "uint16": np.uint16(65535),
            "uint32": np.uint32(4294967295),
            "uint64": np.uint64(18446744073709551615),
        }
        result = ssrjson.dumps_to_bytes(val)
        loaded = ssrjson.loads(result)
        assert loaded["int8"] == -128
        assert loaded["int16"] == -32768
        assert loaded["int32"] == -2147483648
        assert loaded["int64"] == -9223372036854775808
        assert loaded["uint8"] == 255
        assert loaded["uint16"] == 65535
        assert loaded["uint32"] == 4294967295
        assert loaded["uint64"] == 18446744073709551615

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_in_list_all_int_types(self):
        ssrjson.setup_numpy_types(np)
        val = [
            np.int8(-128),
            np.int16(-32768),
            np.int32(-2147483648),
            np.int64(-9223372036854775808),
            np.uint8(255),
            np.uint16(65535),
            np.uint32(4294967295),
            np.uint64(18446744073709551615),
        ]
        result = ssrjson.dumps_to_bytes(val)
        loaded = ssrjson.loads(result)
        assert loaded == [
            -128,
            -32768,
            -2147483648,
            -9223372036854775808,
            255,
            65535,
            4294967295,
            18446744073709551615,
        ]

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_in_dict_all_float_types(self):
        ssrjson.setup_numpy_types(np)
        val = {
            "float16": np.float16(1.5),
            "float32": np.float32(3.14),
            "float64": np.float64(2.718281828459045),
        }
        result = ssrjson.dumps_to_bytes(val)
        loaded = ssrjson.loads(result)
        assert loaded["float16"] == 1.5
        assert loaded["float32"] == pytest.approx(3.14, rel=1e-6)
        assert loaded["float64"] == pytest.approx(2.718281828459045)

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_dumps_to_bytes_in_list_all_float_types(self):
        ssrjson.setup_numpy_types(np)
        val = [np.float16(1.5), np.float32(3.14), np.float64(2.718281828459045)]
        result = ssrjson.dumps_to_bytes(val)
        loaded = ssrjson.loads(result)
        assert loaded[0] == 1.5
        assert loaded[1] == pytest.approx(3.14, rel=1e-6)
        assert loaded[2] == pytest.approx(2.718281828459045)

    @pytest.mark.skipif(not HAS_NUMPY, reason="numpy not installed")
    def test_numpy_mixed_with_python(self):
        """
        Test mixing numpy and Python types
        """
        ssrjson.setup_numpy_types(np)
        val = {
            "python_int": 42,
            "numpy_int": np.int64(42),
            "python_float": 3.14,
            "numpy_float": np.float64(3.14),
            "python_list": [1, 2, 3],
            "numpy_array": np.array([1, 2, 3]),
        }
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded["python_int"] == 42
        assert loaded["numpy_int"] == 42
        assert loaded["python_float"] == pytest.approx(3.14)
        assert loaded["numpy_float"] == pytest.approx(3.14)
        assert loaded["python_list"] == [1, 2, 3]
        assert loaded["numpy_array"] == [1, 2, 3]


class TestNumpyIndent:
    """Test numpy types with indent=2 and indent=4.

    Core invariant: dumps(ndarray, indent=N) == dumps(ndarray.tolist(), indent=N)
    """

    @pytest.fixture(autouse=True)
    def setup_numpy(self):
        if not HAS_NUMPY:
            pytest.skip("numpy not installed")
        ssrjson.setup_numpy_types(np)

    # --- 1D arrays ---

    def test_1d_int_indent2(self):
        arr = np.array([1, 2, 3, 4, 5])
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=2) == expected.encode("utf-8")

    def test_1d_int_indent4(self):
        arr = np.array([1, 2, 3, 4, 5])
        expected = ssrjson.dumps(arr.tolist(), indent=4)
        assert ssrjson.dumps(arr, indent=4) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=4) == expected.encode("utf-8")

    def test_1d_float64_indent2(self):
        arr = np.array([1.1, 2.2, 3.3], dtype=np.float64)
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=2) == expected.encode("utf-8")

    def test_1d_float32_indent2(self):
        arr = np.array([1.5, 2.5, 3.5], dtype=np.float32)
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected

    def test_1d_bool_indent2(self):
        arr = np.array([True, False, True, False], dtype=np.bool_)
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=2) == expected.encode("utf-8")

    def test_1d_bool_indent4(self):
        arr = np.array([True, False, True], dtype=np.bool_)
        expected = ssrjson.dumps(arr.tolist(), indent=4)
        assert ssrjson.dumps(arr, indent=4) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=4) == expected.encode("utf-8")

    # --- 2D arrays ---

    def test_2d_int_indent2(self):
        arr = np.array([[1, 2], [3, 4]])
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=2) == expected.encode("utf-8")

    def test_2d_int_indent4(self):
        arr = np.array([[1, 2], [3, 4]])
        expected = ssrjson.dumps(arr.tolist(), indent=4)
        assert ssrjson.dumps(arr, indent=4) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=4) == expected.encode("utf-8")

    def test_2d_float64_indent2(self):
        arr = np.array([[1.1, 2.2], [3.3, 4.4]], dtype=np.float64)
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected

    def test_2d_large_indent2(self):
        arr = np.array([[i + j * 10 for i in range(5)] for j in range(4)])
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=2) == expected.encode("utf-8")

    # --- 3D arrays ---

    def test_3d_int_indent2(self):
        arr = np.array([[[1, 2], [3, 4]], [[5, 6], [7, 8]]])
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=2) == expected.encode("utf-8")

    def test_3d_int_indent4(self):
        arr = np.array([[[1, 2], [3, 4]], [[5, 6], [7, 8]]])
        expected = ssrjson.dumps(arr.tolist(), indent=4)
        assert ssrjson.dumps(arr, indent=4) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=4) == expected.encode("utf-8")

    def test_3d_float_indent2(self):
        arr = np.array([[[0.1, 0.2], [0.3, 0.4]], [[0.5, 0.6], [0.7, 0.8]]])
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected

    # --- 4D array ---

    def test_4d_int_indent2(self):
        arr = np.arange(24).reshape(2, 3, 2, 2)
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=2) == expected.encode("utf-8")

    # --- Empty / single element arrays ---

    def test_empty_indent2(self):
        arr = np.array([])
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=2) == expected.encode("utf-8")

    def test_single_element_indent2(self):
        arr = np.array([42])
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected
        assert ssrjson.dumps_to_bytes(arr, indent=2) == expected.encode("utf-8")

    def test_single_element_indent4(self):
        arr = np.array([42])
        expected = ssrjson.dumps(arr.tolist(), indent=4)
        assert ssrjson.dumps(arr, indent=4) == expected

    # --- Various integer dtypes with indent ---

    def test_1d_int8_indent2(self):
        arr = np.array([-128, 0, 127], dtype=np.int8)
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected

    def test_1d_uint64_indent2(self):
        arr = np.array([0, 1, 18446744073709551615], dtype=np.uint64)
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected

    def test_1d_int64_indent4(self):
        arr = np.array([-9223372036854775808, 0, 9223372036854775807], dtype=np.int64)
        expected = ssrjson.dumps(arr.tolist(), indent=4)
        assert ssrjson.dumps(arr, indent=4) == expected

    def test_2d_mixed_int_types_indent2(self):
        for dtype in [
            np.int8,
            np.int16,
            np.int32,
            np.int64,
            np.uint8,
            np.uint16,
            np.uint32,
            np.uint64,
        ]:
            arr = np.array([[1, 2], [3, 4]], dtype=dtype)
            expected = ssrjson.dumps(arr.tolist(), indent=2)
            assert ssrjson.dumps(arr, indent=2) == expected, f"Failed for dtype={dtype}"

    # --- Float dtypes with indent ---

    def test_1d_float16_indent2(self):
        arr = np.array([1.0, 1.5, 2.0], dtype=np.float16)
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected

    def test_2d_float32_indent4(self):
        arr = np.array([[1.5, 2.5], [3.5, 4.5]], dtype=np.float32)
        expected = ssrjson.dumps(arr.tolist(), indent=4)
        assert ssrjson.dumps(arr, indent=4) == expected

    # --- ndarray inside dict with indent ---

    def test_dict_with_array_indent2(self):
        val = {"data": np.array([1, 2, 3]), "name": "test"}
        expected = ssrjson.dumps({"data": [1, 2, 3], "name": "test"}, indent=2)
        assert ssrjson.dumps(val, indent=2) == expected
        assert ssrjson.dumps_to_bytes(val, indent=2) == expected.encode("utf-8")

    def test_dict_with_array_indent4(self):
        val = {"data": np.array([1, 2, 3]), "name": "test"}
        expected = ssrjson.dumps({"data": [1, 2, 3], "name": "test"}, indent=4)
        assert ssrjson.dumps(val, indent=4) == expected
        assert ssrjson.dumps_to_bytes(val, indent=4) == expected.encode("utf-8")

    def test_dict_with_2d_array_indent2(self):
        val = {"matrix": np.array([[1, 2], [3, 4]]), "label": "m"}
        expected = ssrjson.dumps({"matrix": [[1, 2], [3, 4]], "label": "m"}, indent=2)
        assert ssrjson.dumps(val, indent=2) == expected

    def test_dict_with_2d_array_indent4(self):
        val = {"matrix": np.array([[1, 2], [3, 4]])}
        expected = ssrjson.dumps({"matrix": [[1, 2], [3, 4]]}, indent=4)
        assert ssrjson.dumps(val, indent=4) == expected

    def test_dict_all_numpy_types_indent2(self):
        val = {
            "int8": np.int8(-1),
            "int64": np.int64(42),
            "uint32": np.uint32(100),
            "float16": np.float16(1.5),
            "float64": np.float64(3.14),
            "bool": np.bool_(True),
            "array": np.array([1, 2, 3]),
        }
        expected_val = {
            "int8": -1,
            "int64": 42,
            "uint32": 100,
            "float16": 1.5,
            "float64": 3.14,
            "bool": True,
            "array": [1, 2, 3],
        }
        expected = ssrjson.dumps(expected_val, indent=2)
        assert ssrjson.dumps(val, indent=2) == expected
        assert ssrjson.dumps_to_bytes(val, indent=2) == expected.encode("utf-8")

    def test_dict_all_numpy_types_indent4(self):
        val = {
            "int8": np.int8(-1),
            "int64": np.int64(42),
            "uint32": np.uint32(100),
            "float16": np.float16(1.5),
            "float64": np.float64(3.14),
            "bool": np.bool_(True),
            "array": np.array([1, 2, 3]),
        }
        expected_val = {
            "int8": -1,
            "int64": 42,
            "uint32": 100,
            "float16": 1.5,
            "float64": 3.14,
            "bool": True,
            "array": [1, 2, 3],
        }
        expected = ssrjson.dumps(expected_val, indent=4)
        assert ssrjson.dumps(val, indent=4) == expected

    # --- ndarray inside list with indent ---

    def test_list_with_arrays_indent2(self):
        val = [np.array([1, 2]), np.array([3, 4])]
        expected = ssrjson.dumps([[1, 2], [3, 4]], indent=2)
        assert ssrjson.dumps(val, indent=2) == expected
        assert ssrjson.dumps_to_bytes(val, indent=2) == expected.encode("utf-8")

    def test_list_with_arrays_indent4(self):
        val = [np.array([1, 2]), np.array([3, 4])]
        expected = ssrjson.dumps([[1, 2], [3, 4]], indent=4)
        assert ssrjson.dumps(val, indent=4) == expected

    def test_list_mixed_numpy_python_indent2(self):
        val = [
            np.int64(42),
            np.float64(3.14),
            np.bool_(False),
            np.array([1, 2]),
            "hello",
            None,
            True,
            [1, 2, 3],
        ]
        expected_val = [42, 3.14, False, [1, 2], "hello", None, True, [1, 2, 3]]
        expected = ssrjson.dumps(expected_val, indent=2)
        assert ssrjson.dumps(val, indent=2) == expected

    def test_list_mixed_numpy_python_indent4(self):
        val = [
            np.int64(42),
            np.float64(3.14),
            np.bool_(False),
            np.array([1, 2]),
            "hello",
            None,
            True,
        ]
        expected_val = [42, 3.14, False, [1, 2], "hello", None, True]
        expected = ssrjson.dumps(expected_val, indent=4)
        assert ssrjson.dumps(val, indent=4) == expected

    # --- Nested structures ---

    def test_nested_dict_with_arrays_indent2(self):
        val = {"level1": {"level2": {"data": np.array([1, 2, 3])}}}
        expected = ssrjson.dumps({"level1": {"level2": {"data": [1, 2, 3]}}}, indent=2)
        assert ssrjson.dumps(val, indent=2) == expected

    def test_nested_dict_with_arrays_indent4(self):
        val = {"level1": {"level2": {"data": np.array([1, 2, 3])}}}
        expected = ssrjson.dumps({"level1": {"level2": {"data": [1, 2, 3]}}}, indent=4)
        assert ssrjson.dumps(val, indent=4) == expected

    def test_list_of_dicts_with_arrays_indent2(self):
        val = [
            {"x": np.array([1, 2]), "y": np.int32(10)},
            {"x": np.array([3, 4]), "y": np.int32(20)},
        ]
        expected = ssrjson.dumps(
            [
                {"x": [1, 2], "y": 10},
                {"x": [3, 4], "y": 20},
            ],
            indent=2,
        )
        assert ssrjson.dumps(val, indent=2) == expected
        assert ssrjson.dumps_to_bytes(val, indent=2) == expected.encode("utf-8")

    def test_dict_with_list_of_arrays_indent2(self):
        val = {"rows": [np.array([1, 2, 3]), np.array([4, 5, 6])]}
        expected = ssrjson.dumps({"rows": [[1, 2, 3], [4, 5, 6]]}, indent=2)
        assert ssrjson.dumps(val, indent=2) == expected

    def test_deeply_nested_indent4(self):
        val = {"a": [{"b": np.array([[1, 2], [3, 4]])}]}
        expected = ssrjson.dumps({"a": [{"b": [[1, 2], [3, 4]]}]}, indent=4)
        assert ssrjson.dumps(val, indent=4) == expected
        assert ssrjson.dumps_to_bytes(val, indent=4) == expected.encode("utf-8")

    # --- Empty containers with indent ---

    def test_empty_array_in_dict_indent2(self):
        val = {"data": np.array([])}
        expected = ssrjson.dumps({"data": []}, indent=2)
        assert ssrjson.dumps(val, indent=2) == expected

    def test_empty_array_in_list_indent2(self):
        val = [np.array([]), np.array([1])]
        expected = ssrjson.dumps([[], [1]], indent=2)
        assert ssrjson.dumps(val, indent=2) == expected

    def test_empty_2d_indent2(self):
        arr = np.array([[], []], dtype=np.int64)
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected

    # --- Large arrays ---

    def test_large_1d_indent2(self):
        arr = np.arange(100)
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected

    def test_large_2d_indent2(self):
        arr = np.arange(200).reshape(20, 10)
        expected = ssrjson.dumps(arr.tolist(), indent=2)
        assert ssrjson.dumps(arr, indent=2) == expected

    def test_large_2d_indent4(self):
        arr = np.arange(200).reshape(20, 10)
        expected = ssrjson.dumps(arr.tolist(), indent=4)
        assert ssrjson.dumps(arr, indent=4) == expected

    # --- Scalar numpy values with indent (in containers) ---

    def test_numpy_scalars_in_list_indent2(self):
        for dtype in [
            np.int8,
            np.int16,
            np.int32,
            np.int64,
            np.uint8,
            np.uint16,
            np.uint32,
            np.uint64,
        ]:
            val = [dtype(1), dtype(2), dtype(3)]
            expected = ssrjson.dumps([1, 2, 3], indent=2)
            assert ssrjson.dumps(val, indent=2) == expected, f"Failed for dtype={dtype}"

    def test_numpy_float_scalars_in_list_indent2(self):
        val = [np.float64(1.5), np.float64(2.5)]
        expected = ssrjson.dumps([1.5, 2.5], indent=2)
        assert ssrjson.dumps(val, indent=2) == expected

    def test_numpy_bool_scalars_in_dict_indent4(self):
        val = {"a": np.bool_(True), "b": np.bool_(False)}
        expected = ssrjson.dumps({"a": True, "b": False}, indent=4)
        assert ssrjson.dumps(val, indent=4) == expected


class TestNumpyUCS:
    """Test numpy ndarray with UCS-2/UCS-4 dict keys to exercise wider encoding paths."""

    @pytest.fixture(autouse=True)
    def setup_numpy(self):
        if not HAS_NUMPY:
            pytest.skip("numpy not installed")
        ssrjson.setup_numpy_types(np)

    def test_ucs2_key_with_ndarray_1d(self):
        # U+00E9 (e-acute) forces UCS-2 internal representation
        val = {"\u00e9": np.array([1, 2, 3])}
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded["\u00e9"] == [1, 2, 3]

    def test_ucs2_key_with_ndarray_2d(self):
        val = {"\u00e9": np.array([[1, 2], [3, 4]])}
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded["\u00e9"] == [[1, 2], [3, 4]]

    def test_ucs4_key_with_ndarray_1d(self):
        # U+1F600 (emoji) forces UCS-4 internal representation
        val = {"\U0001f600": np.array([10, 20, 30])}
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded["\U0001f600"] == [10, 20, 30]

    def test_ucs4_key_with_ndarray_2d(self):
        val = {"\U0001f600": np.array([[1, 2], [3, 4]])}
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded["\U0001f600"] == [[1, 2], [3, 4]]

    def test_ucs2_key_with_large_ndarray(self):
        # Large array to force realloc in the UCS-2 encoding path
        val = {"\u00e9": np.arange(10000, dtype=np.int64)}
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded["\u00e9"] == list(range(10000))

    def test_ucs4_key_with_large_ndarray(self):
        # Large array to force realloc in the UCS-4 encoding path
        val = {"\U0001f600": np.arange(10000, dtype=np.int64)}
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded["\U0001f600"] == list(range(10000))

    def test_ucs2_key_with_ndarray_float(self):
        val = {"\u00e9": np.array([1.1, 2.2, 3.3], dtype=np.float64)}
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert len(loaded["\u00e9"]) == 3

    def test_ucs4_key_with_ndarray_float(self):
        val = {"\U0001f600": np.array([1.1, 2.2], dtype=np.float32)}
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert len(loaded["\U0001f600"]) == 2

    def test_ucs2_key_with_ndarray_bool(self):
        val = {"\u00e9": np.array([True, False, True], dtype=np.bool_)}
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded["\u00e9"] == [True, False, True]

    def test_ucs2_key_with_ndarray_indent2(self):
        val = {"\u00e9": np.array([1, 2, 3])}
        expected = ssrjson.dumps({"\u00e9": [1, 2, 3]}, indent=2)
        assert ssrjson.dumps(val, indent=2) == expected

    def test_ucs4_key_with_ndarray_indent4(self):
        val = {"\U0001f600": np.array([[1, 2], [3, 4]])}
        expected = ssrjson.dumps({"\U0001f600": [[1, 2], [3, 4]]}, indent=4)
        assert ssrjson.dumps(val, indent=4) == expected

    def test_ucs2_key_with_large_ndarray_indent2(self):
        val = {"\u00e9": np.arange(10000, dtype=np.int32)}
        expected = ssrjson.dumps({"\u00e9": list(range(10000))}, indent=2)
        assert ssrjson.dumps(val, indent=2) == expected

    def test_ucs4_key_with_large_ndarray_indent4(self):
        val = {"\U0001f600": np.arange(10000, dtype=np.int32)}
        expected = ssrjson.dumps({"\U0001f600": list(range(10000))}, indent=4)
        assert ssrjson.dumps(val, indent=4) == expected

    def test_mixed_ucs_keys_with_ndarray(self):
        val = {
            "ascii": np.array([1, 2]),
            "\u00e9": np.array([3, 4]),
            "\U0001f600": np.array([5, 6]),
        }
        result = ssrjson.dumps(val)
        loaded = ssrjson.loads(result)
        assert loaded["ascii"] == [1, 2]
        assert loaded["\u00e9"] == [3, 4]
        assert loaded["\U0001f600"] == [5, 6]

    def test_ucs2_key_with_ndarray_dumps_to_bytes(self):
        val = {"\u00e9": np.array([1, 2, 3])}
        result = ssrjson.dumps_to_bytes(val)
        loaded = ssrjson.loads(result)
        assert loaded["\u00e9"] == [1, 2, 3]

    def test_ucs4_key_with_large_ndarray_dumps_to_bytes(self):
        val = {"\U0001f600": np.arange(10000, dtype=np.int64)}
        result = ssrjson.dumps_to_bytes(val)
        loaded = ssrjson.loads(result)
        assert loaded["\U0001f600"] == list(range(10000))


class TestNumpyEdgeCases:
    """Test edge cases: NaN/Inf, 0-d arrays, non-contiguous, Fortran-order."""

    @pytest.fixture(autouse=True)
    def setup_numpy(self):
        if not HAS_NUMPY:
            pytest.skip("numpy not installed")
        ssrjson.setup_numpy_types(np)

    # --- NaN / Inf for float scalars (outputs NaN/Infinity like Python json) ---

    def test_float64_nan(self):
        # np.float64 is a Python float subclass, follows standard float path
        assert ssrjson.dumps(np.float64(float("nan"))) == "NaN"

    def test_float64_inf(self):
        assert ssrjson.dumps(np.float64(float("inf"))) == "Infinity"

    def test_float64_neg_inf(self):
        assert ssrjson.dumps(np.float64(float("-inf"))) == "-Infinity"

    def test_float32_nan(self):
        assert ssrjson.dumps(np.float32(float("nan"))) == "NaN"

    def test_float32_inf(self):
        assert ssrjson.dumps(np.float32(float("inf"))) == "Infinity"

    def test_float16_nan(self):
        assert ssrjson.dumps(np.float16(float("nan"))) == "NaN"

    def test_float16_inf(self):
        assert ssrjson.dumps(np.float16(float("inf"))) == "Infinity"

    def test_float16_neg_inf(self):
        assert ssrjson.dumps(np.float16(float("-inf"))) == "-Infinity"

    def test_float_nan_dumps_to_bytes(self):
        assert ssrjson.dumps_to_bytes(np.float64(float("nan"))) == b"NaN"
        assert ssrjson.dumps_to_bytes(np.float32(float("nan"))) == b"NaN"
        assert ssrjson.dumps_to_bytes(np.float16(float("nan"))) == b"NaN"

    def test_float_inf_dumps_to_bytes(self):
        assert ssrjson.dumps_to_bytes(np.float64(float("inf"))) == b"Infinity"
        assert ssrjson.dumps_to_bytes(np.float32(float("inf"))) == b"Infinity"
        assert ssrjson.dumps_to_bytes(np.float16(float("inf"))) == b"Infinity"

    # --- NaN / Inf inside ndarray ---

    def test_ndarray_with_nan(self):
        result = ssrjson.dumps(np.array([1.0, float("nan"), 3.0]))
        assert "NaN" in result

    def test_ndarray_with_inf(self):
        result = ssrjson.dumps(np.array([1.0, float("inf"), 3.0]))
        assert "Infinity" in result

    def test_ndarray_with_neg_inf(self):
        result = ssrjson.dumps(np.array([1.0, float("-inf"), 3.0]))
        assert "-Infinity" in result

    def test_ndarray_float32_with_nan(self):
        result = ssrjson.dumps(np.array([1.0, float("nan")], dtype=np.float32))
        assert "NaN" in result

    # --- NaN/Inf in containers ---

    def test_float_nan_in_dict(self):
        result = ssrjson.dumps({"val": np.float64(float("nan"))})
        assert "NaN" in result

    def test_float_inf_in_list(self):
        result = ssrjson.dumps([np.float32(float("inf"))])
        assert "Infinity" in result

    # --- Zero-dimensional arrays ---

    def test_0d_array_raises(self):
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps(np.array(42))

    # --- Non-contiguous arrays ---

    def test_non_contiguous_column_slice(self):
        arr = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.int32)
        col = arr[:, 1]  # non-contiguous view
        assert not col.flags["C_CONTIGUOUS"]
        # Should either succeed with correct output or raise an error
        try:
            result = ssrjson.dumps(col)
            loaded = ssrjson.loads(result)
            assert loaded == col.tolist()
        except (ValueError, ssrjson.JSONEncodeError):
            pass  # Rejecting non-contiguous is acceptable

    def test_non_contiguous_step_slice(self):
        arr = np.arange(10, dtype=np.int64)
        sliced = arr[::2]  # [0, 2, 4, 6, 8], non-contiguous
        assert not sliced.flags["C_CONTIGUOUS"]
        try:
            result = ssrjson.dumps(sliced)
            loaded = ssrjson.loads(result)
            assert loaded == sliced.tolist()
        except (ValueError, ssrjson.JSONEncodeError):
            pass

    def test_fortran_order_2d(self):
        arr = np.asfortranarray(np.array([[1, 2], [3, 4]], dtype=np.int32))
        assert arr.flags["F_CONTIGUOUS"]
        assert not arr.flags["C_CONTIGUOUS"]
        try:
            result = ssrjson.dumps(arr)
            loaded = ssrjson.loads(result)
            assert loaded == arr.tolist()
        except (ValueError, ssrjson.JSONEncodeError):
            pass

    # --- Single-element arrays (non-indented) ---

    def test_single_element_1d(self):
        arr = np.array([42])
        result = ssrjson.dumps(arr)
        assert result == "[42]"

    def test_single_element_float(self):
        arr = np.array([3.14], dtype=np.float64)
        result = ssrjson.dumps(arr)
        loaded = ssrjson.loads(result)
        assert loaded == [pytest.approx(3.14)]

    def test_single_element_bool(self):
        arr = np.array([True], dtype=np.bool_)
        result = ssrjson.dumps(arr)
        assert result == "[true]"

    def test_single_element_dumps_to_bytes(self):
        arr = np.array([99], dtype=np.int32)
        result = ssrjson.dumps_to_bytes(arr)
        assert result == b"[99]"
