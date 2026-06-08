{ lib }:
let
  root = ./../..;
in
rec {
  allowed = [
    "/CMakeLists.txt"
    "/src"
    "/cmake"
    "/pysrc"
    "/pyproject.toml"
    "/setup.py"
    "/MANIFEST.in"
    "/README.md"
    "/licenses"
    "/bench"
    "/ci/symbol_analyze.py"
    "/ci/scm.py"
    "/ci/pgo_train.py"
    "/ci/check_glibc_version.py"
  ];

  filter =
    path: type:
    let
      rel = lib.removePrefix (toString root) path;
    in
    lib.any (prefix: lib.hasPrefix prefix rel) allowed
    || (type == "directory" && lib.any (prefix: lib.hasPrefix rel prefix) allowed);

  mkSrc =
    name:
    builtins.path {
      inherit name filter;
      path = root;
    };
}
