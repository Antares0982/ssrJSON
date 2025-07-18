import os

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel

# this is only for publishing
use_nix_prebuilt = bool(os.environ.get("SSRJSON_USE_NIX_PREBUILT"))


with open("./version_file", "r", encoding="utf-8") as f:
    version_string = f.read().strip()


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
        version=version_string,
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

    class CMakeBuild(build_ext):
        def run(self):
            build_dir = os.path.abspath("build")
            if not os.path.exists(build_dir):
                os.makedirs(build_dir)

            # configure
            if os.name == "nt":
                cmake_cmd = [
                    "cmake",
                    "-T",
                    "ClangCL",
                    "-DCMAKE_BUILD_TYPE=Release",
                    f"-DPREDEFINED_VERSION={version_string}",
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
                    f"-DPREDEFINED_VERSION={version_string}",
                    "-DBUILD_TEST=OFF",
                    ".",
                    "-B",
                    "build",
                ]
            subprocess.check_call(cmake_cmd)
            # 运行 cmake --build .
            if os.name == "nt":
                build_cmd = ["cmake", "--build", "build", "--config", "Release"]
            else:
                nproc = subprocess.check_output("nproc").strip()
                build_cmd = ["cmake", "--build", "build", "--", "-j", nproc]
            subprocess.check_call(build_cmd)

            # Copy file
            if os.name == "nt":
                built_filename = "Release/ssrjson.dll"
                target_filename = "ssrjson.pyd"
            else:
                built_filename = "ssrjson.so"
                target_filename = built_filename

            built_path = os.path.join(build_dir, built_filename)

            if not os.path.exists(built_path):
                raise RuntimeError(f"Built library not found: {built_path}")

            target_dir = os.path.join(self.build_lib, "ssrjson")
            if not os.path.exists(target_dir):
                raise RuntimeError("ssrjson directory not found")

            target_path = os.path.join(target_dir, target_filename)

            self.announce(f"Copying {built_path} to {target_path}")
            print(f"Copying {built_path} to {target_path}")
            shutil.copyfile(built_path, target_path)

    setup(
        name="ssrjson",
        version=version_string,
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
        package_data={"ssrjson": ["ssrjson.so", "ssrjson.dll"]},
        include_package_data=True,
    )
