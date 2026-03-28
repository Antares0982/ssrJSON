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
in
clangStdenv.mkDerivation {
  pname = "ssrjson-tarball";
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
          "/pysrc"
          "/licenses"
          "/src"
          "/cmake"
          "/dev_tools/scm.py"
        ];
      in
      lib.any (prefix: lib.hasPrefix prefix rel) allowed
      || (type == "directory" && lib.any (prefix: lib.hasPrefix rel prefix) allowed);
  };
  buildPhase = ''
    export PATH=${cmake}/bin:$PATH
    cp -r pysrc ssrjson
    cp licenses/* .
    rm -r licenses
    python -m build --no-isolation
    mkdir -p $out
    cp dist/*.tar.gz $out
  '';
  buildInputs = [
    pyenv
  ];
}
