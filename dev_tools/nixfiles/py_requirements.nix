{ pkgs-legacy, ... }:
pypkgs:
let
  pkgs = pypkgs.pkgs;
  system = pkgs.stdenv.hostPlatform.system;
  lib = pkgs.lib;
  minorVer = lib.strings.toInt pypkgs.python.sourceVersion.minor;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-legacy; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  useNixpkgsUnstable = (minorVer > pythonVerConfig.latestUseStableNixpkgsVer);
in
with pypkgs;
[

  pytest
  pytest-random-order
  pytest-xdist
]
++ (lib.optionals (system == "x86_64-linux") [ pypkgs.psutil ])
++ (
  with pypkgs; # needed by developers
  lib.optionals (minorVer == pythonVerConfig.curVer) [
    ssrjson-benchmark
    orjson
    objgraph
  ]
)
