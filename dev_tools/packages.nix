{
  pkgs ? import <nixpkgs> { },
  pkgs-24-05,
  ...
}:
let
  lib = pkgs.lib;
  pythonVerConfig = pkgs.lib.importJSON ./pyver.json;
  curVer = pythonVerConfig.curVer;
  leastVer = pythonVerConfig.minSupportVer;
  drvs = (pkgs.callPackage ./_drvs.nix { inherit pkgs-24-05; });
  pyenv = builtins.elemAt drvs.pyenvs (curVer - leastVer);
in
# this defines the order in PATH.
# make sure pyenv selected by curVer is the first one
[ pyenv ]
++ (with drvs; [
  bloaty
  clang
  cmake
  gcc
  gdb
  python-launcher
  valgrind
])
++ drvs.pyenvs
++ lib.optionals (pkgs.system == "x86_64-linux") [
  drvs.sde
]
