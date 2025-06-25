set -e
TARGET_GLIBC_MINOR_VERSION=17
TARGET_PY_MINOR_VER=$1
rm -rf dist wheelhouse ssrjson ssrjson.egg-info
mkdir ssrjson
nix build .#ssrjson-py3${TARGET_PY_MINOR_VER}
SSRJSON_SONAME=ssrjson.cpython-3${TARGET_PY_MINOR_VER}-x86_64-linux-gnu.so
cp result/${SSRJSON_SONAME} ssrjson/
cp pysrc/__init__.py ssrjson/
cp pysrc/__init__.pyi ssrjson/
cp pysrc/py.typed ssrjson/
nix develop .#packShell3${TARGET_PY_MINOR_VER} -c python dev_tools/check_glibc_version.py ssrjson/$SSRJSON_SONAME ${TARGET_GLIBC_MINOR_VERSION}
SSRJSON_USE_NIX_PREBUILT=1 nix develop .#packShell3${TARGET_PY_MINOR_VER} --no-net -c python -m build
nix develop .#packShell3${TARGET_PY_MINOR_VER} --no-net -c auditwheel repair --plat manylinux_2_${TARGET_GLIBC_MINOR_VERSION}_x86_64 dist/*.whl

# sha256sum ssrjson/ssrjson.cpython-3${TARGET_PY_MINOR_VER}-x86_64-linux-gnu.so | awk '{print $1}'
