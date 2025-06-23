import os
import shutil

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel


class CustomBuildExt(build_ext):
    def build_extension(self, ext):
        out_file = self.get_ext_fullpath(ext.name)
        os.makedirs(os.path.dirname(out_file), exist_ok=True)
        import subprocess

        file = subprocess.check_output(
            ["find", ".", "-name", "ssrjson.cpython*.so"], encoding="utf-8"
        ).strip()
        precompiled_path = os.path.abspath(file if file else out_file)
        self.announce(
            f"Copying precompiled extension from {precompiled_path} to {out_file}"
        )
        shutil.copyfile(precompiled_path, out_file)


class CustomBdistWheel(_bdist_wheel):
    def finalize_options(self):
        super().finalize_options()
        self.root_is_pure = False


setup(
    name="ssrjson",
    version="0.0.0",
    packages=["ssrjson"],
    ext_modules=[
        Extension(
            "ssrjson",
            sources=[
                # "result/*" # TODO ???
            ],
        )
    ],
    cmdclass={"build_ext": CustomBuildExt, "bdist_wheel": CustomBdistWheel},
    include_package_data=True,
)
