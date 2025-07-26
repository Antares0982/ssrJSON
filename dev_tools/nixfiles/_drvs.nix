{
  pkgs ? import <nixpkgs> { },
  pkgs-24-05,
  fetchFromGitHub,
  ...
}:
let
  lib = pkgs.lib;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-24-05; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  maxSupportVer = pythonVerConfig.maxSupportVer;
  minSupportVer = pythonVerConfig.minSupportVer;
  latestStableVer = pythonVerConfig.latestStableVer;
  supportedVers = builtins.genList (x: minSupportVer + x) (maxSupportVer - minSupportVer + 1);
  using_pythons_map =
    { py, curPkgs, ... }:
    let
      x = (
        py.override {
          self = x;
          packageOverrides = (
            self: super:
            {
              orjson = curPkgs.callPackage ./orjson_fixed.nix { inherit self pkgs-24-05; };
              ssrjson-benchmark = curPkgs.callPackage ./ssrjson_benchmark.nix { inherit self pkgs-24-05; };
            }
            // (curPkgs.lib.optionalAttrs (py.pythonVersion == "3.14") {
              pytest-random-order =
                (super.pytest-random-order.override {
                  pytest-xdist = null;
                }).overrideAttrs
                  {
                    pytestCheckPhase = ":";
                  };
            })
          );
        }
      );
    in
    x;
  using_pythons = (
    builtins.map using_pythons_map (
      builtins.map (supportedVer: rec {
        curPkgs = if (supportedVer >= latestStableVer) then pkgs else pkgs-24-05;
        py = builtins.getAttr ("python3" + (builtins.toString supportedVer)) (curPkgs);
      }) supportedVers
    )
  );
  # import required python packages
  required_python_packages = pkgs.callPackage ./py_requirements.nix { inherit pkgs-24-05; };
  pyenvs_map = py: (py.withPackages required_python_packages);
  pyenvs = builtins.map pyenvs_map using_pythons;
  debuggable_py = builtins.map (
    py:
    (if ((lib.strings.toInt py.sourceVersion.minor) >= latestStableVer) then pkgs else pkgs-24-05)
    .enableDebugging
      py
  ) using_pythons;
  sde = pkgs.callPackage ./sde.nix { };
  llvmDbg = pkgs.enableDebugging pkgs.llvmPackages.libllvm;
  verToEnvDef = ver: {
    name = "internal_py3" + (builtins.toString ver) + "env";
    value = builtins.elemAt pyenvs (ver - minSupportVer);
  };
in
{
  inherit pyenvs; # list
  inherit debuggable_py; # list
  inherit using_pythons; # list
  inherit llvmDbg;
  inherit (pkgs)
    bloaty
    clang
    cmake
    gcc
    gdb
    python-launcher
    valgrind
    ; # packages
}
// (builtins.listToAttrs (map verToEnvDef versionUtils.versions))
// lib.optionalAttrs (pkgs.system == "x86_64-linux") {
  inherit sde;
}
