{
  pkgs ? import <nixpkgs> { },
  pkgs-legacy,
  fetchFromGitHub,
  ...
}:
let
  system = pkgs.stdenv.hostPlatform.system;
  lib = pkgs.lib;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-legacy; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  pyVerToPkgs = versionUtils.pyVerToPkgs;
  maxSupportVer = pythonVerConfig.maxSupportVer;
  minSupportVer = pythonVerConfig.minSupportVer;
  curVer = pythonVerConfig.curVer;
  supportedVers = builtins.genList (x: minSupportVer + x) (maxSupportVer - minSupportVer + 1);
  using_pythons_map =
    { py, curPkgs, ... }:
    let
      verInt = lib.strings.toInt py.sourceVersion.minor;
      startsWith =
        prefix: str:
        let
          prefixLength = builtins.stringLength prefix;
          strLength = builtins.stringLength str;
        in
        # Check if the string is long enough to contain the prefix
        # and if the substring matches the prefix
        strLength >= prefixLength && builtins.substring 0 prefixLength str == prefix;
      x = (
        py.override {
          self = x;
          packageOverrides = curPkgs.callPackage ./py_overrides.nix {
            inherit
              verInt
              curVer
              curPkgs
              pkgs-legacy
              ;
          };
        }
      );
    in
    x;
  using_pythons = (
    builtins.map using_pythons_map (
      builtins.map (supportedVer: rec {
        curPkgs = pyVerToPkgs supportedVer;
        py = (builtins.getAttr ("python3" + (builtins.toString supportedVer)) (curPkgs));
      }) supportedVers
    )
  );
  # import required python packages
  required_python_packages = pkgs.callPackage ./py_requirements.nix { inherit pkgs-legacy; };
  pyenvs_map = py: (py.withPackages required_python_packages);
  pyenvs = builtins.map pyenvs_map using_pythons;
  debuggable_py = builtins.map (
    py: (pyVerToPkgs (lib.strings.toInt py.sourceVersion.minor)).enableDebugging py
  ) using_pythons;
  pyenv_nodebug = builtins.elemAt pyenvs (curVer - minSupportVer);
  sde = pkgs.callPackage ./sde.nix { };
  llvmDbg = pkgs.enableDebugging pkgs.llvmPackages.libllvm;
  verToEnvDef = ver: {
    name = "internal_py3" + (builtins.toString ver) + "env";
    value = builtins.elemAt pyenvs (ver - minSupportVer);
  };
in
{
  inherit pyenvs; # list
  inherit pyenv_nodebug;
  inherit debuggable_py; # list
  inherit using_pythons; # list
  inherit llvmDbg;
  inherit (pkgs)
    cmake
    gdb
    ;
}
// (builtins.listToAttrs (map verToEnvDef versionUtils.versions))
// lib.optionalAttrs (system == "x86_64-linux") {
  inherit sde;
}
// lib.optionalAttrs (system != "aarch64-darwin") {
  inherit (pkgs)
    bloaty # binary size profiler
    pax-utils # lddtree
    triton-llvm # needed by coverage
    valgrind # memory profiler
    ;
}
