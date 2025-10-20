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
[ pyenv ]
++ drvs.pyenvs
++ (with drvs; [
  cmake
  gdb
])
++ lib.optionals (pkgs.system == "x86_64-linux") (
  with drvs;
  [
    sde
  ]
)
++ lib.optionals (pkgs.system != "aarch64-darwin") (
  with drvs;
  [
    bloaty
    pax-utils
    triton-llvm
    valgrind
  ]
)
