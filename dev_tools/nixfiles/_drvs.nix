{
  pkgs ? import <nixpkgs> { },
  pkgs-legacy,
  fetchFromGitHub,
  ...
}:
let
  lib = pkgs.lib;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-legacy; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  maxSupportVer = pythonVerConfig.maxSupportVer;
  minSupportVer = pythonVerConfig.minSupportVer;
  latestStableVer = pythonVerConfig.latestStableVer;
  curVer = pythonVerConfig.latestStableVer;
  supportedVers = builtins.genList (x: minSupportVer + x) (maxSupportVer - minSupportVer + 1);
  py315ForTest =
    (pkgs.python314.override (
      oldAttr:
      let
        newpkgs = (pkgs // { "python315" = py315ForTest; });
      in
      {
        sourceVersion = {
          major = "3";
          minor = "15";
          patch = "0";
          suffix = "a0";
        };
        noldconfigPatch = "${pkgs.path}/pkgs/development/interpreters/python/cpython/3.14/no-ldconfig.patch";
        pkgsBuildHost = newpkgs;
      }
    )).overrideAttrs
      (oldAttrs: {
        src = pkgs.fetchFromGitHub {
          owner = "python";
          repo = "cpython";
          rev = "2a54acf3c3d9f388c3d878a17ea804a801affca9";
          sha256 = "sha256-ET5cTBbcJk4Nf2vET+kLF3n5wcB6JBa0/UL+nyfoTKk=";
        };
      });
  using_pythons_map =
    { py, curPkgs, ... }:
    let
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
          packageOverrides = (
            self: super:
            {
              orjson =
                if !(startsWith ("3." + (builtins.toString curVer)) py.pythonVersion) then
                  (curPkgs.callPackage ./orjson_fixed.nix {
                    pypkgs = self;
                    inherit pkgs-legacy;
                    isDebug = false;
                  })
                else
                  (curPkgs.callPackage ./orjson-pypi.nix { pypkgs = self; });

              ssrjson-benchmark = curPkgs.callPackage ./ssrjson_benchmark.nix {
                pypkgs = self;
                inherit pkgs-legacy;
              };
            }
            // (curPkgs.lib.optionalAttrs (startsWith "3.14" py.pythonVersion) {
              pytest-random-order =
                (super.pytest-random-order.override {
                  pytest-xdist = null;
                }).overrideAttrs
                  {
                    pytestCheckPhase = ":";
                  };
            })
            // (curPkgs.lib.optionalAttrs (startsWith "3.15" py.pythonVersion) {
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
        curPkgs = if (supportedVer >= latestStableVer) then pkgs else pkgs-legacy;
        py =
          if supportedVer <= 14 then
            (builtins.getAttr ("python3" + (builtins.toString supportedVer)) (curPkgs))
          else
            py315ForTest;
      }) supportedVers
    )
  );
  # import required python packages
  required_python_packages = pkgs.callPackage ./py_requirements.nix { inherit pkgs-legacy; };
  pyenvs_map = py: (py.withPackages required_python_packages);
  pyenvs = builtins.map pyenvs_map using_pythons;
  debuggable_py = builtins.map (
    py:
    (if ((lib.strings.toInt py.sourceVersion.minor) >= latestStableVer) then pkgs else pkgs-legacy)
    .enableDebugging
      py
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
    bloaty
    cmake
    gdb
    pax-utils
    triton-llvm
    valgrind
    ; # packages
}
// (builtins.listToAttrs (map verToEnvDef versionUtils.versions))
// lib.optionalAttrs (pkgs.system == "x86_64-linux") {
  inherit sde;
}
