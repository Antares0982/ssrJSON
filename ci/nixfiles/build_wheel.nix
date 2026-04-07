{
  pkgs,
  lib,
  clangStdenv,
  python,
  cmake,
  forNonNix ? true,
  useNoGIL ? false,
  macOSTargetVersion ? 11,
  ...
}:
let
  system = clangStdenv.hostPlatform.system;
  dylib = pkgs.callPackage ./build_package.nix {
    inherit
      clangStdenv
      python
      cmake
      forNonNix
      useNoGIL
      ;
  };

  pyenv = python.withPackages (
    pypkgs:
    with pypkgs;
    [
      build
      setuptools
      wheel
    ]
    ++ lib.optionals (system != "aarch64-darwin") [
      auditwheel
    ]
  );
  targetGLIBCVerString = "34";
  auditWheelPlats = {
    "x86_64-linux" = "manylinux_2_${targetGLIBCVerString}_x86_64";
    "aarch64-linux" = "manylinux_2_${targetGLIBCVerString}_aarch64";
  };
  auditWheelPlat = auditWheelPlats.${system};
  abiflags = import ./wheel-abiflags.nix;
  abiflag = abiflags.${system};
  ssrJSONVersion = pkgs.callPackage ./ssrjson_version.nix { };
  linuxOnlyString = lib.optionalString (system != "aarch64-darwin");
  darwinOnlyString = lib.optionalString (system == "aarch64-darwin");
in
clangStdenv.mkDerivation {
  pname = "ssrjson-wheel";
  version = ssrJSONVersion;
  src = builtins.path {
    path = ./../..;
    name = "ssrjson-src";
    filter =
      path: type:
      let
        rel = lib.removePrefix (toString ./../..) path;
        allowed = [
          "/CMakeLists.txt"
          "/pyproject.toml"
          "/setup.py"
          "/MANIFEST.in"
          "/README.md"
          "/pysrc"
          "/licenses"
          "/ci/check_glibc_version.py"
        ];
      in
      lib.any (prefix: lib.hasPrefix prefix rel) allowed
      || (type == "directory" && lib.any (prefix: lib.hasPrefix rel prefix) allowed);
  };
  buildPhase =
    let
      pyver-abiname = (builtins.toString python.sourceVersion.minor) + (lib.optionalString useNoGIL "t");
    in
    linuxOnlyString ''
      SSRJSON_SONAME=ssrjson.cpython-3${pyver-abiname}-${abiflag}.so
    ''
    + darwinOnlyString ''
      SSRJSON_SONAME=ssrjson.so
    ''
    + ''
      cp -r pysrc ssrjson
      cp licenses/* .
      cp ${dylib}/$SSRJSON_SONAME ssrjson
      chmod 700 ssrjson/$SSRJSON_SONAME
    ''
    + (lib.optionalString (system == "aarch64-darwin" && forNonNix) ''
      install_name_tool -id "@rpath/ssrjson.so" ssrjson/$SSRJSON_SONAME
    '')
    + ''
      strip --strip-all ssrjson/$SSRJSON_SONAME
    ''
    + linuxOnlyString ''
      python ci/check_glibc_version.py ssrjson/$SSRJSON_SONAME ${targetGLIBCVerString}
    ''
    + ''
      SSRJSON_USE_NIX_PREBUILT=1 python -m build --no-isolation
    ''
    + linuxOnlyString ''
      auditwheel repair --plat ${auditWheelPlat} dist/*.whl
    ''
    + ''
      mkdir -p $out
    ''
    + linuxOnlyString ''
      cp wheelhouse/*.whl $out
    ''
    + darwinOnlyString ''
      cp dist/*.whl $out
      mv $out/$(ls $out) $out/ssrjson-${ssrJSONVersion}-cp3${python.sourceVersion.minor}-cp3${pyver-abiname}-macosx_${builtins.toString macOSTargetVersion}_0_arm64.whl
    '';
  buildInputs = [ pyenv ];
}
