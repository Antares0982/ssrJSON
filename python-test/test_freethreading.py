import json
import os
import sys
import threading
from pathlib import Path

import pytest
import ssrjson

# pylint: disable=broad-exception-caught

IS_GIL_ENABLED = not hasattr(sys, "_is_gil_enabled") or sys._is_gil_enabled()  # pylint: disable=protected-access

SSRJSON_IS_FREETHREADING = ssrjson.get_current_features()["free_threading"]
SSRJSON_IS_LOCKFREE = ssrjson.get_current_features()["lockfree"]
SKIP_RW_RACE_TEST = IS_GIL_ENABLED or SSRJSON_IS_LOCKFREE
SKIP_RO_RACE_TEST = IS_GIL_ENABLED


class TestFreeThreading:
    @staticmethod
    def _default_threads() -> int:
        return min(8, max(2, os.cpu_count() or 2))

    @staticmethod
    def _iter_bench_sources():
        bench_dir = Path(__file__).resolve().parent.parent / "bench"
        files = sorted(bench_dir.glob("*.json"))
        assert files, f"No benchmark JSON files found under {bench_dir}"

        for path in files:
            data = path.read_bytes()
            yield path.name, data.decode("utf-8"), data

    @staticmethod
    def _run_in_threads(*, threads: int, loops: int, worker):
        barrier = threading.Barrier(threads)
        excs = []
        lock = threading.Lock()

        def wrapped_worker():
            try:
                barrier.wait()
                for _ in range(loops):
                    worker()
            except Exception as e:
                with lock:
                    excs.append(e)

        ts = [threading.Thread(target=wrapped_worker) for _ in range(threads)]
        for t in ts:
            t.start()
        for t in ts:
            t.join()

        if excs:
            raise excs[0]

    def test_abi_match(self):
        assert SSRJSON_IS_FREETHREADING == (not IS_GIL_ENABLED)

    def test_lockfree_flag(self):
        if SSRJSON_IS_LOCKFREE:
            assert SSRJSON_IS_FREETHREADING

    @pytest.mark.skipif(
        SKIP_RO_RACE_TEST,
        reason="GIL enabled",
    )
    def test_loads_correctness(self):
        threads = self._default_threads()
        loops = 20

        for name, as_str, as_bytes in self._iter_bench_sources():
            for source in (as_str, as_bytes):
                ref = ssrjson.loads(source)

                def worker(src=source, expected=ref, file=name):
                    got = ssrjson.loads(src)
                    assert got == expected, f"Mismatch in {file} ({type(src).__name__})"

                self._run_in_threads(threads=threads, loops=loops, worker=worker)

    @pytest.mark.skipif(
        SKIP_RO_RACE_TEST,
        reason="GIL enabled",
    )
    def test_dumps_correctness(self):
        threads = self._default_threads()
        loops = 20

        for name, as_str, as_bytes in self._iter_bench_sources():
            for source in (as_str, as_bytes):
                obj = ssrjson.loads(source)
                ref = ssrjson.dumps(obj)

                def worker(o=obj, expected=ref, file=name, src=source):
                    got = ssrjson.dumps(o)
                    assert got == expected, f"Mismatch in {file} ({type(src).__name__})"

                self._run_in_threads(threads=threads, loops=loops, worker=worker)

    @pytest.mark.skipif(
        SKIP_RO_RACE_TEST,
        reason="GIL enabled",
    )
    def test_dumps_to_bytes_correctness(self):
        threads = self._default_threads()
        loops = 20

        for name, as_str, as_bytes in self._iter_bench_sources():
            for source in (as_str, as_bytes):
                obj = ssrjson.loads(source)
                ref = ssrjson.dumps_to_bytes(obj)

                def worker(o=obj, expected=ref, file=name, src=source):
                    got = ssrjson.dumps_to_bytes(o)
                    assert got == expected, f"Mismatch in {file} ({type(src).__name__})"

                self._run_in_threads(threads=threads, loops=loops, worker=worker)

    @pytest.mark.skipif(
        SKIP_RW_RACE_TEST,
        reason="GIL enabled or lockfree build",
    )
    def test_encode_race_dumps(self):
        """Concurrent writer mutates a container while multiple threads call `ssrjson.dumps`.

        The test fails if any thread raises an exception or the process crashes.
        """
        shared = {"arr": [1, 2, 3], "map": {"x": 1}}
        excs = []

        def writer():
            try:
                for i in range(1000):
                    shared["arr"].append(i)
                    if shared["arr"]:
                        shared["arr"].pop(0)
                        shared["map"][f"k{i % 5}"] = i
                        # use pop with default to avoid KeyError due to races
                        shared["map"].pop(f"k{(i + 1) % 5}", None)
            except Exception as e:
                excs.append(e)

        def reader_dumps():
            try:
                for _ in range(1000):
                    ssrjson.dumps(shared)
            except Exception as e:
                excs.append(e)

        threads = []
        t = threading.Thread(target=writer)
        threads.append(t)
        t.start()

        for _ in range(6):
            t = threading.Thread(target=reader_dumps)
            threads.append(t)
            t.start()

        for t in threads:
            t.join()

        assert not excs, f"Exceptions occurred in threads: {excs}"

    @pytest.mark.skipif(
        SKIP_RW_RACE_TEST,
        reason="GIL enabled or lockfree build",
    )
    def test_encode_race_dumps_to_bytes(self):
        """Same as above but using `ssrjson.dumps_to_bytes`."""
        shared = {"arr": [1, 2, 3], "map": {"x": 1}}
        excs = []

        def writer():
            try:
                for i in range(1000):
                    shared["arr"].append(i)
                    if shared["arr"]:
                        shared["arr"].pop(0)
                    shared["map"][f"k{i % 5}"] = i
                    # use pop with default to avoid KeyError due to races
                    shared["map"].pop(f"k{(i + 1) % 5}", None)
            except Exception as e:
                excs.append(e)

        def reader_bytes():
            try:
                for _ in range(1000):
                    ssrjson.dumps_to_bytes(shared)
            except Exception as e:
                excs.append(e)

        threads = []
        t = threading.Thread(target=writer)
        threads.append(t)
        t.start()

        for _ in range(6):
            t = threading.Thread(target=reader_bytes)
            threads.append(t)
            t.start()

        for t in threads:
            t.join()

        assert not excs, f"Exceptions occurred in threads: {excs}"

    @pytest.mark.skipif(
        SKIP_RW_RACE_TEST,
        reason="GIL enabled or lockfree build",
    )
    def test_encode_race_container_shrink(self):
        """Containers that shrink while being encoded: the size must be read after the container is locked."""
        inner = list(range(64))
        d = {"k": 1}
        shared = [inner, {"d": inner, "e": d}, [inner], d]
        stop = threading.Event()
        excs = []

        def writer():
            try:
                while not stop.is_set():
                    inner.extend(range(64))
                    del inner[1:]
                    d.clear()
                    d["k"] = 1
            except Exception as e:
                excs.append(e)

        def reader(fn):
            try:
                for _ in range(20000):
                    json.loads(fn(shared))
            except Exception as e:
                excs.append(e)

        writers = [threading.Thread(target=writer) for _ in range(2)]
        readers = [
            threading.Thread(target=reader, args=(fn,))
            for fn in (ssrjson.dumps, ssrjson.dumps_to_bytes) * 2
        ]
        for t in writers + readers:
            t.start()
        for t in readers:
            t.join()
        stop.set()
        for t in writers:
            t.join()

        assert not excs, f"Exceptions occurred in threads: {excs}"

    @pytest.mark.skipif(IS_GIL_ENABLED, reason="GIL enabled")
    def test_decode_race_bytearray_resize(self):
        """loads(bytearray) while other threads resize it: the input must be read as one consistent snapshot."""
        big = b'"' + b"a" * (4 << 20) + b'"'
        small = b'"' + b"b" * 100 + b'"'
        ba = bytearray(big)
        stop = threading.Event()
        bad = []

        def writer():
            while not stop.is_set():
                for v in (small, big):
                    try:
                        ba[:] = v
                    except BufferError:
                        pass  # a reader is copying it right now

        def reader():
            try:
                for _ in range(300):
                    r = ssrjson.loads(ba)
                    if r != "b" * 100 and not (
                        len(r) == len(big) - 2 and r.count("a") == len(r)
                    ):
                        bad.append(r[:20])
            except Exception as e:
                bad.append(e)

        writers = [threading.Thread(target=writer) for _ in range(2)]
        readers = [threading.Thread(target=reader) for _ in range(4)]
        for t in writers + readers:
            t.start()
        for t in readers:
            t.join()
        stop.set()
        for t in writers:
            t.join()

        assert not bad, f"inconsistent reads: {bad[:3]}"
