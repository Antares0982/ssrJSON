{
  clangStdenv,
  python,
  cmake,
  forNonNix ? false,
  lib,
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
  postInstall =
    (lib.optionalString forNonNix ''
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
