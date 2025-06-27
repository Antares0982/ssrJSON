{
  pkgs ? import <nixpkgs> { },
  pkgs-24-05,
  ...
}:
let
  lib = pkgs.lib;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-24-05; };
  pythonVerConfig = versionUtils.pythonVerConfig;
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
