{
  pkgs ? import <nixpkgs> { },
  pkgs-legacy,
  ...
}:
let
  lib = pkgs.lib;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-legacy; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  curVer = pythonVerConfig.curVer;
  leastVer = pythonVerConfig.minSupportVer;
  drvs = (pkgs.callPackage ./_drvs.nix { inherit pkgs-legacy; });
  pyenv = builtins.elemAt drvs.pyenvs (curVer - leastVer);
in
# this defines the order in PATH.
# make sure pyenv selected by curVer is the first one
[ pyenv ]
++ (with drvs; [
  bloaty
  cmake
  gdb
  pax-utils
  triton-llvm
  valgrind
])
++ drvs.pyenvs
++ lib.optionals (pkgs.system == "x86_64-linux") [
  drvs.sde
]
