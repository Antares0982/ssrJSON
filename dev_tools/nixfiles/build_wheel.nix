{
  pkgs,
  clangStdenv,
  python,
  cmake,
  system,
  forNonNix ? true,
  ...
}:
let
  dylib = pkgs.callPackage ./build_package.nix {
    inherit
      clangStdenv
      python
      cmake
      forNonNix
      ;
  };
  # workaround until `nix build .#ssrjson-wheel-py314` works normally
  auditwheelSkipTest =
    pypkgs:
    pypkgs.auditwheel.overrideAttrs {
      pytestCheckPhase = ":";
    };
  pyenv = python.withPackages (
    pypkgs: with pypkgs; [
      (
        if (pkgs.lib.strings.toInt python.sourceVersion.minor) < 14 then
          auditwheel
        else
          (auditwheelSkipTest pypkgs)
      )
      build
      setuptools
      wheel
    ]
  );
  targetGLIBCVerString = "17";
  auditWheelPlats = {
    "x86_64-linux" = "manylinux_2_${targetGLIBCVerString}_x86_64";
    "aarch64-linux" = "manylinux_2_${targetGLIBCVerString}_aarch64";
  };
  auditWheelPlat = auditWheelPlats.${system};
  abiflags = import ./wheel-abiflags.nix;
  abiflag = abiflags.${system};
  ssrJSONVersion = pkgs.callPackage ./ssrjson_version.nix { };
in
clangStdenv.mkDerivation {
  pname = "ssrjson-wheel";
  version = ssrJSONVersion;
  src = ./.;
  unpackPhase = ''
    cp -r ${./../..}/* .
    chmod -R 700 .
  '';
  buildPhase = ''
    SSRJSON_SONAME=ssrjson.cpython-3${python.sourceVersion.minor}-${abiflag}.so
    cp -r pysrc ssrjson
    cp licenses/* .
    cp ${dylib}/$SSRJSON_SONAME ssrjson
    chmod 700 ssrjson/$SSRJSON_SONAME
    strip --strip-all ssrjson/$SSRJSON_SONAME
    python dev_tools/check_glibc_version.py ssrjson/$SSRJSON_SONAME ${targetGLIBCVerString}
    SSRJSON_USE_NIX_PREBUILT=1 python -m build --no-isolation
    auditwheel repair --plat ${auditWheelPlat} dist/*.whl
    mkdir -p $out
    cp wheelhouse/*.whl $out
  '';
  buildInputs = [ pyenv ];
}
