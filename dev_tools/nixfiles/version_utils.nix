{ pkgs, pkgs-legacy, ... }:
let
  pythonVerConfig = pkgs.lib.importJSON ./../pyver.json;
in
rec {
  inherit pythonVerConfig;
  pyVerToPyVerString = ver: "python3" + (builtins.toString ver);
  stablePython = builtins.getAttr (pyVerToPyVerString pythonVerConfig.activeSupportingVer) pkgs;
  pyVerToPkgs = ver: if ver > pythonVerConfig.latestUseStableNixpkgsVer then pkgs else pkgs-legacy;
  pyVerToPyPackage = ver: builtins.getAttr (pyVerToPyVerString ver) (pyVerToPkgs ver);
  versions = pkgs.lib.range pythonVerConfig.minSupportVer pythonVerConfig.maxSupportVer;
  wheelBuildableVersions = pkgs.lib.range pythonVerConfig.minSupportVer pythonVerConfig.latestWheelBuildableVer;
}
