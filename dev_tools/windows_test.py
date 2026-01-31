import json
import os
import shutil
import subprocess
import sys
import warnings

SSRJSON_FILE = "ssrjson.pyd"
ASAN_DLL = "clang_rt.asan_dynamic-x86_64.dll"
IS_GIL_ENABLED = not hasattr(sys, "_is_gil_enabled") or sys._is_gil_enabled()  # pylint: disable=protected-access


def find_python_cmake_env():
    from sysconfig import get_config_h_filename, get_config_var

    include_dir = os.path.dirname(get_config_h_filename())
    lib_dir = get_config_var("LIBDIR")
    minor = sys.version_info[1]
    library_file = os.path.join(
        lib_dir, f"python3{minor}{'t' if not IS_GIL_ENABLED else ''}.lib"
    )
    return include_dir, library_file


def build(build_dir: str, build_type: str, asan: bool) -> None:
    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)
    configure_cmd = [
        "cmake",
        "-T",
        "ClangCL",
        "-S",
        ".",
        "-B",
        build_dir,
        "-DCMAKE_BUILD_TYPE=" + build_type,
    ]

    if asan:
        configure_cmd += ["-DASAN_ENABLED=ON"]
    if not IS_GIL_ENABLED:
        configure_cmd.append("-DBUILD_FREE_THREADING=ON")
        configure_cmd.append("-DSEARCH_PYTHON3_USE_ENV=ON")
    env = os.environ.copy()
    if not IS_GIL_ENABLED:
        include_dir, library_file = find_python_cmake_env()
        env["Python3_INCLUDE_DIR"] = include_dir
        env["Python3_LIBRARY"] = library_file
    subprocess.run(configure_cmd, check=True, env=env)
    build_cmd = ["cmake", "--build", build_dir, "--config", build_type]
    subprocess.run(build_cmd, check=True)
    copy_src = os.path.join(build_dir, build_type, SSRJSON_FILE)
    copy_to = os.path.join(build_dir, SSRJSON_FILE)
    os.rename(copy_src, copy_to)
    if asan:
        copy_src = os.path.join(build_dir, build_type, ASAN_DLL)
        copy_to = os.path.join(build_dir, ASAN_DLL)
        os.rename(copy_src, copy_to)


def find_exe(build_dir: str) -> str:
    return sys.executable
    info_file = os.path.join(build_dir, "info.json")
    with open(info_file, "rb") as f:
        info = json.load(f)
    python3_root = info.get("Python3_ROOT_DIR")
    if not python3_root:
        warnings.warn(
            f"Python3_ROOT_DIR not set in info.json, fallback to sys.executable={sys.executable}"
        )
        return sys.executable
    exe_path = os.path.join(python3_root, "python.exe")
    if not os.path.exists(exe_path):
        warnings.warn(
            f"python.exe not found in Python3_ROOT_DIR={python3_root}, fallback to sys.executable={sys.executable}"
        )
        return sys.executable
    return exe_path


def test(build_dir: str):
    new_env = os.environ.copy()
    new_env["PYTHONPATH"] = os.path.join(os.curdir, build_dir)
    cmd = [find_exe(build_dir), "-m", "pytest", "python-test"]
    subprocess.run(cmd, check=True, env=new_env)


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser(description="Run tests on Windows")
    parser.add_argument(
        "--build-type", help="CMake Build type, default to `Debug`", default="Debug"
    )
    parser.add_argument("--asan", help="Run asan check", action="store_true")
    parser.add_argument(
        "--build-dir",
        help="Build directory (relative path), default to `build`",
        default="build",
    )
    args = parser.parse_args()
    build_type: str = args.build_type
    build_dir: str = args.build_dir
    asan = bool(args.asan)
    build(build_dir, build_type, asan)
    test(build_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
