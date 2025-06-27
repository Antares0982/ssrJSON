{ pkgs, pkgs-24-05, ... }:
pypkgs:
let
  pkgs = pypkgs.pkgs;
  lib = pkgs.lib;
  minorVer = lib.strings.toInt pypkgs.python.sourceVersion.minor;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-24-05; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  useNixpkgsUnstable = (minorVer >= pythonVerConfig.latestStableVer);
in
with pypkgs;
[
  objgraph
  psutil
  pytz
  pytest
  pytest-random-order
]
++ (
  with pypkgs; # needed by tests, but cannot be built in python3.14
  (lib.optionals (minorVer < 14) [
    arrow
    orjson
    pip
    pytest-xdist
  ])
)
# benchmark is only needed for latestStableVer
++ (with pypkgs; (lib.optionals (minorVer == pythonVerConfig.latestStableVer) [ pytest-benchmark ]))
