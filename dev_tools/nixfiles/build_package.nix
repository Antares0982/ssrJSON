{
  clangStdenv,
  python,
  cmake,
  forNonNix ? false,
  lib,
  pax-utils,
  callPackage,
  useNoGIL ? false,
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
  src = ./.;
  unpackPhase = ''
    cp -r ${./../..}/* .
    chmod -R 700 .
  '';
  postInstall =
    let
      pyver-abiname = (builtins.toString python.sourceVersion.minor) + (lib.optionalString useNoGIL "t");
    in
    ''
      PATH=${pax-utils}/bin:$PATH ${python}/bin/python ../dev_tools/symbol_analyze.py $out/ssrjson.so --find-needless | xargs -n 1 basename | while read tmplib; do
        patchelf $out/ssrjson.so --remove-needed $tmplib
      done
      patchelf --remove-needed libpython3.${pyver-abiname}.so.1.0 $out/ssrjson.so
    ''
    + (lib.optionalString forNonNix ''
      patchelf --set-rpath /lib64 $out/ssrjson.so
    '')
    + ''
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
  ++ (lib.optional useNoGIL "-DBUILD_FREE_THREADING=ON");
}
