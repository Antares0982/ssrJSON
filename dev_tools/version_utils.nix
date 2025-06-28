{ pkgs, pkgs-24-05, ... }:
let
  pythonVerConfig = pkgs.lib.importJSON ./pyver.json;
in
rec {
  inherit pythonVerConfig;
  pyVerToPyVerString = ver: "python3" + (builtins.toString ver);
  stablePython = builtins.getAttr (pyVerToPyVerString pythonVerConfig.latestStableVer) pkgs;
  pyVerToPkgs = ver: if ver < pythonVerConfig.latestStableVer then pkgs-24-05 else pkgs;
  pyVerToPyPackage = ver: builtins.getAttr (pyVerToPyVerString ver) (pyVerToPkgs ver);
  versions = pkgs.lib.range pythonVerConfig.minSupportVer pythonVerConfig.maxSupportVer;
  wheelBuildableVersions = pkgs.lib.range pythonVerConfig.minSupportVer pythonVerConfig.latestStableVer;
}
