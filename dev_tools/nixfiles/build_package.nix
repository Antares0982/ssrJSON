{
  clangStdenv,
  python,
  cmake,
  forNonNix ? false,
  lib,
  pax-utils,
  ...
}:
clangStdenv.mkDerivation rec {
  pname = "ssrjson";
  version = builtins.readFile ../../version_file;
  src = ./.;
  unpackPhase = ''
    cp -r ${./../..}/* .
    chmod -R 700 .
  '';
  # TODO aarch64?
  postInstall = ''
    PATH=${pax-utils}/bin:$PATH ${python}/bin/python ../dev_tools/symbol_analyze.py $out/ssrjson.so --find-needless | xargs -n 1 basename | while read tmplib; do
      patchelf $out/ssrjson.so --remove-needed $tmplib
    done
    patchelf --remove-needed libpython3.${python.sourceVersion.minor}.so.1.0 $out/ssrjson.so
  ''
  + (lib.optionalString forNonNix ''
    patchelf --set-rpath /lib64 $out/ssrjson.so
  '')
  + ''
    mv $out/ssrjson.so $out/ssrjson.cpython-3${python.sourceVersion.minor}-x86_64-linux-gnu.so
  '';
  nativeBuildInputs = [
    cmake
  ];
  buildInputs = [ python ];
  cmakeFlags = [
    "-DPREDEFINED_VERSION=${version}"
    "-DBUILD_TEST=OFF"
    "-DBUILD_SHIPPING_SIMD=ON"
  ];
}
