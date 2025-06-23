{
  clangStdenv,
  python,
  cmake,
  ...
}:
clangStdenv.mkDerivation rec {
  pname = "ssrjson";
  version = "0.0.0";
  src = ./.;
  unpackPhase = ''
    cp -r ${./..}/* .
    chmod -R 700 .
  '';
  # TODO aarch64?
  postInstall = ''
    patchelf --set-rpath /lib64 $out/ssrjson.so
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
