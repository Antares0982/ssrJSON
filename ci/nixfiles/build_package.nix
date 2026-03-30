{
  clangStdenv,
  python,
  cmake,
  forNonNix ? false,
  lib,
  pax-utils,
  callPackage,
  useNoGIL ? false,
  macOSTargetVersion ? 11,
  ...
}:
let
  system = clangStdenv.hostPlatform.system;
  abiflags = import ./wheel-abiflags.nix;
  abiflag = abiflags.${system};
  ssrJSONVersion = callPackage ./ssrjson_version.nix { };
in
clangStdenv.mkDerivation rec {
  pname = "ssrjson";
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
          "/src"
          "/cmake"
          "/pysrc"
          "/dev_tools/symbol_analyze.py"
          "/ci/scm.py"
        ];
      in
      lib.any (prefix: lib.hasPrefix prefix rel) allowed
      || (type == "directory" && lib.any (prefix: lib.hasPrefix rel prefix) allowed);
  };
  postInstall =
    let
      pyver-abiname = (builtins.toString python.sourceVersion.minor) + (lib.optionalString useNoGIL "t");
    in
    lib.optionalString (system != "aarch64-darwin") ''
      PATH=${pax-utils}/bin:$PATH ${python}/bin/python ../dev_tools/symbol_analyze.py $out/ssrjson.so --find-needless | xargs -n 1 basename | while read tmplib; do
        patchelf $out/ssrjson.so --remove-needed $tmplib
      done
      patchelf --remove-needed libpython3.${pyver-abiname}.so.1.0 $out/ssrjson.so
    ''
    + (lib.optionalString (forNonNix && system != "aarch64-darwin") ''
      patchelf --set-rpath /lib64 $out/ssrjson.so
    '')
    + ''
      strip $out/ssrjson.so
    ''
    + lib.optionalString (system != "aarch64-darwin") ''
      mv $out/ssrjson.so $out/ssrjson.cpython-3${pyver-abiname}-${abiflag}.so
    '';
  nativeBuildInputs = [
    cmake
  ];
  buildInputs = [ python ];
  cmakeFlags = [
    "-DPREDEFINED_VERSION=${version}"
    "-DBUILD_TEST=OFF"
    "-DBUILD_SHIPPING_SIMD=ON"
  ]
  ++ (lib.optional (
    clangStdenv.hostPlatform.isDarwin && forNonNix
  ) "-DCMAKE_OSX_DEPLOYMENT_TARGET=${builtins.toString macOSTargetVersion}.0")
  ++ (lib.optional useNoGIL "-DBUILD_FREE_THREADING=ON");
}
