"""Run ASAN tests on macOS with full instrumentation.

Builds ssrjson.so with -DASAN_ENABLED=ON (compile-time instrumentation +
ASAN runtime linked).  Instead of DYLD_INSERT_LIBRARIES, a small wrapper
binary is compiled that links both ASAN and Python directly, then calls
Py_Initialize() to embed the interpreter.  Because the wrapper and
ssrjson.so both link the ASAN runtime via normal LC_LOAD_DYLIB, dyld
deduplicates correctly - no double-load, no "Interceptors are not
working" error.
"""

import glob
import os
import shutil
import subprocess
import sys
import tempfile
import warnings

IS_GIL_ENABLED = not hasattr(sys, "_is_gil_enabled") or sys._is_gil_enabled()  # pylint: disable=protected-access


def _wrapper_src(site_packages: str) -> str:
    """Return C source for the ASAN wrapper launcher.

    Links Python directly (Py_Initialize) rather than dlopen+Py_Main.
    Compiled with -fsanitize=address, the wrapper binary links the ASAN
    runtime as a normal LC_LOAD_DYLIB dependency.  When the embedded
    Python imports the (also instrumented) ssrjson.so, dyld correctly
    deduplicates the ASAN dylib.
    """
    return rf"""
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {{
    if (argc < 3) {{
        fprintf(stderr, "usage: %s <build_dir> <--c|-m> [args...]\n", argv[0]);
        return 1;
    }}
    const char *build_dir = argv[1];
    const char *mode = argv[2];

    setenv("PYTHONPATH", build_dir, 1);
    // setenv("PYTHONMALLOC", "malloc", 1);

    int py_argc = argc - 2;
    wchar_t **py_argv = (wchar_t**)PyMem_RawMalloc((py_argc + 1) * sizeof(wchar_t*));
    if (!py_argv) return 1;
    py_argv[0] = Py_DecodeLocale(argv[0], NULL);
    for (int i = 1; i < py_argc; i++)
        py_argv[i] = Py_DecodeLocale(argv[i + 2], NULL);
    py_argv[py_argc] = NULL;

    Py_Initialize();
    PySys_SetArgv(py_argc, py_argv);

    // Add virtualenv / user site-packages to sys.path
    PyRun_SimpleString(
        "import site, sys\n"
        "site.addsitedir(r'{site_packages}')\n"
    );

    int rc = 0;
    if (strcmp(mode, "-c") == 0) {{
        rc = PyRun_SimpleString(argv[3]);
    }} else {{
        char buf[4096];
        int pos = snprintf(buf, sizeof(buf), "import pytest; pytest.main([");
        for (int i = 3; i < argc; i++) {{
            pos += snprintf(buf + pos, sizeof(buf) - pos,
                            "%s\"%s\"", (i > 3 ? ", " : ""), argv[i]);
        }}
        snprintf(buf + pos, sizeof(buf) - pos, "])");
        rc = PyRun_SimpleString(buf);
    }}

    if (PyErr_Occurred())
        PyErr_Print();

    Py_Finalize();
    for (int i = 0; i < py_argc; i++)
        PyMem_RawFree(py_argv[i]);
    PyMem_RawFree(py_argv);
    return rc;
}}
"""


def find_python_cmake_env():
    from sysconfig import get_config_h_filename, get_config_var

    include_dir = os.path.dirname(get_config_h_filename())
    lib_dir = get_config_var("LIBDIR")
    minor = sys.version_info[1]
    library_file = os.path.join(
        lib_dir, f"libpython3.{minor}{'t' if not IS_GIL_ENABLED else ''}.dylib"
    )
    return include_dir, library_file


def build(build_dir: str, build_type: str, asan: bool, lockfree: bool) -> None:
    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)
    new_env = os.environ.copy()
    cc = shutil.which("clang")
    cxx = shutil.which("clang++")
    if not cc or not cxx:
        raise RuntimeError("clang/clang++ not found in PATH")
    new_env["CC"] = cc
    new_env["CXX"] = cxx
    configure_cmd = [
        "cmake",
        "-S",
        ".",
        "-B",
        build_dir,
        "-DCMAKE_BUILD_TYPE=" + build_type,
    ]
    if not IS_GIL_ENABLED:
        configure_cmd += ["-DBUILD_FREE_THREADING=ON"]
    if lockfree:
        configure_cmd += ["-DFREE_THREADING_LOCKFREE=ON"]

    include_dir, library_file = find_python_cmake_env()
    new_env["Python3_INCLUDE_DIR"] = include_dir
    new_env["Python3_LIBRARY"] = library_file
    configure_cmd += ["-DSEARCH_PYTHON3_USE_ENV=ON"]

    if asan:
        configure_cmd += ["-DASAN_ENABLED=ON"]

    subprocess.run(configure_cmd, check=True, env=new_env)
    build_cmd = ["cmake", "--build", build_dir, "-j"]
    subprocess.run(build_cmd, check=True)


def build_wrapper(
    clang_exe: str, python_include: str, python_lib: str, site_packages: str
) -> str:
    """Compile the ASAN wrapper launcher, return its path."""
    src = os.path.join(tempfile.gettempdir(), "ssrjson_asan_wrapper.c")
    with open(src, "w") as fh:
        fh.write(_wrapper_src(site_packages))
    out = os.path.join(tempfile.gettempdir(), "ssrjson_asan_wrapper")
    lib_dir = os.path.dirname(python_lib)
    subprocess.run(
        [
            clang_exe,
            "-fsanitize=address",
            "-g",
            "-O1",
            "-I",
            python_include,
            "-L",
            lib_dir,
            src,
            "-o",
            out,
            python_lib,
        ],
        check=True,
    )
    return out


