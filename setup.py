import os
import sys

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel

# this is only for publishing
use_nix_prebuilt = bool(os.environ.get("SSRJSON_USE_NIX_PREBUILT"))


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


VERSION_STRING = get_version_from_pyproject_toml()

if use_nix_prebuilt:

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

    def run_check(cmd):
        try:
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError:
            print(f"command failed: {cmd}", file=sys.stderr)
            raise
        except Exception:
            print(f"command failed: {cmd}", file=sys.stderr)
            raise

    class CMakeBuild(build_ext):
        def run(self):
            build_dir = os.path.abspath("build")
            if not os.path.exists(build_dir):
                os.makedirs(build_dir)
            # Configure
            if os.name == "nt":
                cmake_cmd = [
                    "cmake",
                    "-T",
                    "ClangCL",
                    "-DCMAKE_BUILD_TYPE=Release",
                    f"-DPREDEFINED_VERSION={VERSION_STRING}",
                    "-DBUILD_TEST=OFF",
                    ".",
                    "-B",
                    "build",
                ]
            else:
                cmake_cmd = [
                    "cmake",
                    "-DCMAKE_C_COMPILER=clang",
                    "-DCMAKE_CXX_COMPILER=clang++",
                    "-DCMAKE_BUILD_TYPE=Release",
                    f"-DPREDEFINED_VERSION={VERSION_STRING}",
                    "-DBUILD_TEST=OFF",
                    ".",
                    "-B",
                    "build",
                ]
            run_check(cmake_cmd)
            # Build
            if os.name == "nt":
                build_cmd = ["cmake", "--build", "build", "--config", "Release"]
            else:
                # use `-j` default job count
                build_cmd = ["cmake", "--build", "build", "-j"]
            run_check(build_cmd)
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
