{
  clangStdenv,
  python,
  cmake,
  ...
}:
let
  dylib = import ./build_package.nix {
    inherit clangStdenv python cmake;
  };
  pyenv = python.withPackages (
    pypkgs: with pypkgs; [
      auditwheel
      build
      setuptools
      wheel
    ]
  );
  targetGLIBCVerString = "17";
in
clangStdenv.mkDerivation {
  pname = "ssrjson-wheel";
  version = builtins.readFile ../version_file;
  src = ./.;
  unpackPhase = ''
    cp -r ${./..}/* .
    chmod -R 700 .
  '';
  buildPhase = ''
    SSRJSON_SONAME=ssrjson.cpython-3${python.sourceVersion.minor}-x86_64-linux-gnu.so
    cp -r pysrc ssrjson
    cp ${dylib}/$SSRJSON_SONAME ssrjson
    chmod 700 ssrjson/$SSRJSON_SONAME
    python dev_tools/check_glibc_version.py ssrjson/$SSRJSON_SONAME ${targetGLIBCVerString}
    SSRJSON_USE_NIX_PREBUILT=1 python -m build --no-isolation
    auditwheel repair --plat manylinux_2_${targetGLIBCVerString}_x86_64 dist/*.whl
    mkdir -p $out
    cp wheelhouse/*.whl $out
  '';
  buildInputs = [ pyenv ];
}
