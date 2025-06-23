{
  pkgs ? import <nixpkgs> { },
  pkgs-24-05,
  fetchFromGitHub,
  ...
}:
let
  lib = pkgs.lib;
  pythonVerConfig = lib.importJSON ./pyver.json;
  maxSupportVer = pythonVerConfig.maxSupportVer;
  minSupportVer = pythonVerConfig.minSupportVer;
  latestStableVer = pythonVerConfig.latestStableVer;
  supportedVers = builtins.genList (x: minSupportVer + x) (maxSupportVer - minSupportVer + 1);
  using_pythons_map =
    { py, curPkgs, ... }:
    let
      x = (
        (curPkgs.enableDebugging py).override {
          self = x;
          packageOverrides = (
            self: super:
            {
              orjson = curPkgs.callPackage ./orjson_fixed.nix { inherit super; };
              pytest-benchmark = curPkgs.callPackage ./pytest-benchmark-fixed.nix { inherit super; };
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
            # // (curPkgs.lib.optionalAttrs (py.pythonOlder "3.11") {
            #   # tomli =
            #   #   assert (lib.versionAtLeast super.tomli.version "2.0.3");
            #   #   (super.tomli.overrideAttrs {
            #   #     src = fetchFromGitHub {
            #   #       owner = "hukkin";
            #   #       repo = super.tomli.pname;
            #   #       rev = "2.0.2";
            #   #       hash = "sha256-YduGLNprrW1yFQ2gUNuueHTtQ+bXH43hVFzDR6rKtFI=";
            #   #     };
            #   #   });
            # })
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
  required_python_packages = import ./py_requirements.nix;
  pyenvs_map = py: (py.withPackages required_python_packages);
  pyenvs = builtins.map pyenvs_map using_pythons;
  sde = pkgs.callPackage ./sde.nix { };
  llvmDbg = pkgs.enableDebugging pkgs.llvmPackages.libllvm;
in
{
  inherit pyenvs; # list
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
  py39env = builtins.elemAt pyenvs 0;
  py310env = builtins.elemAt pyenvs 1;
  py311env = builtins.elemAt pyenvs 2;
  py312env = builtins.elemAt pyenvs 3;
  py313env = builtins.elemAt pyenvs 4;
}
// lib.optionalAttrs (pkgs.system == "x86_64-linux") {
  inherit sde;
}
