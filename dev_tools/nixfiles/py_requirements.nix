{ pkgs, pkgs-legacy, ... }:
pypkgs:
let
  pkgs = pypkgs.pkgs;
  lib = pkgs.lib;
  minorVer = lib.strings.toInt pypkgs.python.sourceVersion.minor;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-legacy; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  useNixpkgsUnstable = (minorVer >= pythonVerConfig.latestStableVer);
in
with pypkgs;
[
  build
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
++ (
  with pypkgs;
  lib.optionals (minorVer == pythonVerConfig.curVer) [
    ssrjson-benchmark
  ]
)
