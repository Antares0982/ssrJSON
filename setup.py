import os
import sys

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel

# this is only for publishing
USE_NIX_PREBUILT = bool(os.environ.get("SSRJSON_USE_NIX_PREBUILT"))
USE_PGO = bool(os.environ.get("SSRJSON_ENABLE_PGO"))
PGO_PROFILE = os.environ.get("SSRJSON_PGO_PROFILE")
IS_GIL_ENABLED = not hasattr(sys, "_is_gil_enabled") or sys._is_gil_enabled()  # pylint: disable=protected-access


def get_version_from_pyproject_toml():
    with open("./pyproject.toml", "r", encoding="utf-8") as f:
        content = f.read()
    prefix = 'version = "'
    for line in content.splitlines():
        if line.startswith(prefix):
            version_line = line[len(prefix) :]
            version_string = version_line[: version_line.find('"')]
            return version_string
    raise RuntimeError("Invalid pyproject.toml, expected version into inside")


def find_windows_python_cmake_env():
    from sysconfig import get_config_h_filename, get_config_var

    include_dir = os.path.dirname(get_config_h_filename())
    lib_dir = get_config_var("LIBDIR")
    minor = sys.version_info[1]
    library_file = os.path.join(
        lib_dir, f"python3{minor}{'t' if not IS_GIL_ENABLED else ''}.lib"
    )
    return include_dir, library_file


VERSION_STRING = get_version_from_pyproject_toml()

if USE_NIX_PREBUILT:

    class PrebuiltBuildExt(build_ext):
        def build_extension(self, ext):
            pass

    class PrebuiltBdistWheel(_bdist_wheel):
        def finalize_options(self):
            super().finalize_options()
            self.root_is_pure = False

    setup(
        name="ssrjson",
        version=VERSION_STRING,
        packages=["ssrjson"],
        ext_modules=[
            Extension(
                "ssrjson",
                sources=[],
            )
        ],
        cmdclass={"build_ext": PrebuiltBuildExt, "bdist_wheel": PrebuiltBdistWheel},
        include_package_data=True,
    )
else:
    import shutil
    import subprocess
    import glob as _glob

    def run_check(cmd, **kwargs):
        try:
            subprocess.run(cmd, check=True, **kwargs)
        except subprocess.CalledProcessError:
            print(f"command failed: {cmd}", file=sys.stderr)
            raise
        except Exception:
            print(f"command failed: {cmd}", file=sys.stderr)
            raise

    class CMakeBuild(build_ext):
        def run(self):
            env = os.environ.copy()
            build_dir = os.path.abspath("build")
            if not os.path.exists(build_dir):
                os.makedirs(build_dir)

            common_flags = [
                "-DCMAKE_BUILD_TYPE=Release",
                f"-DPREDEFINED_VERSION={VERSION_STRING}",
                "-DBUILD_CTESTS=OFF",
            ]
            if not IS_GIL_ENABLED:
                common_flags.append("-DBUILD_FREE_THREADING=ON")
                if os.name == "nt":
                    common_flags.append("-DSEARCH_PYTHON3_USE_ENV=ON")
                    include_dir, library_file = find_windows_python_cmake_env()
                    env["Python3_INCLUDE_DIR"] = include_dir
                    env["Python3_LIBRARY"] = library_file

            if os.name == "nt":
                cmake_cmd = ["cmake", "-T", "ClangCL"] + common_flags + [".", "-B"]
                build_cmd = ["cmake", "--build", build_dir, "--config", "Release"]
            else:
                cmake_cmd = (
                    [
                        "cmake",
                        "-DCMAKE_C_COMPILER=clang",
                        "-DCMAKE_CXX_COMPILER=clang++",
                    ]
                    + common_flags
                    + [".", "-B"]
                )
                build_cmd = ["cmake", "--build", build_dir, "-j"]

            if USE_PGO:
                if PGO_PROFILE:
                    run_check(
                        cmake_cmd
                        + [
                            build_dir,
                            f"-DBUILD_PGO_USE={os.path.abspath(PGO_PROFILE)}",
                        ],
                        env=env,
                    )
                else:
                    script_dir = os.path.dirname(os.path.abspath(__file__))
                    pgo_instr_dir = os.path.abspath("build-pgo-instr")
                    pgo_data_dir = os.path.abspath("test_data/pgo")
                    os.makedirs(pgo_data_dir, exist_ok=True)

                    # Phase 1: Instrumented build
                    run_check(
                        cmake_cmd + [pgo_instr_dir, "-DBUILD_PGO_GENERATE=ON"],
                        env=env,
                    )
                    build_instr_cmd = (
                        ["cmake", "--build", pgo_instr_dir, "--config", "Release"]
                        if os.name == "nt"
                        else ["cmake", "--build", pgo_instr_dir, "-j"]
                    )
                    run_check(build_instr_cmd, env=env)

                    # Phase 2: Training
                    train_env = env.copy()
                    train_env["LLVM_PROFILE_FILE"] = os.path.join(
                        pgo_data_dir, "ssrjson_%m_%p.profraw"
                    )
                    train_cmd = [
                        sys.executable,
                        os.path.join(script_dir, "ci", "pgo_train.py"),
                        "--build-dir",
                        pgo_instr_dir,
                        "--bench-dir",
                        os.path.join(script_dir, "bench"),
                        "--profile-dir",
                        pgo_data_dir,
                    ]
                    run_check(train_cmd, env=train_env)

                    # Merge profiles
                    merged = os.path.join(pgo_data_dir, "merged.profdata")
                    profraw_files = _glob.glob(os.path.join(pgo_data_dir, "*.profraw"))
                    if not profraw_files:
                        raise RuntimeError(
                            "PGO training did not produce any .profraw files"
                        )
                    run_check(
                        ["llvm-profdata", "merge", "-o", merged, "-sparse"]
                        + profraw_files,
                        env=env,
                    )

                    # Phase 3: PGO-optimized build
                    run_check(
                        cmake_cmd + [build_dir, f"-DBUILD_PGO_USE={merged}"],
                        env=env,
                    )
            else:
                run_check(cmake_cmd + [build_dir], env=env)

            run_check(build_cmd, env=env)

            # Copy file
            if os.name == "nt":
                built_filename = "Release/ssrjson.pyd"
                target_filename = "ssrjson.pyd"
            else:
                built_filename = "ssrjson.so"
                target_filename = built_filename
            #
            built_path = os.path.join(build_dir, built_filename)
            if not os.path.exists(built_path):
                raise RuntimeError(f"Built library not found: {built_path}")
            #
            target_dir = os.path.join(self.build_lib, "ssrjson")
            if not os.path.exists(target_dir):
                raise RuntimeError("ssrjson directory not found")
            target_path = os.path.join(target_dir, target_filename)
            self.announce(f"Copying {built_path} to {target_path}")
            print(f"Copying {built_path} to {target_path}")
            shutil.copyfile(built_path, target_path)

    setup(
        name="ssrjson",
        version=VERSION_STRING,
        packages=["ssrjson"],
        ext_modules=[
            Extension(
                "ssrjson",
                sources=[],
            )
        ],
        cmdclass={
            "build_ext": CMakeBuild,
        },
        include_package_data=True,
    )
