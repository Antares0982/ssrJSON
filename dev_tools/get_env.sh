# input: Python3_EXECUTABLE, BUILD_PY_VER, ISOLATE_BUILD (all optional)
# output: Python3_EXECUTABLE, Python3_INCLUDE_DIR, Python3_LIBRARY, CUR_PYVER, BUILD_DIR, Python3_ROOT_DIR

if [ -z ${Python3_EXECUTABLE+x} ]; then
    if [ -z ${BUILD_PY_VER+x} ]; then
        export Python3_EXECUTABLE=$(readlink -f $(which python3))
    else
        export Python3_EXECUTABLE=$(readlink -f $(which python3.$BUILD_PY_VER))
    fi
fi
export Python3_INCLUDE_DIR=$($Python3_EXECUTABLE -c "from sysconfig import get_config_h_filename; from os.path import dirname; print(dirname(get_config_h_filename()))")
export Python3_LIBRARY=$($Python3_EXECUTABLE -c "from sysconfig import get_config_var; print(get_config_var('LIBDIR'))")/libpython3.so
export CUR_PYVER=$(echo "$($Python3_EXECUTABLE --version)" | cut -d'.' -f2)
export Python3_ROOT_DIR=$(dirname $(dirname $(readlink -f $Python3_EXECUTABLE)))

if [ -z ${ISOLATE_BUILD+x} ]; then
    export BUILD_DIR=build
else
    export BUILD_DIR=build-py3.$CUR_PYVER
fi

echo "Python3_EXECUTABLE=$Python3_EXECUTABLE"
echo "Python3_INCLUDE_DIR=$Python3_INCLUDE_DIR"
echo "Python3_LIBRARY=$Python3_LIBRARY"
echo "CUR_PYVER=$CUR_PYVER"
echo "BUILD_DIR=$BUILD_DIR"
