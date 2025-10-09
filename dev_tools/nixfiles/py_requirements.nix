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

  pytest
  pytest-random-order
  pytest-xdist
]
++ (lib.optionals (pkgs.system == "x86_64-linux") [ pypkgs.psutil ])
++ (
  with pypkgs; # needed by developers
  lib.optionals (minorVer == pythonVerConfig.curVer) [
    ssrjson-benchmark
    orjson
    objgraph
  ]
)
