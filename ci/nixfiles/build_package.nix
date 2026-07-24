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
  usePgo ? true,
  sde,
  ...
}:
let
  system = clangStdenv.hostPlatform.system;
  abiflags = import ./wheel-abiflags.nix;
  abiflag = abiflags.${system};
  ssrJSONVersion = callPackage ./ssrjson_version.nix { };
  pgoProfile =
    if usePgo then
      callPackage ./build_pgo_profile.nix {
        inherit
          clangStdenv
          python
          cmake
          lib
          callPackage
          useNoGIL
          sde
          ;
      }
    else
      null;
  commonCmakeFlags = [
    "-DPREDEFINED_VERSION=${ssrJSONVersion}"
    "-DBUILD_CTESTS=OFF"
    "-DBUILD_SHIPPING_SIMD=ON"
  ]
  ++ lib.optional (
    clangStdenv.hostPlatform.isDarwin && forNonNix
  ) "-DCMAKE_OSX_DEPLOYMENT_TARGET=${builtins.toString macOSTargetVersion}.0"
  ++ lib.optional useNoGIL "-DBUILD_FREE_THREADING=ON"
  ++ lib.optional (pgoProfile != null) "-DBUILD_PGO_USE=${pgoProfile}/ssrjson.profdata";
  pyver-abiname = (builtins.toString python.sourceVersion.minor) + (lib.optionalString useNoGIL "t");
  srcFilter = import ./source_filter.nix { inherit lib; };
in
clangStdenv.mkDerivation {
  pname = "ssrjson";
  version = ssrJSONVersion;
  src = srcFilter.mkSrc "ssrjson-src";
  postInstall =
    lib.optionalString (system != "aarch64-darwin") ''
      PATH=${pax-utils}/bin:$PATH ${python}/bin/python ../ci/symbol_analyze.py $out/ssrjson.so --find-needless | xargs -n 1 basename | while read tmplib; do
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
  cmakeFlags = commonCmakeFlags;
  # Disable hardening flags that affect codegen on the hot paths. These add
  # runtime overhead without benefit for this performance-critical library
  # (fortify: fortified libc wrappers; zerocallusedregs: register clearing on
  # every function return; libcxxhardeningfast: libc++ bounds checks on the C++
  # float-formatting path). Stack protector is already disabled via CMake.
  hardeningDisable = [
    "fortify"
    "zerocallusedregs"
    "libcxxhardeningfast"
  ];
}
