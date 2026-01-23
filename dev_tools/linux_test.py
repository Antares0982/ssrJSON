import json
import os
import shutil
import subprocess
import sys
import warnings

SSRJSON_FILE = "ssrjson.so"
ASAN_DLL = "clang_rt.asan_dynamic-x86_64.dll"


def find_python_cmake_env():
    # only intended for python3.15
    from sysconfig import get_config_h_filename, get_config_var

    include_dir = os.path.dirname(get_config_h_filename())
    lib_dir = get_config_var("LIBDIR")
    minor = sys.version_info[1]
    library_file = os.path.join(lib_dir, f"libpython3.{minor}.so")
    return include_dir, library_file


def build(build_dir: str, build_type: str, asan: bool) -> None:
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
    if sys.version_info >= (3, 15):
        # currently CMake cannot find Python3.15 properly
        include_dir, library_file = find_python_cmake_env()
        new_env["Python3_INCLUDE_DIR"] = include_dir
        new_env["Python3_LIBRARY"] = library_file
        configure_cmd += ["-DSEARCH_PYTHON3_USE_ENV=ON"]

    if asan:
        configure_cmd += ["-DASAN_ENABLED=ON"]
    subprocess.run(configure_cmd, check=True, env=new_env)
    build_cmd = ["cmake", "--build", build_dir, "-j"]
    subprocess.run(build_cmd, check=True)


def find_exe(build_dir: str) -> str:
    info_file = os.path.join(build_dir, "info.json")
    with open(info_file, "rb") as f:
        info = json.load(f)
    python3_root = info.get("Python3_ROOT_DIR")
    if not python3_root:
        warnings.warn(
            f"Python3_ROOT_DIR not set in info.json, fallback to sys.executable={sys.executable}"
        )
        return sys.executable
    exe_path = os.path.join(python3_root, "bin", "python")
    if not os.path.exists(exe_path):
        warnings.warn(
            f"python not found in Python3_ROOT_DIR={python3_root}, fallback to sys.executable={sys.executable}"
        )
        return sys.executable
    return exe_path


def find_libasan_ldd(build_dir: str) -> str:
    so_file = os.path.join(build_dir, SSRJSON_FILE)
    ldd_result = subprocess.run(
        ["ldd", so_file],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=True,
    )
    for line in ldd_result.stdout.splitlines():
        if "libasan.so" in line:
            parts = line.strip().split("=>")
            if len(parts) >= 2:
                lib_path = parts[1].strip().split()[0]
                return lib_path
    raise RuntimeError("libasan.so not found in ldd output")


def get_minor_version(python_exe: str) -> int:
    minor_version_str = (
        subprocess.run(
            [python_exe, "--version"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True,
        )
        .stdout.strip()
        .removeprefix("Python ")
        .split(".")[1]
    )
    return int(minor_version_str)


def test(build_dir: str, asan: bool):
    new_env = os.environ.copy()
    new_env["PYTHONPATH"] = os.path.join(os.curdir, build_dir)
    python_exe = find_exe(build_dir)

    if asan:
        minor_version = get_minor_version(python_exe)
        new_env["LD_PRELOAD"] = find_libasan_ldd(build_dir)
        if minor_version <= 13:
            # version below python3.13 has memory leak issues internally
            new_env["ASAN_OPTIONS"] = "detect_leaks=0"
    cmd = [find_exe(build_dir), "-m", "pytest", "--random-order", "python-test"]
    subprocess.run(cmd, check=True, env=new_env)


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser(description="Run tests on Linux")
    parser.add_argument(
        "--build-type", help="CMake Build type, default to `Debug`", default="Debug"
    )
    parser.add_argument("--asan", help="Run asan check", action="store_true")
    parser.add_argument("--build-only", help="Build only", action="store_true")
    parser.add_argument(
        "--build-dir",
        help="Build directory (relative path), default to `build`",
        default="build",
    )
    args = parser.parse_args()
    build_type: str = args.build_type
    build_dir: str = args.build_dir
    asan = bool(args.asan)
    build_only = bool(args.build_only)
    build(build_dir, build_type, asan)
    if not build_only:
        test(build_dir, asan)
    return 0


if __name__ == "__main__":
    sys.exit(main())
