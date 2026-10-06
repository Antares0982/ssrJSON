"""Build and test scalar-capable distributions."""

import argparse
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import sysconfig
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def run(args, **kwargs):
    print("+", " ".join(map(str, args)), flush=True)
    subprocess.run(list(map(str, args)), check=True, **kwargs)


def test(prefix, directory=None):
    env = os.environ.copy()
    env.pop("PYTHONPATH", None)
    if directory:
        env["PYTHONPATH"] = str(directory.resolve())
    probe = (
        "import struct,ssrjson,numpy; "
        f'assert struct.calcsize("P")=={struct.calcsize("P")}; '
        "print(ssrjson.__file__,ssrjson.get_current_features()); "
        'assert ssrjson.get_current_features()["simd"]=="SCALAR"'
    )
    run([*prefix, sys.executable, "-c", probe], env=env)
    args = [
        *prefix,
        sys.executable,
        "-m",
        "pytest",
        "--random-order",
        str(ROOT / "python-test"),
    ]
    if prefix or os.environ.get("SSRJSON_EMULATED"):
        args += [
            "--ignore",
            str(ROOT / "python-test/test_memory.py"),
            "--ignore",
            str(ROOT / "python-test/test_freethreading.py"),
        ]
    run(args, env=env)
    if directory:
        executable = directory / (
            "ssrjson_test.exe" if os.name == "nt" else "ssrjson_test"
        )
        if executable.exists():
            run([*prefix, executable], env=env)
        else:
            module = directory / "ssrjson_test.so"
            run(
                [
                    *prefix,
                    sys.executable,
                    "-c",
                    f"import ctypes; assert ctypes.PyDLL({str(module)!r}).ssrjson_run_tests() == 0",
                ],
                env=env,
            )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sde")
    parser.add_argument("--release", action="store_true")
    args = parser.parse_args()
    os.chdir(ROOT)
    assert struct.calcsize("P") == (8 if args.sde else 4)
    prefix = (
        [
            args.sde,
            "-p4p",
            "-chip_check_image",
            "ssrjson.pyd",
            "-chip_check_image",
            "ssrjson_test.exe",
            "--",
        ]
        if args.sde
        else []
    )
    flags = [f"-DPython3_EXECUTABLE={sys.executable}"]
    if sysconfig.get_config_var("Py_GIL_DISABLED"):
        flags += ["-DBUILD_FREE_THREADING=ON"]
    if os.name == "nt" and sysconfig.get_config_var("Py_GIL_DISABLED"):
        from windows_test import find_python_cmake_env

        include, library = find_python_cmake_env()
        os.environ["Python3_INCLUDE_DIR"] = include
        os.environ["Python3_LIBRARY"] = library
        os.environ["Python3_EXECUTABLE"] = sys.executable
        flags += ["-DSEARCH_PYTHON3_USE_ENV=ON"]
    if os.name == "nt":
        flags += [
            "-T",
            "ClangCL",
            "-A",
            "Win32" if struct.calcsize("P") == 4 else "x64",
        ]
    else:
        flags += ["-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++"]
    for mode in ["Debug", "Release"]:
        directory = ROOT / f"build-portable-{mode.lower()}"
        run(
            ["cmake", "-S", ROOT, "-B", directory, f"-DCMAKE_BUILD_TYPE={mode}", *flags]
        )
        run(["cmake", "--build", directory, "--config", mode, "-j", "2"])
        test(prefix, directory / mode if os.name == "nt" else directory)

    # Build from the release source distribution.
    artifacts = ROOT / "portable-dist"
    artifacts.mkdir(exist_ok=True)
    if args.release:
        tarball = next((ROOT / "dist").glob("*.tar.gz"))
    else:
        run([sys.executable, "-m", "build", "--sdist", "--outdir", artifacts])
        tarball = next(artifacts.glob("*.tar.gz"))
    run(
        [
            sys.executable,
            "-m",
            "pip",
            "install",
            "--force-reinstall",
            "--no-cache-dir",
            tarball,
        ]
    )
    test(prefix)
    with tempfile.TemporaryDirectory() as wheel_dir:
        run(
            [
                sys.executable,
                "-m",
                "pip",
                "wheel",
                "--no-deps",
                "--no-cache-dir",
                "-w",
                wheel_dir,
                tarball,
            ]
        )
        wheel = next(Path(wheel_dir).glob("*.whl"))
        if sys.platform == "linux":
            run(
                [
                    "auditwheel",
                    "repair",
                    "--plat",
                    os.environ["AUDITWHEEL_PLAT"],
                    "-w",
                    artifacts,
                    wheel,
                ]
            )
        else:
            shutil.copy2(wheel, artifacts)
    wheel = next(artifacts.glob("*.whl"))
    run([sys.executable, "-m", "pip", "install", "--force-reinstall", wheel])
    test(prefix)


if __name__ == "__main__":
    main()