def asan_check_ssrjson_import_leak(wrapper: str, build_dir: str) -> None:
    cmd = [wrapper, build_dir, "-c", "import ssrjson"]
    subprocess.run(cmd, check=True)


def get_minor_version(python_exe: str) -> int:
    result = subprocess.run(
        [python_exe, "--version"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=True,
    )
    return int(result.stdout.strip().removeprefix("Python ").split(".")[1])


def asan_check_cpython_leak(wrapper: str, build_dir: str, env: dict) -> bool:
    cmd = [wrapper, build_dir, "-m", "--collect-only", "python-test"]
    result = _run_with_asan_log(cmd, env, check=False)
    stdout = result.stdout or ""
    if result.returncode != 0:
        if "ERROR: LeakSanitizer" in stdout:
            return True
        _print_asan_logs(os.path.join(tempfile.gettempdir(), "asan_log"))
    return False


def find_site_packages(executable: str) -> str:
    """Return the site-packages directory for the given Python executable."""
    result = subprocess.run(
        [executable, "-c", "import sysconfig; print(sysconfig.get_path('purelib'))"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=True,
    )
    return result.stdout.strip()


def _run_with_asan_log(
    cmd: list, env: dict, check: bool = True
) -> subprocess.CompletedProcess:
    """Run a command with ASAN log_path, print the log on failure."""
    asan_log = os.path.join(tempfile.gettempdir(), "asan_log")
    for f in glob.glob(asan_log + "*"):
        try:
            os.unlink(f)
        except OSError:
            pass

    run_env = env.copy()
    existing = run_env.get("ASAN_OPTIONS", "")
    prefix = f"log_path={asan_log}"
    run_env["ASAN_OPTIONS"] = prefix if not existing else f"{existing}:{prefix}"

    try:
        return subprocess.run(
            cmd,
            check=check,
            env=run_env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
    except subprocess.CalledProcessError as e:
        _print_asan_logs(asan_log)
        if e.stdout:
            sys.stderr.write(e.stdout)
        raise


def _print_asan_logs(asan_log: str) -> None:
    """Print any ASAN log files found at the given prefix."""
    logs = sorted(glob.glob(asan_log + "*"))
    if not logs:
        return
    print(f"\n{'=' * 70}", file=sys.stderr)
    print("ASAN report(s):", file=sys.stderr)
    for logfile in logs:
        with open(logfile, "r") as fh:
            content = fh.read()
        if content.strip():
            print(f"--- {os.path.basename(logfile)} ---", file=sys.stderr)
            sys.stderr.write(content)
    print(f"{'=' * 70}\n", file=sys.stderr)


def test(build_dir: str, asan: bool):
    python_exe = sys.executable
    new_env = os.environ.copy()

    if asan:
        cc = shutil.which("clang")
        if not cc:
            raise RuntimeError("clang not found in PATH")
        include_dir, python_lib = find_python_cmake_env()
        site_packages = find_site_packages(python_exe)
        wrapper = build_wrapper(cc, include_dir, python_lib, site_packages)
        new_env["MallocNanoZone"] = "0"

        minor_ver = get_minor_version(python_exe)
        is_below_314 = minor_ver < 14
        if not is_below_314:
            _run_with_asan_log([wrapper, build_dir, "-c", "import ssrjson"], new_env)
        if is_below_314 or asan_check_cpython_leak(wrapper, build_dir, new_env):
            new_env["ASAN_OPTIONS"] = "detect_leaks=0"
            warnings.warn(
                "Disable ASAN leak detection due to CPython memory leak issues"
            )

        cmd = [wrapper, build_dir, "-m", "--random-order", "python-test"]
        _run_with_asan_log(cmd, new_env)
    else:
        new_env["PYTHONPATH"] = os.path.join(os.curdir, build_dir)
        cmd = [python_exe, "-m", "pytest", "--random-order", "python-test"]
        subprocess.run(cmd, check=True, env=new_env)


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser(description="Run ASAN tests on macOS")
    parser.add_argument(
        "--build-type", help="CMake Build type, default to `Debug`", default="Debug"
    )
    parser.add_argument(
        "--asan", help="Run address sanitizer check", action="store_true"
    )
    parser.add_argument(
        "--lockfree", help="Test lock-free free-threading build", action="store_true"
    )
    parser.add_argument("--build-only", help="Build only", action="store_true")
    parser.add_argument(
        "--build-dir",
        help="Build directory (relative path), default to `build`",
        default="build",
    )
    args = parser.parse_args()
    build_type: str = args.build_type
    asan = bool(args.asan)
    lockfree = bool(args.lockfree)
    build_only = bool(args.build_only)
    build_dir: str = args.build_dir
    if lockfree and IS_GIL_ENABLED:
        raise ValueError(
            "Lock-free free-threading build is not supported when GIL is enabled"
        )
    build(build_dir, build_type, asan, lockfree)
    if not build_only:
        test(build_dir, asan)
    return 0


if __name__ == "__main__":
    sys.exit(main())
