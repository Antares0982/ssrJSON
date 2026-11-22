{
  clangStdenv,
  python,
  mkPackagingEnv,
  cmake,
  callPackage,
  lib,
  ...
}:
let
  pyenv = mkPackagingEnv { inherit python; };
  ssrJSONVersion = callPackage ./ssrjson_version.nix { };
  srcFilter = import ./source_filter.nix { inherit lib; };
in
clangStdenv.mkDerivation {
  pname = "ssrjson-tarball";
  version = ssrJSONVersion;
  src = srcFilter.mkSrc "ssrjson-src";
  buildPhase = ''
    export PATH=${cmake}/bin:$PATH
    cp licenses/* .
    rm -r licenses
    python -m build --sdist --no-isolation
    mkdir -p $out
    cp dist/*.tar.gz $out
  '';
  nativeBuildInputs = [ pyenv ];
}
