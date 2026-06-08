{
  clangStdenv,
  python,
  cmake,
  callPackage,
  lib,
  ...
}:
let
  pyenv = python.withPackages (
    pypkgs: with pypkgs; [
      build
      setuptools
      wheel
    ]
  );
  ssrJSONVersion = callPackage ./ssrjson_version.nix { };
  srcFilter = import ./source_filter.nix { inherit lib; };
in
clangStdenv.mkDerivation {
  pname = "ssrjson-tarball";
  version = ssrJSONVersion;
  src = srcFilter.mkSrc "ssrjson-src";
  buildPhase = ''
    export PATH=${cmake}/bin:$PATH
    cp -r pysrc ssrjson
    cp licenses/* .
    rm -r licenses
    python -m build --sdist --no-isolation
    mkdir -p $out
    cp dist/*.tar.gz $out
  '';
  buildInputs = [
    pyenv
  ];
}
